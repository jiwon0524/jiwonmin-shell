#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
output=$(printf 'sleep 0.1 &\njobs\nsleep 0.3\njobs\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$output" | grep -q 'Running'
printf '%s\n' "$output" | grep -q 'Done'
output=$(printf 'sleep 0.1 &\nfg\necho finished\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$output" | grep -q 'finished'
printf '2 job-control checks passed\n'
