# DEU Shell — Stage 3

C11 educational shell. Supports single commands, `cd`, `exit`, input/output redirection (`<`, `>`, `>>`), and multi-stage pipelines (`|`). Background jobs (`&`) and interactive job control are not yet implemented. The assigned 11 commands currently run as external executables; they are **not** implemented internally.

## Build and test

```sh
make clean && make && make test
./deu-shell
```

Run commands inside the shell **without** typing the `deu-shell>` prompt.

## Limitations

- Builtin `cd` and `exit` with redirection are rejected in stage 3.
- `cd` inside a pipeline changes only the child process's directory.
- The parser is an educational subset of POSIX shell grammar.
- Signal/job control and command-specific builtins are future stages.
