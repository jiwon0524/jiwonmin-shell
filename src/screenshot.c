#define _POSIX_C_SOURCE 200809L
#include "screenshot.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int run_capture(char *const args[]) {
    pid_t pid = fork();
    if (pid < 0) { perror("jiwonmin: fork"); return -1; }
    if (pid == 0) { execvp(args[0], args); _exit(errno == ENOENT ? 127 : 126); }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("jiwonmin: waitpid"); return -1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
}

static int ensure_directory(const char *path) {
    if (mkdir(path, 0700) == 0 || errno == EEXIST) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
    }
    perror("jiwonmin: screenshot directory"); return -1;
}

int screenshot_execute(int argc, char *const *argv) {
    const char *output = NULL;
    int delay = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) output = argv[++i];
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            char *end = NULL;
            errno = 0;
            long n = strtol(argv[++i], &end, 10);
            if (errno || !end || *end || n < 0 || n > 60) {
                fputs("jiwonmin: delay must be 0..60 seconds\n", stderr); return 2;
            }
            delay = (int)n;
        } else {
            fputs("usage: jiwonmin [-d seconds] [-o filename.png]\n", stderr);
            return 2;
        }
    }
    const char *session = getenv("XDG_SESSION_TYPE");
    if (session && strcmp(session, "x11")) {
        fputs("jiwonmin: this version supports X11 sessions only\n", stderr); return 1;
    }
    const char *display = getenv("DISPLAY");
    if (!display || !*display) {
        fputs("jiwonmin: DISPLAY is not set; run from your graphical desktop\n", stderr); return 1;
    }
    char path[PATH_MAX];
    if (!output) {
        const char *home = getenv("HOME");
        if (!home || !*home) { fputs("jiwonmin: HOME is not set\n", stderr); return 1; }
        char dir[PATH_MAX];
        int n = snprintf(dir, sizeof(dir), "%s/Pictures", home);
        if (n < 0 || (size_t)n >= sizeof(dir) || ensure_directory(dir)) return 1;
        n = snprintf(dir, sizeof(dir), "%s/Pictures/Jiwonmin", home);
        if (n < 0 || (size_t)n >= sizeof(dir) || ensure_directory(dir)) return 1;
        time_t now = time(NULL);
        struct tm local;
        if (now == (time_t)-1 || !localtime_r(&now, &local)) return 1;
        char stamp[32];
        if (!strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &local)) return 1;
        int found = 0;
        for (int i = 0; i < 1000; ++i) {
            n = snprintf(path, sizeof(path), "%s/screenshot_%s_%03d.png", dir, stamp, i);
            if (n < 0 || (size_t)n >= sizeof(path)) return 1;
            if (access(path, F_OK) != 0 && errno == ENOENT) { found = 1; break; }
        }
        if (!found) { fputs("jiwonmin: no free screenshot filename\n", stderr); return 1; }
        output = path;
    } else if (access(output, F_OK) == 0) {
        fputs("jiwonmin: destination already exists; refusing to overwrite\n", stderr); return 1;
    }
    char seconds[16];
    snprintf(seconds, sizeof(seconds), "%d", delay);
    char *scrot[] = {"scrot", "-d", seconds, "--", (char *)output, NULL};
    char *gnome[] = {"gnome-screenshot", "-d", seconds, "-f", (char *)output, NULL};
    int rc = run_capture(scrot);
    if (rc == 127) rc = run_capture(gnome);
    if (rc == 127) {
        fputs("jiwonmin: install scrot (sudo apt install scrot) or gnome-screenshot\n", stderr);
        return 1;
    }
    if (rc != 0) { fputs("jiwonmin: screen capture failed\n", stderr); return 1; }
    struct stat st;
    if (stat(output, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size == 0) {
        fputs("jiwonmin: capture did not create a valid file\n", stderr); return 1;
    }
    printf("Screenshot saved: %s\n", output);
    return 0;
}
