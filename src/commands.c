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

static int usage(const char *cmd, const char *how) {
    fprintf(stderr, "%s: usage: %s\n", cmd, how); return 2;
}
static int error_path(const char *path) { perror(path); return 1; }
static int copy_stream(int input, int output) {
    char buf[8192]; ssize_t n;
    while ((n = read(input, buf, sizeof(buf))) > 0) {
        ssize_t pos = 0;
        while (pos < n) {
            ssize_t w = write(output, buf + pos, (size_t)(n - pos));
            if (w < 0) { if (errno == EINTR) continue; perror("write"); return 1; }
            pos += w;
        }
    }
    if (n < 0) { perror("read"); return 1; }
    return 0;
}
static int command_pwd(int argc, char *const *argv) {
    if (argc != 1) return usage("pwd", "pwd");
    (void)argv;
    char *path = getcwd(NULL, 0);
    if (!path) return error_path("getcwd");
    puts(path); free(path); return 0;
}
static int command_ls(int argc, char *const *argv) {
    if (argc > 2) return usage("ls", "ls [directory]");
    const char *path = argc == 2 ? argv[1] : ".";
    DIR *dir = opendir(path);
    if (!dir) return error_path(path);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        puts(entry->d_name);
    }
    if (closedir(dir) < 0) return error_path(path);
    return 0;
}
static int command_mkdir(int argc, char *const *argv) {
    if (argc != 2) return usage("mkdir", "mkdir directory");
    return mkdir(argv[1], 0777) < 0 ? error_path(argv[1]) : 0;
}
static int command_rmdir(int argc, char *const *argv) {
    if (argc != 2) return usage("rmdir", "rmdir empty-directory");
    return rmdir(argv[1]) < 0 ? error_path(argv[1]) : 0;
}
static int command_ln(int argc, char *const *argv) {
    if (argc == 4 && !strcmp(argv[1], "-s"))
        return symlink(argv[2], argv[3]) < 0 ? error_path(argv[3]) : 0;
    if (argc == 3)
        return link(argv[1], argv[2]) < 0 ? error_path(argv[2]) : 0;
    return usage("ln", "ln [-s] source link-name");
}
static int command_rm(int argc, char *const *argv) {
    if (argc != 2) return usage("rm", "rm file");
    return unlink(argv[1]) < 0 ? error_path(argv[1]) : 0;
}
static int command_mv(int argc, char *const *argv) {
    if (argc != 3) return usage("mv", "mv source destination");
    return rename(argv[1], argv[2]) < 0 ? error_path(argv[1]) : 0;
}
static int command_cp(int argc, char *const *argv) {
    if (argc != 3) return usage("cp", "cp source destination");
    struct stat source, target;
    if (stat(argv[1], &source) < 0) return error_path(argv[1]);
    if (!S_ISREG(source.st_mode)) { fputs("cp: source must be a regular file\n", stderr); return 1; }
    if (stat(argv[2], &target) == 0 && source.st_dev == target.st_dev &&
        source.st_ino == target.st_ino) {
        fputs("cp: source and destination are the same file\n", stderr); return 1;
    }
    int in = open(argv[1], O_RDONLY);
    if (in < 0) return error_path(argv[1]);
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, source.st_mode & 0777);
    if (out < 0) { close(in); return error_path(argv[2]); }
    int rc = copy_stream(in, out);
    if (close(out) < 0) { perror("close"); rc = 1; }
    if (close(in) < 0) { perror("close"); rc = 1; }
    return rc;
}
static int command_cat(int argc, char *const *argv) {
    if (argc == 1) return copy_stream(STDIN_FILENO, STDOUT_FILENO);
    int rc = 0;
    for (int i = 1; i < argc; ++i) {
        int fd = !strcmp(argv[i], "-") ? STDIN_FILENO : open(argv[i], O_RDONLY);
        if (fd < 0) { rc = error_path(argv[i]); continue; }
        if (copy_stream(fd, STDOUT_FILENO)) rc = 1;
        if (fd != STDIN_FILENO && close(fd) < 0) rc = error_path(argv[i]);
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
static int command_grep(int argc, char *const *argv) {
    if (argc < 2) return usage("grep", "grep pattern [file ...]");
    int found = 0, error = 0;
    if (argc == 2) error = grep_stream(stdin, argv[1], 0, "stdin", &found);
    for (int i = 2; i < argc; ++i) {
        FILE *fp = !strcmp(argv[i], "-") ? stdin : fopen(argv[i], "r");
        if (!fp) { error_path(argv[i]); error = 2; continue; }
        int rc = grep_stream(fp, argv[1], argc > 3, argv[i], &found);
        if (rc) error = rc;
        if (fp != stdin && fclose(fp) != 0) { perror(argv[i]); error = 2; }
    }
    return error ? error : found ? 0 : 1;
}
int commands_execute(int argc, char *const *argv) {
    if (!argc) return -1;
    const char *cmd = argv[0];
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
