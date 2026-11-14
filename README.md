# Jiwonmin Shell — Stage 8 (X11 Screenshot)

Educational C11 shell implementing eleven assignment commands: `ls`, `pwd`, `cd`, `mkdir`, `rmdir`, `ln`, `cp`, `rm`, `mv`, `cat`, and `grep`.

Ten commands are implemented in `src/commands.c`; `cd` is implemented in `src/runner.c` because it must change the shell process's working directory. Internal commands can participate in pipelines and redirection. Unknown commands fall back to `execvp()`.

## Build and test

```sh
make clean && make && make test
./jiwonmin-shell
```

## Supported command subset

- `ls [directory]`: lists visible names, one per line; no flags.
- `pwd`, `cd [directory]`, `mkdir directory`, `rmdir empty-directory`.
- `ln source link-name`, `ln -s source link-name`.
- `cp source destination`: regular files only, no directory copies.
- `rm file`, `mv source destination`: simple file operations, no options.
- `cat [file ...]` and `grep pattern [file ...]`: basic text operations; `grep` matches literal substrings, not regular expressions.
- `jobs`, `fg`, `exit`, background `&`, redirections `<`, `>`, `>>`, and pipelines `|` remain available.

This is not a full replacement for GNU coreutils or POSIX shell utilities. In particular, command flags and recursive operations are not implemented. Job control is basic. Manual interactive-terminal regression tests for Ctrl+C, Ctrl+Z, Ctrl+\\, jobs and fg are still required. Shell exit sends SIGHUP to tracked jobs and SIGCONT to stopped jobs.

## Team work

Assign command groups and document *actual* implementation, review, and test contributions. Do not attribute generated code to a teammate who did not write or review it.

## Stage 7 code organization
The internal command dispatcher is in `src/commands.c`. File and directory operations are in `src/commands_fs.c`; text commands are in `src/commands_text.c`. Shared declarations are in `src/commands_internal.h`. These are organizational changes, not full GNU utility implementations.

## Jiwonmin Screenshot (X11 / GNOME)

`jiwonmin` captures the full desktop into `~/Pictures/Jiwonmin/screenshot_YYYYMMDD_HHMMSS_NNN.png`.

- `jiwonmin -d 3`: capture after a 3-second delay.
- `jiwonmin -o /tmp/capture.png`: save to a chosen path (will not overwrite existing files).
- Requires an X11 graphical session and either `scrot` or `gnome-screenshot` installed. Install via `sudo apt install scrot` if necessary.
- Captures the **entire desktop**, including the shell window. To hide the shell before capture, use `-d 3` and minimize it during the countdown.
- This is a full-screen capture feature, not yet region/window selection.
- Screen capture is user-triggered; no background surveillance or automatic uploads.

The crab file hunter was not included in this release; this release is based on the tested Stage 7 source.
