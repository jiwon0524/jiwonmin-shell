#define _POSIX_C_SOURCE 200809L
#include "plan.h"
#include "runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    char *line = NULL;
    size_t cap = 0;
    int last_status = 0;
    puts("DEU Shell / stage 2: single-command execution (no pipes/redirection/background yet)");
    for (;;) {
        if (isatty(fileno(stdin))) { fputs("deu-shell> ", stdout); fflush(stdout); }
        if (getline(&line, &cap, stdin) < 0) break;
        Plan plan;
        char error[128];
        if (plan_parse(line, &plan, error, sizeof(error)) != 0) {
            fprintf(stderr, "syntax: %s\n", error);
            last_status = 2;
            continue;
        }
        int exit_requested = 0;
        last_status = runner_execute(&plan, &exit_requested);
        plan_release(&plan);
        if (exit_requested) break;
    }
    free(line);
    return last_status;
}
