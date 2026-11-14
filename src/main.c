#define _POSIX_C_SOURCE 200809L
#include "plan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    char *line = NULL; size_t cap = 0;
    puts("DEU Shell / stage 1: parse-only mode (commands are NOT executed)");
    while (1) {
        fputs("deu-shell> ", stdout); fflush(stdout);
        if (getline(&line, &cap, stdin) < 0) break;
        if (!strcmp(line, "exit\n") || !strcmp(line, "exit")) break;
        Plan plan; char error[128];
        if (plan_parse(line, &plan, error, sizeof(error))) fprintf(stderr, "syntax: %s\n", error);
        else { if (plan.count) plan_print(&plan); plan_release(&plan); }
    }
    free(line); return 0;
}
