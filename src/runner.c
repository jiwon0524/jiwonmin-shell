#define _POSIX_C_SOURCE 200809L
#include "runner.h"
#include "commands.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_JOBS 32
typedef struct {
    int used;
    int id;
    pid_t pgid;
    pid_t pids[PLAN_MAX_STAGES];
    unsigned char state[PLAN_MAX_STAGES]; /* 0 running, 1 stopped, 2 finished */
    size_t count;
    pid_t last_pid;
    int last_status;
} Job;
static Job jobs[MAX_JOBS];
static int next_id = 1;
static int interactive = 0;
static pid_t shell_pgid;
static struct termios shell_modes;

static int status_code(int status) {
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}
static int all_done(const Job *job) {
    for (size_t i = 0; i < job->count; ++i) if (job->state[i] != 2) return 0;
    return 1;
}
static int all_stopped(const Job *job) {
    int any_stopped = 0;
    for (size_t i = 0; i < job->count; ++i) {
        if (job->state[i] == 0) return 0;
        if (job->state[i] == 1) any_stopped = 1;
    }
    return any_stopped;
}
static void record_status(Job *job, pid_t pid, int status) {
    for (size_t i = 0; i < job->count; ++i) {
        if (job->pids[i] != pid) continue;
        if (WIFSTOPPED(status)) job->state[i] = 1;
        else if (WIFCONTINUED(status)) job->state[i] = 0;
        else if (WIFEXITED(status) || WIFSIGNALED(status)) job->state[i] = 2;
        if (pid == job->last_pid && (WIFEXITED(status) || WIFSIGNALED(status)))
            job->last_status = status_code(status);
        return;
    }
}
static Job *allocate_job(void) {
    for (size_t i = 0; i < MAX_JOBS; ++i) if (!jobs[i].used) {
        memset(&jobs[i], 0, sizeof(jobs[i]));
        jobs[i].used = 1;
        jobs[i].id = next_id++;
        return &jobs[i];
    }
    return NULL;
}
static void record_any(pid_t pid, int status) {
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].used) record_status(&jobs[i], pid, status);
}
void runner_poll(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
        record_any(pid, status);
    for (size_t i = 0; i < MAX_JOBS; ++i) if (jobs[i].used && all_done(&jobs[i])) {
        printf("[%d] Done (status %d)\n", jobs[i].id, jobs[i].last_status);
        jobs[i].used = 0;
    }
}
static void ignore_signal(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(sig, &sa, NULL);
}
static void default_signal(int sig) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(sig, &sa, NULL);
}
int runner_init(void) {
    interactive = isatty(STDIN_FILENO);
    shell_pgid = getpgrp();
    if (!interactive) return 0;
    /* When started in the background, wait until the terminal gives us control. */
    while (tcgetpgrp(STDIN_FILENO) != getpgrp()) kill(-getpgrp(), SIGTTIN);
    ignore_signal(SIGINT);
    ignore_signal(SIGQUIT);
    ignore_signal(SIGTSTP);
    ignore_signal(SIGTTIN);
    ignore_signal(SIGTTOU);
    if (setpgid(0, 0) < 0 && errno != EACCES && errno != EPERM) { perror("setpgid"); return -1; }
    shell_pgid = getpgrp();
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0) { perror("tcsetpgrp"); return -1; }
    if (tcgetattr(STDIN_FILENO, &shell_modes) < 0) { perror("tcgetattr"); return -1; }
    return 0;
}
void runner_shutdown(void) {
    /* Notify remaining jobs that the shell is exiting. */
    runner_poll();
    for (size_t i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].used || jobs[i].pgid <= 0) continue;
        if (kill(-jobs[i].pgid, SIGHUP) < 0 && errno != ESRCH)
            perror("SIGHUP");
        if (all_stopped(&jobs[i]) &&
            kill(-jobs[i].pgid, SIGCONT) < 0 && errno != ESRCH)
            perror("SIGCONT");
    }
    if (interactive && tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp: restore shell");
}
static int redirect_fd(const char *path, int target, int flags, mode_t mode) {
    int fd = open(path, flags, mode);
    if (fd < 0) { perror(path); return -1; }
    if (dup2(fd, target) < 0) { perror("dup2"); close(fd); return -1; }
    close(fd);
    return 0;
}
static int apply_redirections(const Stage *stage) {
    if (stage->input_path && redirect_fd(stage->input_path, STDIN_FILENO, O_RDONLY, 0) < 0) return -1;
    if (stage->output_path) {
        int flags = O_WRONLY | O_CREAT | (stage->append_output ? O_APPEND : O_TRUNC);
        if (redirect_fd(stage->output_path, STDOUT_FILENO, flags, 0666) < 0) return -1;
    }
    return 0;
}
static int builtin(const Stage *stage, int *exit_requested) {
    const char *cmd = stage->argv[0];
    if (!strcmp(cmd, "exit")) {
        if (stage->argc != 1) { fputs("exit: no arguments supported\n", stderr); return 2; }
        *exit_requested = 1; return 0;
    }
    if (!strcmp(cmd, "cd")) {
        if (stage->argc > 2) { fputs("cd: too many arguments\n", stderr); return 2; }
        const char *target = stage->argc == 2 ? stage->argv[1] : getenv("HOME");
        if (!target) { fputs("cd: HOME is not set\n", stderr); return 1; }
        if (chdir(target) < 0) { perror("cd"); return 1; }
        char *cwd = getcwd(NULL, 0);
        if (!cwd) { perror("getcwd"); return 1; }
        int ok = setenv("PWD", cwd, 1);
        free(cwd);
        if (ok < 0) { perror("setenv PWD"); return 1; }
        return 0;
    }
    if (!strcmp(cmd, "export")) {
        if (stage->argc < 2) { fputs("사용법: export NAME=VALUE [...]\n", stderr); return 2; }
        for (size_t i = 1; i < stage->argc; ++i) {
            const char *arg = stage->argv[i];
            const char *eq = strchr(arg, '=');
            size_t n = eq ? (size_t)(eq - arg) : 0;
            if (!n || !( (arg[0] >= 'A' && arg[0] <= 'Z') ||
                          (arg[0] >= 'a' && arg[0] <= 'z') || arg[0] == '_')) {
                fputs("export: NAME=VALUE 형식이 필요합니다.\n", stderr); return 2;
            }
            for (size_t j=1;j<n;++j)
                if (!((arg[j]>='A' && arg[j]<='Z') || (arg[j]>='a' && arg[j]<='z') ||
                      (arg[j]>='0' && arg[j]<='9') || arg[j]=='_')) {
                    fputs("export: 잘못된 변수 이름\n", stderr); return 2;
                }
            char *name = strndup(arg, n);
            if (!name) { perror("export"); return 1; }
            int rc = setenv(name, eq + 1, 1);
            free(name);
            if (rc) { perror("export"); return 1; }
        }
        return 0;
    }
    if (!strcmp(cmd, "unset")) {
        if (stage->argc < 2) { fputs("사용법: unset NAME [...]\n", stderr); return 2; }
        for (size_t i=1;i<stage->argc;++i)
            if (unsetenv(stage->argv[i]) < 0) { perror("unset"); return 1; }
        return 0;
    }
    if (!strcmp(cmd, "env")) {
        if (stage->argc != 1) { fputs("사용법: env\n", stderr); return 2; }
        extern char **environ;
        for (char **p = environ; *p; ++p) puts(*p);
        return 0;
    }
    return -1;
}
static int wait_foreground(Job *job) {
    int status;
    int wait_error = 0;
    for (;;) {
        if (all_done(job) || all_stopped(job)) break;
        pid_t pid = waitpid(-job->pgid, &status, WUNTRACED | WCONTINUED);
        if (pid < 0) {
            if (errno == EINTR) continue;
            if (errno != ECHILD) perror("waitpid");
            wait_error = 1;
            break;
        }
        record_status(job, pid, status);
    }
    if (interactive) {
        (void)tcsetpgrp(STDIN_FILENO, shell_pgid);
        (void)tcsetattr(STDIN_FILENO, TCSADRAIN, &shell_modes);
    }
    if (all_stopped(job)) {
        printf("[%d] Stopped\n", job->id);
        return 128 + SIGTSTP;
    }
    if (wait_error) return 1;
    int result = job->last_status;
    job->used = 0;
    return result;
}
static void print_jobs(void) {
    runner_poll();
    for (size_t i = 0; i < MAX_JOBS; ++i) if (jobs[i].used)
        printf("[%d] %s %ld\n", jobs[i].id,
               all_stopped(&jobs[i]) ? "Stopped" : "Running", (long)jobs[i].pgid);
}
static int foreground_job(const Stage *stage) {
    runner_poll();
    Job *job = NULL;
    if (stage->argc > 2) { fputs("fg: usage: fg [job-id]\n", stderr); return 2; }
    if (stage->argc == 2) {
        char *end;
        long id = strtol(stage->argv[1], &end, 10);
        if (*end != '\0' || id <= 0) { fputs("fg: invalid job id\n", stderr); return 2; }
        for (size_t i = 0; i < MAX_JOBS; ++i)
            if (jobs[i].used && jobs[i].id == id) job = &jobs[i];
    } else {
        for (size_t i = 0; i < MAX_JOBS; ++i)
            if (jobs[i].used && (!job || jobs[i].id > job->id)) job = &jobs[i];
    }
    if (!job) { fputs("fg: no such job\n", stderr); return 1; }
    if (interactive && tcsetpgrp(STDIN_FILENO, job->pgid) < 0) {
        perror("tcsetpgrp");
        return 1;
    }
    if (kill(-job->pgid, SIGCONT) < 0) {
        perror("SIGCONT");
        if (interactive && tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
            perror("tcsetpgrp: restore shell");
        return 1;
    }
    for (size_t i = 0; i < job->count; ++i)
        if (job->state[i] == 1) job->state[i] = 0;
    return wait_foreground(job);
}
int runner_execute(const Plan *plan, int *exit_requested) {
    *exit_requested = 0;
    if (!plan->count) return 0;
    const Stage *first = &plan->stages[0];
    int is_builtin = !strcmp(first->argv[0], "cd") || !strcmp(first->argv[0], "exit") ||
                     !strcmp(first->argv[0], "jobs") || !strcmp(first->argv[0], "fg") ||
                     !strcmp(first->argv[0], "export") || !strcmp(first->argv[0], "unset");
    if (plan->count == 1 && is_builtin && !plan->background) {
        if (first->input_path || first->output_path) {
            fputs("jiwonmin-shell: redirection of parent builtins is not supported\n", stderr); return 2;
        }
        if (!strcmp(first->argv[0], "jobs")) {
            if (first->argc != 1) { fputs("jobs: no arguments supported\n", stderr); return 2; }
            print_jobs(); return 0;
        }
        if (!strcmp(first->argv[0], "fg")) return foreground_job(first);
        return builtin(first, exit_requested);
    }
    Job *job = allocate_job();
    if (!job) { fputs("jiwonmin-shell: job table full\n", stderr); return 1; }
    int previous_read = -1;
    for (size_t i = 0; i < plan->count; ++i) {
        int link[2] = {-1, -1};
        if (i + 1 < plan->count && pipe(link) < 0) { perror("pipe"); goto launch_error; }
        /* Avoid replaying buffered parent output in a child using stdio. */
        fflush(NULL);
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (link[0] >= 0) close(link[0]);
            if (link[1] >= 0) close(link[1]);
            goto launch_error;
        }
        if (pid == 0) {
            pid_t group = job->pgid ? job->pgid : getpid();
            if (setpgid(0, group) < 0) {
                perror("setpgid: child");
                _exit(1);
            }
            default_signal(SIGINT); default_signal(SIGQUIT); default_signal(SIGTSTP);
            default_signal(SIGTTIN); default_signal(SIGTTOU);
            if (previous_read >= 0 && dup2(previous_read, STDIN_FILENO) < 0) { perror("dup2"); _exit(1); }
            if (link[1] >= 0 && dup2(link[1], STDOUT_FILENO) < 0) { perror("dup2"); _exit(1); }
            if (previous_read >= 0) close(previous_read);
            if (link[0] >= 0) close(link[0]);
            if (link[1] >= 0) close(link[1]);
            const Stage *stage = &plan->stages[i];
            if (apply_redirections(stage) < 0) _exit(1);
            int ignored_exit = 0;
            int code = builtin(stage, &ignored_exit);
            if (code >= 0) _exit(code);
            code = commands_execute((int)stage->argc, stage->argv);
            if (code >= 0) { fflush(stdout); fflush(stderr); _exit(code); }
            execvp(stage->argv[0], stage->argv);
            fprintf(stderr, "jiwonmin-shell: %s: %s\n", stage->argv[0], strerror(errno));
            _exit(errno == ENOENT ? 127 : 126);
        }
        if (!job->pgid) job->pgid = pid;
        /* Both parent and child attempt setpgid to close the fork race.
           EACCES means the child has already executed its program. */
        if (setpgid(pid, job->pgid) < 0 &&
            errno != EACCES && errno != ESRCH) {
            perror("setpgid: parent");
            job->pids[job->count++] = pid;
            goto launch_error;
        }
        job->pids[job->count++] = pid;
        job->last_pid = pid;
        if (previous_read >= 0) close(previous_read);
        if (link[1] >= 0) close(link[1]);
        previous_read = link[0];
    }
    if (previous_read >= 0) close(previous_read);
    if (plan->background) {
        printf("[%d] %ld\n", job->id, (long)job->pgid);
        return 0;
    }
    if (interactive && tcsetpgrp(STDIN_FILENO, job->pgid) < 0) {
        perror("tcsetpgrp");
        goto launch_error;
    }
    return wait_foreground(job);
launch_error:
    if (previous_read >= 0) close(previous_read);
    if (job->pgid > 0) (void)kill(-job->pgid, SIGTERM);
    if (interactive && tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp: restore shell");
    for (size_t i = 0; i < job->count; ++i) {
        int status;
        while (waitpid(job->pids[i], &status, 0) < 0 && errno == EINTR) {}
    }
    job->used = 0;
    return 1;
}
