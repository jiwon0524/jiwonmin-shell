# Development notes

- Stage 1: tokenization and execution plan parsing.
- Stage 2: add a single-command runner with `fork` / `execvp` / `waitpid` and parent-process `cd` / `exit`.
- Unsupported execution modes explicitly fail rather than silently ignoring operators.
- Next: implement redirection and pipelines, then job control and the required built-in commands.
- Tests: `make test` includes parser and runner checks.

## Stage 3
Added pipe-connected child processes and per-stage input/output redirection; tested with shell integration tests. Background and signal/job control remain unimplemented.
