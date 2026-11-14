#!/bin/sh
set -eu
out=$(printf 'false && echo WRONG; echo DONE\ntrue || echo WRONG2\nfalse || echo RECOVER\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$out" | grep -qx DONE
printf '%s\n' "$out" | grep -qx RECOVER
if printf '%s\n' "$out" | grep -q WRONG; then exit 1; fi
out=$(printf 'cd /tmp && pwd\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$out" | grep -qx /tmp
printf '2 chaining integration checks passed\n'
