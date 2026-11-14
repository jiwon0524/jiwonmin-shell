# Development notes

## Stage 1 boundaries
- Parse into an explicit Plan with multiple Stage records.
- Reject malformed pipelines, duplicate redirects, misplaced background markers.
- Print parsed plan; do not execute anything.

## Remaining requirements
- fork/exec/wait and builtin cd/exit
- redirection and pipeline wiring
- signal and foreground/background job handling
- implementation of 11 requested commands
- integration tests and real-terminal signal tests

## References to verify during development
- Linux man-pages: fork(2), execve(2), pipe(2), dup2(2), waitpid(2), sigaction(2)
- GNU Bash Reference Manual: pipelines, redirections, job control
