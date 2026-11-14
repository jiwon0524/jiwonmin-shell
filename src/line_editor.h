#ifndef LINE_EDITOR_H
#define LINE_EDITOR_H
#include <stddef.h>
char *line_editor_read(const char *prompt, const char *const *history, size_t count);
#endif
