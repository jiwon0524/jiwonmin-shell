#ifndef COMMAND_CHECK_H
#define COMMAND_CHECK_H
#include <stddef.h>
/* 1 = executable/builtin, 0 = unknown; explicit paths are checked too. */
int command_exists(const char *name);
/* Caller frees result. NULL means no sufficiently close suggestion. */
char *command_suggest(const char *name);
/* First command word bounds, ignoring leading spaces; 0 if none. */
int command_word(const char *line, size_t *begin, size_t *end);
#endif
