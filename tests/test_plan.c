#include "plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void ok(const char *input, size_t stages, int bg) {
    Plan p; char e[128]; assert(plan_parse(input, &p, e, sizeof e) == 0);
    assert(p.count == stages && p.background == bg); plan_release(&p);
}
static void bad(const char *input) {
    Plan p; char e[128]; assert(plan_parse(input, &p, e, sizeof e) != 0);
}
int main(void) {
    ok("", 0, 0); ok("pwd", 1, 0); ok("cat in.txt | grep 'hello world' > out.txt &", 2, 1);
    ok("echo \"a|b\" >> result.txt", 1, 0); ok("echo ''", 1, 0);
    bad("| ls"); bad("ls |"); bad("ls >"); bad("ls & pwd"); bad("echo 'broken"); bad("ls > a > b");
    puts("11 parser checks passed"); return 0;
}
