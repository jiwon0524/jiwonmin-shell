#!/bin/sh
set -eu
out=$(printf 'pwd\n!!\nhistory\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$out" | grep -q '  pwd'
printf '%s\n' "$out" | grep -q '  history'
out=$(printf 'pwd\nhistory -c\nhistory\nexit\n' | ./jiwonmin-shell)
if printf '%s\n' "$out" | grep -q '  pwd'; then echo 'history clear failed' >&2; exit 1; fi
out=$(printf 'pwd\n!1\nexit\n' | ./jiwonmin-shell)
[ "$(printf '%s\n' "$out" | grep -c '^pwd$')" -eq 1 ]
out=$(printf 'history 0\nexit\n' | ./jiwonmin-shell)
if printf '%s\n' "$out" | grep -q '  history'; then echo 'history 0 failed' >&2; exit 1; fi
echo '4 history checks passed'
