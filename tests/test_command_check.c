#define _POSIX_C_SOURCE 200809L
#include "../src/command_check.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    size_t a=0,b=0;
    assert(command_word("  grep foo",&a,&b) && a==2 && b==6);
    assert(!command_word("   ",&a,&b));
    assert(command_exists("grep"));
    assert(command_exists("cd"));
    assert(!command_exists("some_missing_command_xyz_987"));
    char *s=command_suggest("grpe");
    assert(s && !strcmp(s,"grep"));free(s);
    s=command_suggest("mkdri");
    assert(s && !strcmp(s,"mkdir"));free(s);
    s=command_suggest("some_missing_command_xyz_987");
    assert(!s);
    puts("8 command color/suggestion checks passed");
    return 0;
}
