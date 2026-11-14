#include "commands.h"
#include "commands_internal.h"
#include <string.h>
#include "screenshot.h"

int commands_execute(int argc, char *const *argv) {
    if (!argc) return -1;
    const char *cmd = argv[0];
    if (!strcmp(cmd, "jiwonmin")) return screenshot_execute(argc, argv);
    if (!strcmp(cmd, "pwd")) return command_pwd(argc, argv);
    if (!strcmp(cmd, "ls")) return command_ls(argc, argv);
    if (!strcmp(cmd, "mkdir")) return command_mkdir(argc, argv);
    if (!strcmp(cmd, "rmdir")) return command_rmdir(argc, argv);
    if (!strcmp(cmd, "ln")) return command_ln(argc, argv);
    if (!strcmp(cmd, "cp")) return command_cp(argc, argv);
    if (!strcmp(cmd, "rm")) return command_rm(argc, argv);
    if (!strcmp(cmd, "mv")) return command_mv(argc, argv);
    if (!strcmp(cmd, "cat")) return command_cat(argc, argv);
    if (!strcmp(cmd, "grep")) return command_grep(argc, argv);
    return -1;
}
