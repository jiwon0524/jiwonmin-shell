# DEU Shell — Stage 1

This milestone is **a parser demonstration, not a working shell**. It does not execute commands, spawn processes, change directories, or implement job control.

```sh
make
make test
./deu-shell
```

Try `cat input.txt | grep "error code" > result.txt &` to inspect the plan. Type `exit` to quit.

Tokenizer supports whitespace, single/double quotes, backslash escapes, `|`, `<`, `>`, `>>`, `&`. It does not support shell expansions, `&&`, `||`, `;`, heredocs, or full POSIX shell grammar.

Next milestone: command dispatch and execution, with regression tests.

**Academic integrity:** This is an AI-assisted reference implementation. Check the course AI policy before submitting, understand and modify it substantively, and accurately record contributions. It is not guaranteed to be dissimilar to other projects.
