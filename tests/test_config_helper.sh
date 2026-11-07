#!/bin/sh
set -eu
H="${1:?usage: test_config_helper.sh /path/to/helper}"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/base"
printf 'NFQWS2_ENABLE=1\n# user config\n' > "$tmp/base/config"

out=$(QZAPRET_TEST_BASE="$tmp/base" "$H" config-read)
[ "$out" = "$(cat "$tmp/base/config")" ] || { echo "FAIL: config-read changed content"; exit 1; }
echo "ok: reads regular config"

rm "$tmp/base/config"
if QZAPRET_TEST_BASE="$tmp/base" "$H" config-read >/dev/null 2>&1; then
    echo "FAIL: missing config was accepted"; exit 1
fi
echo "ok: rejects missing config"

ln -s /etc/passwd "$tmp/base/config"
if QZAPRET_TEST_BASE="$tmp/base" "$H" config-read >/dev/null 2>&1; then
    echo "FAIL: symlink config was accepted"; exit 1
fi
echo "ok: rejects symlink config"

rm "$tmp/base/config"
dd if=/dev/zero of="$tmp/base/config" bs=1048577 count=1 2>/dev/null
if QZAPRET_TEST_BASE="$tmp/base" "$H" config-read >/dev/null 2>&1; then
    echo "FAIL: oversized config was accepted"; exit 1
fi
echo "ok: rejects config larger than 1 MiB"
