#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

# A pipeline must propagate data through three processes.
printf 'echo alpha | cat | grep alpha > %s\nexit\n' "$tmp/result" |
    ./jiwonmin-shell > "$tmp/stdout"
printf 'alpha\n' > "$tmp/expected"
cmp "$tmp/expected" "$tmp/result"

# A nonexistent input file must be reported without terminating the shell.
printf 'cat < %s\necho alive\nexit\n' "$tmp/missing" |
    ./jiwonmin-shell > "$tmp/stdout" 2> "$tmp/stderr"
grep -Fx alive "$tmp/stdout" >/dev/null
grep -F "$tmp/missing" "$tmp/stderr" >/dev/null

# An invalid job identifier should fail cleanly.
printf 'fg 99999\necho alive\nexit\n' |
    ./jiwonmin-shell > "$tmp/stdout" 2> "$tmp/stderr"
grep -Fx alive "$tmp/stdout" >/dev/null
grep -F 'no such job' "$tmp/stderr" >/dev/null

printf '3 regression checks passed\n'

# Copying a file onto itself must not truncate the source.
printf 'keep-me\n' > "$tmp/source"
printf 'cp %s %s\nexit\n' "$tmp/source" "$tmp/source" |
    ./jiwonmin-shell > "$tmp/stdout" 2> "$tmp/stderr" || true
printf 'keep-me\n' > "$tmp/expected"
cmp "$tmp/expected" "$tmp/source"
grep -F 'same file' "$tmp/stderr" >/dev/null

# A failed cat operand must not prevent a later valid file from being read.
printf 'cat %s %s\nexit\n' "$tmp/missing" "$tmp/source" |
    ./jiwonmin-shell > "$tmp/stdout" 2> "$tmp/stderr" || true
grep -Fx 'keep-me' "$tmp/stdout" >/dev/null

printf '2 additional regression checks passed\n'
