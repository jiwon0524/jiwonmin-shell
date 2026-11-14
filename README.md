# jiwonmin Shell — Stage 5

Educational C11 shell implementing eleven assignment commands: `ls`, `pwd`, `cd`, `mkdir`, `rmdir`, `ln`, `cp`, `rm`, `mv`, `cat`, and `grep`.

Ten commands are implemented in `src/commands.c`; `cd` is implemented in `src/runner.c` because it must change the shell process's working directory. Internal commands can participate in pipelines and redirection. Unknown commands fall back to `execvp()`.

## Build and test

```sh
make clean && make && make test
./deu-shell
```

## Supported command subset

- `ls [directory]`: lists visible names, one per line; no flags.
- `pwd`, `cd [directory]`, `mkdir directory`, `rmdir empty-directory`.
- `ln source link-name`, `ln -s source link-name`.
- `cp source destination`: regular files only, no directory copies.
- `rm file`, `mv source destination`: simple file operations, no options.
- `cat [file ...]` and `grep pattern [file ...]`: basic text operations; `grep` matches literal substrings, not regular expressions.
- `jobs`, `fg`, `exit`, background `&`, redirections `<`, `>`, `>>`, and pipelines `|` remain available.

This is not a full replacement for GNU coreutils or POSIX shell utilities. In particular, command flags and recursive operations are not implemented. Job control is basic and needs interactive terminal regression testing.

## Team work

Assign command groups and document *actual* implementation, review, and test contributions. Do not attribute generated code to a teammate who did not write or review it.
