#define _POSIX_C_SOURCE 200809L
#include "runner.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int wait_one(pid_t pid) {
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 1;
}

static int redirect_fd(const char *path, int target, int flags, mode_t mode) {
    int fd = open(path, flags, mode);
    if (fd < 0) { perror(path); return -1; }
    if (dup2(fd, target) < 0) {
        perror("dup2");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int apply_redirections(const Stage *stage) {
    if (stage->input_path &&
        redirect_fd(stage->input_path, STDIN_FILENO, O_RDONLY, 0) < 0) return -1;
    if (stage->output_path) {
        int flags = O_WRONLY | O_CREAT | (stage->append_output ? O_APPEND : O_TRUNC);
        if (redirect_fd(stage->output_path, STDOUT_FILENO, flags, 0666) < 0) return -1;
    }
    return 0;
}

static int run_builtin(const Stage *stage, int *exit_requested) {
    const char *cmd = stage->argv[0];
    if (strcmp(cmd, "exit") == 0) {
        if (stage->argc != 1) { fputs("exit: no arguments supported\n", stderr); return 2; }
        *exit_requested = 1;
        return 0;
    }
    if (strcmp(cmd, "cd") == 0) {
        if (stage->argc > 2) { fputs("cd: too many arguments\n", stderr); return 2; }
        const char *target = stage->argc == 2 ? stage->argv[1] : getenv("HOME");
        if (!target) { fputs("cd: HOME is not set\n", stderr); return 1; }
        if (chdir(target) != 0) { perror("cd"); return 1; }
        return 0;
    }
    return -1;
}

int runner_execute(const Plan *plan, int *exit_requested) {
    *exit_requested = 0;
    if (!plan->count) return 0;
    if (plan->background) {
        fputs("deu-shell: background jobs are not supported in stage 3\n", stderr);
        return 2;
    }
    if (plan->count == 1 &&
        (!strcmp(plan->stages[0].argv[0], "cd") ||
         !strcmp(plan->stages[0].argv[0], "exit"))) {
        if (plan->stages[0].input_path || plan->stages[0].output_path) {
            fputs("deu-shell: redirection of parent builtins is not supported yet\n", stderr);
            return 2;
        }
        return run_builtin(&plan->stages[0], exit_requested);
    }
    pid_t children[PLAN_MAX_STAGES];
    size_t launched = 0;
    int previous_read = -1;
    for (size_t i = 0; i < plan->count; ++i) {
        int link[2] = {-1, -1};
        if (i + 1 < plan->count && pipe(link) < 0) {
            perror("pipe");
            goto launch_error;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (link[0] >= 0) close(link[0]);
            if (link[1] >= 0) close(link[1]);
            goto launch_error;
        }
        if (pid == 0) {
            if (previous_read >= 0 && dup2(previous_read, STDIN_FILENO) < 0) {
                perror("dup2"); _exit(1);
            }
            if (link[1] >= 0 && dup2(link[1], STDOUT_FILENO) < 0) {
                perror("dup2"); _exit(1);
            }
            if (previous_read >= 0) close(previous_read);
            if (link[0] >= 0) close(link[0]);
            if (link[1] >= 0) close(link[1]);
            const Stage *stage = &plan->stages[i];
            if (apply_redirections(stage) < 0) _exit(1);
            int ignored_exit = 0;
            if (!strcmp(stage->argv[0], "cd") || !strcmp(stage->argv[0], "exit")) {
                int code = run_builtin(stage, &ignored_exit);
                _exit(code < 0 ? 1 : code);
            }
            execvp(stage->argv[0], stage->argv);
            fprintf(stderr, "deu-shell: %s: %s\n", stage->argv[0], strerror(errno));
            _exit(errno == ENOENT ? 127 : 126);
        }
        children[launched++] = pid;
        if (previous_read >= 0) close(previous_read);
        if (link[1] >= 0) close(link[1]);
        previous_read = link[0];
    }
    if (previous_read >= 0) close(previous_read);
    int result = 0;
    for (size_t i = 0; i < launched; ++i) {
        int code = wait_one(children[i]);
        if (i == launched - 1) result = code;
    }
    return result;

launch_error:
    if (previous_read >= 0) close(previous_read);
    for (size_t i = 0; i < launched; ++i) (void)wait_one(children[i]);
    return 1;
}
