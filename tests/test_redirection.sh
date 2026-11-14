#!/bin/sh
set -eu
shell=./jiwonmin-shell
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
# File appending and stderr routing.
printf 'echo first > %s/out; echo second >> %s/out; cat %s/out\nexit\n' "$tmp" "$tmp" "$tmp" | "$shell" > "$tmp/log" 2>&1
grep -qx first "$tmp/out" || exit 1
grep -qx second "$(tail -n 1 "$tmp/out" | sed 's/second/second/')" 2>/dev/null || grep -q second "$tmp/out"
echo 'append output check passed'
printf 'ls %s/missing 2> %s/err\nexit\n' "$tmp" "$tmp" | "$shell" > "$tmp/log" 2>&1
test -s "$tmp/err"
echo 'stderr redirect check passed'
printf 'ls %s/missing 2>> %s/err\nexit\n' "$tmp" "$tmp" | "$shell" > "$tmp/log" 2>&1
test "$(wc -c < "$tmp/err")" -gt 10
echo 'stderr append check passed'
printf 'ls %s/missing > %s/all 2>&1\nexit\n' "$tmp" "$tmp" | "$shell" > "$tmp/log" 2>&1
test -s "$tmp/all"
echo 'stderr dup check passed'
printf 'ls %s/missing 2>&1 > %s/out2\nexit\n' "$tmp" "$tmp" | "$shell" > "$tmp/log" 2>&1
test ! -s "$tmp/out2"
grep -q missing "$tmp/log"
echo 'redirection order check passed'
printf 'echo hello | cat >> %s/pipe\nexit\n' "$tmp" | "$shell" > "$tmp/log" 2>&1
grep -q hello "$tmp/pipe"
echo 'pipeline redirection check passed'
