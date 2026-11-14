#!/bin/sh
set -eu
out=$(printf 'help\nexit\n' | ./jiwonmin-shell)
printf '%s\n' "$out" | grep -q '명령어 도움말'
printf '%s\n' "$out" | grep -q 'jiwonmin'
printf '%s\n' "$out" | grep -q '파일 및 디렉터리'
echo '3 Korean help checks passed'
