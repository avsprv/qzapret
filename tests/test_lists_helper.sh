#!/bin/sh
set -eu
H="$1"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/base/ipset"
export QZAPRET_TEST_BASE="$tmp/base"
export QZAPRET_TEST_BACKUP_DIR="$tmp/backups"
printf 'example.org\n192.0.2.0/24\n' > "$tmp/base/ipset/zapret-hosts-user.txt"

out=$("$H" lists-read zapret-hosts-user.txt)
[ "$out" = "$(cat "$tmp/base/ipset/zapret-hosts-user.txt")" ] || {
    echo "FAIL: user list was not read exactly"; exit 1;
}
echo "ok: reads allowlisted user list"

if "$H" lists-read ../config >/dev/null 2>&1; then
    echo "FAIL: path traversal was accepted"; exit 1
fi
if "$H" lists-read config >/dev/null 2>&1; then
    echo "FAIL: non-allowlisted name was accepted"; exit 1
fi
echo "ok: rejects non-allowlisted names"

if "$H" lists-read zapret-hosts.txt >/dev/null 2>&1; then
    echo "FAIL: generated list was accepted"; exit 1
fi
echo "ok: rejects generated lists"

if "$H" lists-read zapret-ip-user.txt >/dev/null 2>&1; then
    echo "FAIL: missing list was accepted"; exit 1
fi
echo "ok: reports missing list"

printf 'updated.example\n2001:db8::/32\n' | "$H" lists-write zapret-hosts-user.txt > "$tmp/write.out"
grep -q 'backup:' "$tmp/write.out"
grep -q 'updated.example' "$tmp/base/ipset/zapret-hosts-user.txt"
grep -q 'example.org' "$tmp/backups/lists/"*/list
echo "ok: creates backup and atomically updates an existing source hostlist"

printf 'new.example\n' | "$H" lists-write zapret-hosts-user-exclude.txt > "$tmp/create.out"
grep -q 'list created' "$tmp/create.out"
grep -q 'new.example' "$tmp/base/ipset/zapret-hosts-user-exclude.txt"
echo "ok: creates a missing source hostlist"

if printf 'invalid domain with spaces\n' | "$H" lists-write zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: malformed hostlist text was accepted"; exit 1
fi
grep -q 'updated.example' "$tmp/base/ipset/zapret-hosts-user.txt"
echo "ok: rejects malformed input without changing current data"

if printf 'generated\n' | "$H" lists-write zapret-ip-user.txt >/dev/null 2>&1; then
    echo "FAIL: derived IP output was writable"; exit 1
fi
echo "ok: rejects writes to derived IP outputs"

if dd if=/dev/zero bs=1048577 count=1 2>/dev/null | "$H" lists-write zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: oversized list was accepted"; exit 1
fi
grep -q 'updated.example' "$tmp/base/ipset/zapret-hosts-user.txt"
echo "ok: rejects lists larger than 1 MiB without replacing current data"

rm "$tmp/base/ipset/zapret-hosts-user.txt"
ln -s /etc/passwd "$tmp/base/ipset/zapret-hosts-user.txt"
if "$H" lists-read zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink list was accepted for reading"; exit 1
fi
if printf 'example.com\n' | "$H" lists-write zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink list was accepted for writing"; exit 1
fi
echo "ok: rejects symlink list for read and write"

rm -rf "$tmp/base/ipset"
ln -s "$tmp" "$tmp/base/ipset"
if "$H" lists-read zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink ipset directory was accepted"; exit 1
fi
if printf 'example.com\n' | "$H" lists-write zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink ipset directory was accepted for writing"; exit 1
fi
echo "ok: rejects symlink ipset directory"
