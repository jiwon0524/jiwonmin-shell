#define _XOPEN_SOURCE 700
#include "crab.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Jiwonmin Crab: only regular files are eligible for relocation.
 * Never follow directory symlinks; never overwrite a destination file. */
typedef struct {
    const char *wanted;
    const char *destination;
    bool approved;
    bool animate;
    unsigned long directories, files, matches, moved, failures;
    unsigned frame;
} Crab;
static volatile sig_atomic_t cancelled;

static void on_interrupt(int sig) { (void)sig; cancelled = 1; }
static char *join(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    if (na > SIZE_MAX - nb - 2) return NULL;
    char *out = malloc(na + nb + 2);
    if (out) snprintf(out, na + nb + 2, "%s/%s", a, b);
    return out;
}
static char *expand_home(const char *path) {
    if (path[0] != '~' || path[1] != '/') return strdup(path);
    const char *home = getenv("HOME");
    return home ? join(home, path + 2) : NULL;
}
/* Compact blue-crab silhouette: wide shell, two claws and four pairs of legs.
 * The alternate leg poses and lateral position are real terminal frames. */
static void frame(Crab *c, const char *action, const char *path, const char *file) {
    if (!c->animate) return;
    bool holding = !strcmp(action, "GRAB!") || !strcmp(action, "DELIVER");
    /* Three frames per directory/file visit make sideways walking visible,
       rather than moving only once when a new file is encountered. */
    unsigned steps = holding ? 2 : 3;
    for (unsigned step = 0; step < steps && !cancelled; ++step) {
        unsigned position = (c->frame / 2) % 18;
        unsigned pose = c->frame++ % 4;
        printf("\033[H\033[2J\033[?25l");
        puts("  JIWONMIN CRAB  |  File Hunter\n");
        printf("  %-10s %.110s\n\n", action, path);
        for (unsigned i = 0; i < position; ++i) putchar(' ');
        /* Indent the entire crab, not just its first line. */
        static const char *walk[4][5] = {
            {"  \\__        __/", " (  \\________/  )", " /\\  ( o  o )  /\\", "<  > (________) <  >", " /\\ / /  \\ \\ /\\"},
            {"  \\__        __/", " (  \\________/  )", " \\/  ( o  o )  \\/", "<  > (________) <  >", " \\/ \\ \\  / / \\/"},
            {"  \\__        __/", " (  \\________/  )", " /\\  ( o  o )  /\\", "<  > (________) <  >", " \\/ / /  \\ \\ \\/"},
            {"  \\__        __/", " (  \\________/  )", " \\/  ( o  o )  \\/", "<  > (________) <  >", " /\\ \\ \\  / / /\\"}
        };
        for (unsigned row = 0; row < 5; ++row) {
            printf("%*s%s", (int)position, "", walk[pose][row]);
            if (holding && row == 3) printf("  <==[%s]", file ? file : "FILE");
            putchar('\n');
        }
        printf("\n  folders: %lu  files: %lu  found: %lu  moved: %lu\n",
               c->directories, c->files, c->matches, c->moved);
        fflush(stdout);
        struct timespec delay = {.tv_sec = 0, .tv_nsec = 85000000};
        nanosleep(&delay, NULL);
    }
}
static int confirm_move(const char *source, const char *target) {
    FILE *tty = fopen("/dev/tty", "r+");
    if (!tty) { fprintf(stderr, "jiwonmin: interactive confirmation unavailable (use --yes for scripted runs)\n"); return 0; }
    fprintf(tty, "Move '%s' to '%s'? [y/N] ", source, target);
    fflush(tty);
    char response[16] = {0};
    int yes = fgets(response, sizeof(response), tty) && (response[0] == 'y' || response[0] == 'Y');
    fclose(tty);
    return yes;
}
/* Hard link + unlink: no replacement, no cross-device partial copy, regular files only.
 * If removal fails, the new link is rolled back. */
static int move_file(const char *source, const char *target) {
    struct stat info;
    if (lstat(source, &info) != 0 || !S_ISREG(info.st_mode)) { errno = EINVAL; return -1; }
    if (link(source, target) != 0) return -1;
    if (unlink(source) == 0) return 0;
    int saved = errno;
    if (unlink(target) != 0) perror("jiwonmin: rollback");
    errno = saved;
    return -1;
}
static void scan(Crab *c, const char *directory) {
    if (cancelled) return;
    DIR *dir = opendir(directory);
    if (!dir) { perror(directory); ++c->failures; return; }
    ++c->directories;
    frame(c, "WALK", directory, NULL);
    struct dirent *entry;
    while (!cancelled && (entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char *path = join(directory, entry->d_name);
        if (!path) { ++c->failures; break; }
        struct stat st;
        if (lstat(path, &st) != 0) { perror(path); ++c->failures; free(path); continue; }
        if (S_ISDIR(st.st_mode)) {
            scan(c, path);
        } else if (S_ISREG(st.st_mode)) {
            ++c->files;
            frame(c, "SEARCH", path, NULL);
            if (!strcmp(entry->d_name, c->wanted)) {
                ++c->matches;
                frame(c, "GRAB!", path, entry->d_name);
                char *target = join(c->destination, entry->d_name);
                if (!target) { ++c->failures; free(path); continue; }
                if (c->approved || confirm_move(path, target)) {
                    if (cancelled) { free(target); free(path); break; }
                    if (move_file(path, target) == 0) {
                        ++c->moved;
                        frame(c, "DELIVER", target, entry->d_name);
                        printf("[MOVED] %s -> %s\n", path, target);
                    } else { fprintf(stderr, "[FAILED] %s -> %s: %s\n", path, target, strerror(errno)); ++c->failures; }
                } else { printf("[SKIPPED] %s\n", path); }
                free(target);
            }
        }
        free(path);
    }
    closedir(dir);
}
int crab_execute(int argc, char *const *argv) {
    if (argc != 6 && argc != 7) {
        fputs("Usage: jiwonmin ROOT --find FILENAME --move DEST [--yes]\n", stderr); return 2;
    }
    if (strcmp(argv[2], "--find") || strcmp(argv[4], "--move") ||
        (argc == 7 && strcmp(argv[6], "--yes")) || !argv[3][0] ||
        strchr(argv[3], '/') || !strcmp(argv[3], ".") || !strcmp(argv[3], "..")) {
        fputs("jiwonmin: invalid arguments; FILENAME must be a single file name\n", stderr); return 2;
    }
    char *root = expand_home(argv[1]);
    char *dest = expand_home(argv[5]);
    if (!root || !dest) { free(root); free(dest); fputs("jiwonmin: out of memory or HOME not set\n", stderr); return 1; }
    struct stat sr, sd;
    if (lstat(root, &sr) != 0 || !S_ISDIR(sr.st_mode) ||
        lstat(dest, &sd) != 0 || !S_ISDIR(sd.st_mode)) {
        fputs("jiwonmin: ROOT and DEST must be existing real directories\n", stderr);
        free(root); free(dest); return 1;
    }
    /* Never recurse into the destination itself. Use canonical paths to catch
       destination nested under root, including path aliases. */
    char *canonical_root = realpath(root, NULL);
    char *canonical_dest = realpath(dest, NULL);
    if (!canonical_root || !canonical_dest) {
        perror("jiwonmin: realpath"); free(canonical_root); free(canonical_dest); free(root); free(dest); return 1;
    }
    size_t n = strlen(canonical_root);
    if (!strncmp(canonical_root, canonical_dest, n) &&
        (canonical_dest[n] == '/' || canonical_dest[n] == '\0')) {
        fputs("jiwonmin: destination cannot be inside the search tree\n", stderr);
        free(canonical_root); free(canonical_dest); free(root); free(dest); return 2;
    }
    Crab c = {.wanted=argv[3], .destination=canonical_dest, .approved=(argc == 7),
              .animate=isatty(STDOUT_FILENO) && isatty(STDIN_FILENO)};
    struct sigaction old_int, act = {0};
    act.sa_handler = on_interrupt;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, &old_int);
    cancelled = 0;
    scan(&c, canonical_root);
    sigaction(SIGINT, &old_int, NULL);
    if (c.animate) printf("\033[?25h\n");
    printf("[JCRAB] directories=%lu files=%lu matches=%lu moved=%lu failures=%lu%s\n",
           c.directories, c.files, c.matches, c.moved, c.failures,
           cancelled ? " INTERRUPTED" : "");
    free(canonical_root); free(canonical_dest); free(root); free(dest);
    return (cancelled ? 130 : c.failures ? 1 : 0);
}
