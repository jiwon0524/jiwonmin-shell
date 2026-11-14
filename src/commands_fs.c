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
int command_usage(const char *cmd, const char *how) {
    fprintf(stderr, "%s: usage: %s\n", cmd, how); return 2;
}
int command_error_path(const char *path) { perror(path); return 1; }
int command_copy_stream(int input, int output) {
    char buf[8192]; ssize_t n;
    while ((n = read(input, buf, sizeof(buf))) > 0) {
        ssize_t pos = 0;
        while (pos < n) {
            ssize_t w = write(output, buf + pos, (size_t)(n - pos));
            if (w < 0) {
                if (errno == EINTR) continue;
                perror("write");
                return 1;
            }
            if (w == 0) {
                fputs("write: no progress\n", stderr);
                return 1;
            }
            pos += w;
        }
    }
    if (n < 0) { perror("read"); return 1; }
    return 0;
}
int command_pwd(int argc, char *const *argv) {
    if (argc != 1) return command_usage("pwd", "pwd");
    (void)argv;
    char *path = getcwd(NULL, 0);
    if (!path) return command_error_path("getcwd");
    puts(path); free(path); return 0;
}
int command_ls(int argc, char *const *argv) {
    if (argc > 2) return command_usage("ls", "ls [directory]");
    const char *path = argc == 2 ? argv[1] : ".";
    DIR *dir = opendir(path);
    if (!dir) return command_error_path(path);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        puts(entry->d_name);
    }
    if (closedir(dir) < 0) return command_error_path(path);
    return 0;
}
int command_mkdir(int argc, char *const *argv) {
    if (argc != 2) return command_usage("mkdir", "mkdir directory");
    return mkdir(argv[1], 0777) < 0 ? command_error_path(argv[1]) : 0;
}
int command_rmdir(int argc, char *const *argv) {
    if (argc != 2) return command_usage("rmdir", "rmdir empty-directory");
    return rmdir(argv[1]) < 0 ? command_error_path(argv[1]) : 0;
}
int command_ln(int argc, char *const *argv) {
    if (argc == 4 && !strcmp(argv[1], "-s"))
        return symlink(argv[2], argv[3]) < 0 ? command_error_path(argv[3]) : 0;
    if (argc == 3)
        return link(argv[1], argv[2]) < 0 ? command_error_path(argv[2]) : 0;
    return command_usage("ln", "ln [-s] source link-name");
}
int command_rm(int argc, char *const *argv) {
    if (argc != 2) return command_usage("rm", "rm file");
    return unlink(argv[1]) < 0 ? command_error_path(argv[1]) : 0;
}
int command_mv(int argc, char *const *argv) {
    if (argc != 3) return command_usage("mv", "mv source destination");
    return rename(argv[1], argv[2]) < 0 ? command_error_path(argv[1]) : 0;
}
int command_cp(int argc, char *const *argv) {
    if (argc != 3) return command_usage("cp", "cp source destination");
    struct stat source, target;
    if (stat(argv[1], &source) < 0) return command_error_path(argv[1]);
    if (!S_ISREG(source.st_mode)) { fputs("cp: source must be a regular file\n", stderr); return 1; }
    if (stat(argv[2], &target) == 0 && source.st_dev == target.st_dev &&
        source.st_ino == target.st_ino) {
        fputs("cp: source and destination are the same file\n", stderr); return 1;
    }
    int in = open(argv[1], O_RDONLY);
    if (in < 0) return command_error_path(argv[1]);
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, source.st_mode & 0777);
    if (out < 0) { close(in); return command_error_path(argv[2]); }
    int rc = command_copy_stream(in, out);
    if (close(out) < 0) { perror("close"); rc = 1; }
    if (close(in) < 0) { perror("close"); rc = 1; }
    return rc;
}
