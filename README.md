# DEU Shell — Stage 4

Educational C11 shell. Supports external foreground commands, `cd`, `exit`, `<`, `>`, `>>`, multi-stage pipelines, background jobs (`&`), and basic `jobs` / `fg` job control. In an interactive terminal, foreground process groups receive Ctrl+C (SIGINT), Ctrl+Z (SIGTSTP), and Ctrl+\ (SIGQUIT). The shell itself ignores those signals.

## Build

```sh
make clean && make && make test
./deu-shell
```

## Manual interactive checks

Run one line at a time *inside* the shell:

```text
sleep 10 &
jobs
fg
```

While `fg` is waiting, press Ctrl+C or Ctrl+Z. After Ctrl+Z, run `jobs` and `fg` to resume. `Ctrl+Z` is SIGTSTP, **not** SIGQUIT.

## Current limitations

- Parent builtins do not support redirection; `jobs` and `fg` are unavailable in pipelines.
- Background status is reaped when the shell next processes input, rather than asynchronously notifying while waiting at the prompt.
- Job IDs can be reused only via new allocations, not by a persistent jobspec grammar; `fg [numeric-id]` is supported.
- No terminal mode preservation per stopped job; interactive full-screen programs may require additional work.
- `exit` does not terminate outstanding background jobs.
- The assignment's 11 commands still run via `execvp` and have **not** been independently implemented.
