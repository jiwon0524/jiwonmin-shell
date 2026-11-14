#define _POSIX_C_SOURCE 200809L
#include "commands.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "commands_internal.h"
int command_cat(int argc, char *const *argv) {
    if (argc == 1) return command_copy_stream(STDIN_FILENO, STDOUT_FILENO);
    int rc = 0;
    for (int i = 1; i < argc; ++i) {
        int fd = !strcmp(argv[i], "-") ? STDIN_FILENO : open(argv[i], O_RDONLY);
        if (fd < 0) { rc = command_error_path(argv[i]); continue; }
        if (command_copy_stream(fd, STDOUT_FILENO)) rc = 1;
        if (fd != STDIN_FILENO && close(fd) < 0) rc = command_error_path(argv[i]);
    }
    return rc;
}
static int grep_stream(FILE *stream, const char *needle, int print_name, const char *name, int *found) {
    char *line = NULL; size_t cap = 0; ssize_t n;
    while ((n = getline(&line, &cap, stream)) >= 0) {
        if (strstr(line, needle)) {
            *found = 1;
            if (print_name && printf("%s:", name) < 0) { free(line); return 2; }
            if (fwrite(line, 1, (size_t)n, stdout) != (size_t)n) { free(line); return 2; }
            if (n && line[n-1] != '\n') putchar('\n');
        }
    }
    free(line);
    if (ferror(stream)) { perror(name); return 2; }
    return 0;
}
int command_grep(int argc, char *const *argv) {
    if (argc < 2) return command_usage("grep", "grep pattern [file ...]");
    int found = 0, error = 0;
    if (argc == 2) error = grep_stream(stdin, argv[1], 0, "stdin", &found);
    for (int i = 2; i < argc; ++i) {
        FILE *fp = !strcmp(argv[i], "-") ? stdin : fopen(argv[i], "r");
        if (!fp) { command_error_path(argv[i]); error = 2; continue; }
        int rc = grep_stream(fp, argv[1], argc > 3, argv[i], &found);
        if (rc) error = rc;
        if (fp != stdin && fclose(fp) != 0) { perror(argv[i]); error = 2; }
    }
    return error ? error : found ? 0 : 1;
}
