#define _POSIX_C_SOURCE 200809L
#include "runner.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_for(pid_t child) {
    int status;
    for (;;) {
        if (waitpid(child, &status, 0) >= 0) break;
        if (errno == EINTR) continue;
        perror("waitpid"); return 1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

int runner_execute(const Plan *plan, int *exit_requested) {
    *exit_requested = 0;
    if (plan->count == 0) return 0;
    if (plan->count != 1 || plan->background || plan->stages[0].input_path || plan->stages[0].output_path) {
        fputs("deu-shell: pipeline, redirection and background execution are not supported in stage 2\n", stderr);
        return 2;
    }
    const Stage *stage = &plan->stages[0];
    const char *cmd = stage->argv[0];
    if (strcmp(cmd, "exit") == 0) {
        if (stage->argc != 1) { fputs("exit: no arguments supported\n", stderr); return 2; }
        *exit_requested = 1; return 0;
    }
    if (strcmp(cmd, "cd") == 0) {
        if (stage->argc > 2) { fputs("cd: too many arguments\n", stderr); return 2; }
        const char *target = stage->argc == 2 ? stage->argv[1] : getenv("HOME");
        if (!target) { fputs("cd: HOME is not set\n", stderr); return 1; }
        if (chdir(target) != 0) { perror("cd"); return 1; }
        return 0;
    }
    pid_t child = fork();
    if (child < 0) { perror("fork"); return 1; }
    if (child == 0) {
        execvp(cmd, stage->argv);
        fprintf(stderr, "deu-shell: %s: %s\n", cmd, strerror(errno));
        _exit(errno == ENOENT ? 127 : 126);
    }
    return wait_for(child);
}
