#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out=$(./jiwonmin-shell <<'INPUT'
export JWS_TEST=hello
echo $JWS_TEST
/bin/sh -c 'test "$JWS_TEST" = hello && echo inherited'
unset JWS_TEST
echo "${JWS_TEST}"
exit
INPUT
)
printf '%s\n' "$out" | grep -qx hello
printf '%s\n' "$out" | grep -qx inherited
out=$(./jiwonmin-shell <<'INPUT'
false
echo $?
true && echo $?
exit
INPUT
)
printf '%s\n' "$out" | grep -qx 1
printf '%s\n' "$out" | grep -qx 0
out=$(./jiwonmin-shell <<'INPUT'
cd /tmp && echo $PWD
exit
INPUT
)
printf '%s\n' "$out" | grep -qx /tmp
out=$(./jiwonmin-shell <<'INPUT'
export JWS_TEST=hello
echo '$JWS_TEST'
exit
INPUT
)
printf '%s\n' "$out" | grep -qx '\$JWS_TEST'
echo '4 environment integration checks passed'
