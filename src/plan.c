#define _POSIX_C_SOURCE 200809L
#include "plan.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { WORD, PIPE, INPUT, OUTPUT, APPEND, BACKGROUND } TokenType;
typedef struct { TokenType type; char *text; } Token;
static void fail(char *err, size_t cap, const char *message) {
    if (cap) snprintf(err, cap, "%s", message);
}
static void clear_tokens(Token *items, size_t n) {
    for (size_t i = 0; i < n; ++i) free(items[i].text);
}
static int push_token(Token *items, size_t *count, TokenType type, const char *data, size_t len,
                      char *err, size_t cap) {
    if (*count >= PLAN_MAX_TOKENS) { fail(err, cap, "too many tokens"); return -1; }
    char *copy = NULL;
    if (type == WORD) {
        copy = malloc(len + 1);
        if (!copy) { fail(err, cap, "out of memory"); return -1; }
        memcpy(copy, data, len); copy[len] = '\0';
    }
    items[(*count)++] = (Token){type, copy};
    return 0;
}
static int tokenize(const char *line, Token *tokens, size_t *n, char *err, size_t cap) {
    size_t pos = 0, len = strlen(line);
    if (len >= PLAN_TEXT_CAP) { fail(err, cap, "input too long"); return -1; }
    while (pos < len) {
        if (isspace((unsigned char)line[pos])) { ++pos; continue; }
        char ch = line[pos];
        if (ch == '|' || ch == '<' || ch == '>' || ch == '&') {
            TokenType type = ch == '|' ? PIPE : ch == '<' ? INPUT : ch == '&' ? BACKGROUND : OUTPUT;
            if (ch == '>' && pos + 1 < len && line[pos + 1] == '>') { type = APPEND; ++pos; }
            if (push_token(tokens, n, type, NULL, 0, err, cap)) return -1;
            ++pos; continue;
        }
        char buffer[PLAN_TEXT_CAP]; size_t used = 0;
        int quote = 0, started = 0;
        while (pos < len) {
            ch = line[pos];
            if (!quote && (isspace((unsigned char)ch) || ch == '|' || ch == '<' || ch == '>' || ch == '&')) break;
            started = 1;
            if (ch == '\\') {
                if (++pos == len) { fail(err, cap, "unfinished escape"); return -1; }
                buffer[used++] = line[pos++]; continue;
            }
            if (ch == '\'' || ch == '"') {
                if (!quote) { quote = ch; ++pos; continue; }
                if (quote == ch) { quote = 0; ++pos; continue; }
            }
            buffer[used++] = ch; ++pos;
        }
        if (quote) { fail(err, cap, "unclosed quote"); return -1; }
        if (started && push_token(tokens, n, WORD, buffer, used, err, cap)) return -1;
    }
    return 0;
}
void plan_release(Plan *plan) {
    for (size_t s = 0; s < plan->count; ++s) {
        Stage *st = &plan->stages[s];
        for (size_t a = 0; a < st->argc; ++a) free(st->argv[a]);
        free(st->input_path); free(st->output_path);
    }
    memset(plan, 0, sizeof(*plan));
}
int plan_parse(const char *line, Plan *out, char *err, size_t cap) {
    Token tokens[PLAN_MAX_TOKENS] = {0}; size_t n = 0;
    memset(out, 0, sizeof(*out));
    if (cap) err[0] = '\0';
    if (tokenize(line, tokens, &n, err, cap)) goto invalid;
    if (!n) { clear_tokens(tokens, n); return 0; }
    out->count = 1;
    for (size_t i = 0; i < n; ++i) {
        Stage *st = &out->stages[out->count - 1];
        Token tk = tokens[i];
        if (tk.type == WORD) {
            if (st->argc >= PLAN_MAX_ARGS) { fail(err, cap, "too many arguments"); goto invalid; }
            st->argv[st->argc] = strdup(tk.text);
            if (!st->argv[st->argc]) { fail(err, cap, "out of memory"); goto invalid; }
            st->argc++; st->argv[st->argc] = NULL;
        } else if (tk.type == PIPE) {
            if (!st->argc || out->count >= PLAN_MAX_STAGES) { fail(err, cap, "invalid pipeline"); goto invalid; }
            out->count++;
        } else if (tk.type == BACKGROUND) {
            if (i != n - 1) { fail(err, cap, "& must be last"); goto invalid; }
            out->background = 1;
        } else {
            if (i + 1 >= n || tokens[i + 1].type != WORD) { fail(err, cap, "redirection requires filename"); goto invalid; }
            char **target = tk.type == INPUT ? &st->input_path : &st->output_path;
            if (*target) { fail(err, cap, "duplicate redirection"); goto invalid; }
            *target = strdup(tokens[++i].text);
            if (!*target) { fail(err, cap, "out of memory"); goto invalid; }
            if (tk.type != INPUT) st->append_output = tk.type == APPEND;
        }
    }
    for (size_t i = 0; i < out->count; ++i) {
        if (!out->stages[i].argc) { fail(err, cap, "empty command in pipeline"); goto invalid; }
    }
    clear_tokens(tokens, n); return 0;
invalid:
    clear_tokens(tokens, n); plan_release(out); return -1;
}
void plan_print(const Plan *plan) {
    printf("[COMMAND PLAN] stages=%zu background=%s\n", plan->count, plan->background ? "yes" : "no");
    for (size_t i = 0; i < plan->count; ++i) {
        const Stage *st = &plan->stages[i];
        printf(" stage %zu:", i + 1);
        for (size_t a = 0; a < st->argc; ++a) printf(" [%s]", st->argv[a]);
        printf("\n  stdin=%s stdout=%s mode=%s\n", st->input_path ? st->input_path : "(inherit)",
               st->output_path ? st->output_path : "(inherit)", st->append_output ? "append" : "truncate");
    }
}
