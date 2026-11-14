#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
printf 'alpha\nbeta\nalpha two\n' > "$tmp/source"
cat > "$tmp/commands" <<EOF
mkdir $tmp/folder
pwd
ls $tmp
cp $tmp/source $tmp/copy
cat $tmp/copy | grep alpha > $tmp/matches
ln $tmp/copy $tmp/hard
ln -s $tmp/copy $tmp/soft
mv $tmp/hard $tmp/moved
rm $tmp/moved
rm $tmp/soft
rmdir $tmp/folder
exit
EOF
./deu-shell < "$tmp/commands" > "$tmp/out"
test -f "$tmp/copy"
test ! -e "$tmp/folder"
test ! -e "$tmp/moved"
test ! -L "$tmp/soft"
printf 'alpha\nalpha two\n' > "$tmp/expected"
cmp "$tmp/expected" "$tmp/matches"
grep -Fx "$tmp" "$tmp/out" >/dev/null || grep -Fx "$(pwd)" "$tmp/out" >/dev/null
grep -Fx 'source' "$tmp/out" >/dev/null
# The internal grep returns status 1 when no match; it must not print.
printf 'grep xyz-not-found %s\nexit\n' "$tmp/source" | ./deu-shell > "$tmp/no-match"
test "$(wc -l < "$tmp/no-match")" -eq 1
# Copying a file over itself must be rejected without damaging it.
printf 'cp %s %s\nexit\n' "$tmp/source" "$tmp/source" | ./deu-shell >/dev/null 2>"$tmp/error"
grep -q 'same file' "$tmp/error"
printf 'alpha\nbeta\nalpha two\n' > "$tmp/unchanged"
cmp "$tmp/unchanged" "$tmp/source"
printf '4 internal-command integration checks passed\n'
