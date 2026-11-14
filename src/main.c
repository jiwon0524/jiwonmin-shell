#define _POSIX_C_SOURCE 200809L
#include "plan.h"
#include "runner.h"
#include "line_editor.h"
#include "command_check.h"
#include "chain.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HISTORY_LIMIT 100
static char *history[HISTORY_LIMIT];
static unsigned long long history_first = 1;
static size_t history_count;

static void history_clear(void) {
    for (size_t i = 0; i < history_count; ++i) free(history[i]);
    history_count = 0;
    history_first = 1;
}

static int history_add(const char *command) {
    char *copy = strdup(command);
    if (!copy) { perror("history"); return -1; }
    if (history_count == HISTORY_LIMIT) {
        free(history[0]);
        memmove(history, history + 1, (HISTORY_LIMIT - 1) * sizeof(*history));
        --history_count;
        ++history_first;
    }
    history[history_count++] = copy;
    return 0;
}

static const char *history_lookup(unsigned long long number) {
    if (number < history_first || number - history_first >= history_count) return NULL;
    return history[(size_t)(number - history_first)];
}

static int history_command(const Stage *stage) {
    size_t show = history_count;
    if (stage->argc == 2 && !strcmp(stage->argv[1], "-c")) {
        history_clear();
        return 0;
    }
    if (stage->argc > 2) {
        fputs("사용법: history [개수|-c]\n", stderr);
        return 2;
    }
    if (stage->argc == 2) {
        const char *p = stage->argv[1];
        if (!*p) { fputs("history: 양의 정수를 입력하세요.\n", stderr); return 2; }
        for (const char *q = p; *q; ++q)
            if (!isdigit((unsigned char)*q)) {
                fputs("history: 양의 정수를 입력하세요.\n", stderr); return 2;
            }
        errno = 0;
        unsigned long long n = strtoull(p, NULL, 10);
        if (errno == ERANGE) n = ULLONG_MAX;
        if (n < show) show = (size_t)n;
    }
    for (size_t i = history_count - show; i < history_count; ++i)
        printf("%5llu  %s\n", history_first + i, history[i]);
    return 0;
}

static char *expand_history(const char *input) {
    if (input[0] != '!') return strdup(input);
    const char *found = NULL;
    if (!strcmp(input, "!!")) {
        if (history_count) found = history[history_count - 1];
    } else if (input[1] && input[1] != '!') {
        for (const char *p = input + 1; *p; ++p)
            if (!isdigit((unsigned char)*p)) {
                fputs("history: 잘못된 재실행 형식입니다. (!번호 또는 !!)\n", stderr);
                return NULL;
            }
        errno = 0;
        unsigned long long n = strtoull(input + 1, NULL, 10);
        if (errno != ERANGE) found = history_lookup(n);
    }
    if (!found) {
        fputs("history: 해당 명령어 기록이 없습니다.\n", stderr);
        return NULL;
    }
    puts(found);
    return strdup(found);
}

static int execute_single(const char *command, int *exit_requested) {
    Plan plan;
    char error[128];
    int last_status=0;
    if (plan_parse(command,&plan,error,sizeof(error))!=0) {
        fprintf(stderr,"문법 오류: %s\n",error);
        return 2;
    }
    int is_history = plan.count && plan.stages[0].argc &&
                     !strcmp(plan.stages[0].argv[0],"history");
    if (is_history && (plan.count != 1 || plan.background ||
                       plan.stages[0].input_path || plan.stages[0].output_path)) {
        fputs("history: 파이프, 리다이렉션, 백그라운드 실행은 지원하지 않습니다.\n",stderr);
        last_status=2;
    } else if (is_history) {
        int clear=plan.stages[0].argc==2 && !strcmp(plan.stages[0].argv[1],"-c");
        if (!clear && history_add(command)!=0) last_status=1;
        else last_status=history_command(&plan.stages[0]);
    } else {
        for (size_t i=0;i<plan.count;i++) {
            if (!plan.stages[i].argc) continue;
            const char *name=plan.stages[i].argv[0];
            if (!command_exists(name)) {
                char *suggestion=command_suggest(name);
                if (isatty(STDERR_FILENO))
                    fprintf(stderr,"\033[31m명령어를 찾을 수 없습니다: %s\033[0m\n",name);
                if (suggestion) {
                    if (isatty(STDERR_FILENO))
                        fprintf(stderr,"\033[33m추천 명령어: %s\033[0m\n",suggestion);
                    else fprintf(stderr,"추천 명령어: %s\n",suggestion);
                }
                free(suggestion);
            }
        }
        if (history_add(command)!=0) last_status=1;
        else last_status=runner_execute(&plan,exit_requested);
    }
    plan_release(&plan);
    return last_status;
}

int main(void) {
    char *line = NULL;
    size_t cap = 0;
    int last_status = 0;
    if (runner_init() != 0) return 1;
    puts("Jiwonmin Shell 14단계 | && / || / ; / Tab / history");
    for (;;) {
        runner_poll();
        if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
            free(line);
            line = line_editor_read("jiwonmin-shell> ",
                                    (const char *const *)history, history_count);
            cap = line ? strlen(line) + 1 : 0;
            if (!line) break;
        } else if (getline(&line, &cap, stdin) < 0) break;
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        char *start = line;
        while (isspace((unsigned char)*start)) ++start;
        if (!*start) continue;
        char *command = expand_history(start);
        if (!command) { last_status = 1; continue; }
        Chain chain;
        char error[128];
        if (chain_parse(command,&chain,error,sizeof(error))!=0) {
            fprintf(stderr,"문법 오류: %s\n",error);
            last_status=2;
            free(command);
            continue;
        }
        int exit_requested=0;
        for (size_t i=0;i<chain.count;i++) {
            ChainItem *item=&chain.items[i];
            if (item->before==CHAIN_AND && last_status!=0) continue;
            if (item->before==CHAIN_OR && last_status==0) continue;
            last_status=execute_single(item->text,&exit_requested);
            if (exit_requested) break;
        }
        chain_free(&chain);
        free(command);
        if (exit_requested) break;
    }
    free(line);
    history_clear();
    runner_shutdown();
    return last_status;
}
