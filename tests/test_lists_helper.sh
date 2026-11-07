#!/bin/sh
set -eu

H="$1"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

mkdir -p "$tmp/base/ipset"
export QZAPRET_TEST_BASE="$tmp/base"
printf '# comment\nexample.org\n192.0.2.0/24\n' > "$tmp/base/ipset/zapret-hosts-user.txt"

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
    echo "FAIL: missing file was accepted"; exit 1
fi
echo "ok: reports missing list"

rm "$tmp/base/ipset/zapret-hosts-user.txt"
ln -s /etc/passwd "$tmp/base/ipset/zapret-hosts-user.txt"
if "$H" lists-read zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink list was accepted"; exit 1
fi
echo "ok: rejects symlink list"

rm "$tmp/base/ipset/zapret-hosts-user.txt"
dd if=/dev/zero of="$tmp/base/ipset/zapret-hosts-user.txt" bs=1048577 count=1 2>/dev/null
if "$H" lists-read zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: oversized list was accepted"; exit 1
fi
echo "ok: rejects lists larger than 1 MiB"

rm -rf "$tmp/base/ipset"
ln -s "$tmp" "$tmp/base/ipset"
if "$H" lists-read zapret-hosts-user.txt >/dev/null 2>&1; then
    echo "FAIL: symlink ipset directory was accepted"; exit 1
fi
echo "ok: rejects symlink ipset directory"
