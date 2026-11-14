#!/bin/sh
set -eu
actual=$(printf 'pwd\ncd /\npwd\nexit\n' | ./deu-shell)
printf '%s\n' "$actual" | grep -Fx '/' >/dev/null
actual=$(printf 'echo hello\nexit\n' | ./deu-shell)
printf '%s\n' "$actual" | grep -Fx 'hello' >/dev/null
actual=$(printf 'pwd | cat\nexit\n' | ./deu-shell 2>&1)
printf '%s\n' "$actual" | grep -Fx "$(pwd)" >/dev/null
printf '3 runner checks passed\n'
