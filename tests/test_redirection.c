#include "plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check(const char *line, size_t n) {
    Plan p; char err[128]; assert(plan_parse(line, &p, err, sizeof err) == 0);
    assert(p.count == 1 && p.stages[0].redir_count == n);
    plan_release(&p);
}
int main(void) {
    check("echo hello >> a", 1);
    check("ls missing 2> err", 1);
    check("ls missing 2>> err", 1);
    check("ls missing > out 2>&1", 2);
    check("ls missing 2>&1 > out", 2);
    Plan p; char err[128]; assert(plan_parse("ls >", &p, err, sizeof err) != 0);
    puts("6 extended redirection parser checks passed");
    return 0;
}
