#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/source/nested" "$tmp/destination"
printf 'hello\n' > "$tmp/source/nested/report.txt"
printf 'jiwonmin %s --find report.txt --move %s --yes\nexit\n' "$tmp/source" "$tmp/destination" | ./jiwonmin-shell > "$tmp/out"
test -f "$tmp/destination/report.txt"
test ! -e "$tmp/source/nested/report.txt"
grep -q 'moved=1' "$tmp/out"
printf 'again\n' > "$tmp/source/report.txt"
printf 'jiwonmin %s --find report.txt --move %s --yes\nexit\n' "$tmp/source" "$tmp/destination" | ./jiwonmin-shell > "$tmp/out" 2> "$tmp/err" || true
test -f "$tmp/source/report.txt"
grep -q 'hello' "$tmp/destination/report.txt"
grep -q 'FAILED' "$tmp/err"
printf 'jiwonmin %s --find report.txt --move %s --yes\nexit\n' "$tmp/source" "$tmp/source/nested" | ./jiwonmin-shell > "$tmp/out" 2> "$tmp/err" || true
test -f "$tmp/source/report.txt"
grep -q 'destination cannot be inside' "$tmp/err"
printf '3 Jiwonmin Crab checks passed\n'
