#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
printf 'alpha\nbeta\nalpha beta\n' > "$tmp/input"
printf 'cat < %s | grep alpha > %s\nexit\n' "$tmp/input" "$tmp/output" | ./jiwonmin-shell >/dev/null
printf 'alpha\nalpha beta\n' > "$tmp/expected"
cmp "$tmp/expected" "$tmp/output"
printf 'echo first > %s\necho second >> %s\nexit\n' "$tmp/append" "$tmp/append" | ./jiwonmin-shell >/dev/null
printf 'first\nsecond\n' > "$tmp/append_expected"
cmp "$tmp/append_expected" "$tmp/append"
printf 'echo hello | cat | cat\nexit\n' | ./jiwonmin-shell > "$tmp/piped"
grep -qx hello "$tmp/piped"
printf 'cat < %s\nexit\n' "$tmp/missing" | ./jiwonmin-shell >"$tmp/missing_out" 2>"$tmp/missing_err"
grep -q "$tmp/missing" "$tmp/missing_err"
printf 'echo nope &\nexit\n' | ./jiwonmin-shell >"$tmp/bg_out" 2>"$tmp/bg_err"
grep -q '^\[.*\]' "$tmp/bg_out"
printf '5 stage-3 checks passed\n'
