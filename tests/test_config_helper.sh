#!/bin/sh
set -eu
H="$1"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/base"
export QZAPRET_TEST_BASE="$tmp/base"
export QZAPRET_TEST_BACKUP_DIR="$tmp/backups"
printf 'NFQWS2_ENABLE=1\n# original\n' > "$tmp/base/config"
out=$("$H" config-read)
[ "$out" = "$(cat "$tmp/base/config")" ] || { echo "FAIL: config-read changed content"; exit 1; }
echo "ok: reads regular config"
printf 'NFQWS2_ENABLE=0\n# changed\n' | "$H" config-write > "$tmp/write.out"
grep -q 'backup:' "$tmp/write.out"
grep -q 'NFQWS2_ENABLE=0' "$tmp/base/config"
echo "ok: validates, backs up and writes config"
if printf 'if then\n' | "$H" config-write >/dev/null 2>&1; then
    echo "FAIL: invalid shell syntax was accepted"; exit 1
fi
grep -q 'NFQWS2_ENABLE=0' "$tmp/base/config"
echo "ok: rejects invalid syntax without changing config"
if dd if=/dev/zero bs=1048577 count=1 2>/dev/null | "$H" config-write >/dev/null 2>&1; then
    echo "FAIL: oversized config was accepted"; exit 1
fi
grep -q 'NFQWS2_ENABLE=0' "$tmp/base/config"
echo "ok: rejects config larger than 1 MiB"
printf 'NFQWS2_ENABLE=1\n# newer\n' | "$H" config-write >/dev/null
"$H" config-rollback >/dev/null
grep -q 'NFQWS2_ENABLE=0' "$tmp/base/config"
echo "ok: restores latest backup and preserves pre-rollback config"
rm "$tmp/base/config"
ln -s /etc/passwd "$tmp/base/config"
if "$H" config-read >/dev/null 2>&1; then
    echo "FAIL: symlink config was accepted"; exit 1
fi
if printf 'BAD=1\n' | "$H" config-write >/dev/null 2>&1; then
    echo "FAIL: symlink config was writable"; exit 1
fi
echo "ok: rejects symlink config"
