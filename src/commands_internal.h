#ifndef JIWONMIN_COMMANDS_INTERNAL_H
#define JIWONMIN_COMMANDS_INTERNAL_H
#include <sys/types.h>
int command_usage(const char *cmd, const char *how);
int command_error_path(const char *path);
int command_copy_stream(int input, int output);
int command_pwd(int argc, char *const *argv);
int command_ls(int argc, char *const *argv);
int command_mkdir(int argc, char *const *argv);
int command_rmdir(int argc, char *const *argv);
int command_ln(int argc, char *const *argv);
int command_cp(int argc, char *const *argv);
int command_rm(int argc, char *const *argv);
int command_mv(int argc, char *const *argv);
int command_cat(int argc, char *const *argv);
int command_grep(int argc, char *const *argv);
#endif
