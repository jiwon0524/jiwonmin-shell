#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
printf 'jiwonmin -z\nexit\n' | ./jiwonmin-shell > "$tmp/out" 2> "$tmp/err" || true
grep -F 'usage: jiwonmin' "$tmp/err" >/dev/null
printf 'jiwonmin -d 99\nexit\n' | ./jiwonmin-shell > "$tmp/out" 2> "$tmp/err" || true
grep -F 'delay must be' "$tmp/err" >/dev/null
printf 'jiwonmin\nexit\n' | env -u DISPLAY ./jiwonmin-shell > "$tmp/out" 2> "$tmp/err" || true
grep -F 'DISPLAY is not set' "$tmp/err" >/dev/null
printf '3 screenshot validation checks passed\n'
