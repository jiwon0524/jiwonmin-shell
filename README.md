# DEU Shell — stage 2

An educational C11 shell prototype. This is an intermediate implementation, not a complete shell.

## Build and run

```sh
make
make test
./deu-shell
```

## Supported

- Tokenization and parsing (from stage 1)
- One foreground external command with arguments via `fork`, `execvp`, and `waitpid`
- Built-in `cd` (current process) and `exit`
- Errors for unknown commands and unsupported stage-2 execution modes

## Not yet supported

- Executing pipelines, redirects, or background jobs (parsed, but execution explicitly rejected)
- Signal/job control
- Direct implementations of the assignment's eleven required commands (currently other commands run via the OS)

## Development provenance

Stage 2 is AI-assisted scaffolding. Review and adapt the code yourself and follow your course's AI-use rules before submission. No third-party shell source code was copied into this stage.
