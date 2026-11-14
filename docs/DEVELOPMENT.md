# Development notes

- Stage 1: tokenization and execution plan parsing.
- Stage 2: add a single-command runner with `fork` / `execvp` / `waitpid` and parent-process `cd` / `exit`.
- Unsupported execution modes explicitly fail rather than silently ignoring operators.
- Next: implement redirection and pipelines, then job control and the required built-in commands.
- Tests: `make test` includes parser and runner checks.

## Stage 3
Added pipe-connected child processes and per-stage input/output redirection; tested with shell integration tests. Background and signal/job control remain unimplemented.

## Stage 4
Added process groups, terminal foreground handoff, stopped/running/finished job tracking, background execution, jobs/fg, and signal defaults for child processes. Tested in noninteractive automation; Ctrl+C/Ctrl+Z must be checked manually in an interactive terminal.

## Stage 5
Implemented the ten non-cd assignment commands in `src/commands.c`, using POSIX file/directory operations; retained parent-process `cd` in runner. Added end-to-end tests for file manipulation, piping, grep, and self-copy protection. Supported syntax is deliberately narrower than GNU utilities.


## Stage 6 — job-control hardening
- Check process-group assignment failures in both parent and child.
- Restore terminal ownership when foreground handoff fails.
- Handle `fg` resume errors instead of silently continuing.
- On shell exit, notify tracked background/stopped jobs with SIGHUP (and SIGCONT for stopped jobs).
- Add regression tests for multi-stage pipes, redirection failure recovery, and invalid `fg` identifiers.

Limitations: interactive terminal signal handling still needs PTY-based tests; no claim of full POSIX job control compliance.
