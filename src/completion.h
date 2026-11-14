#ifndef COMPLETION_H
#define COMPLETION_H
#include <stddef.h>
/* Returns a new full input line, or NULL if no completion is available. */
char *completion_expand(const char *line, size_t cursor, size_t *new_cursor, size_t *matches);
#endif
