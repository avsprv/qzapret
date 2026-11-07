#!/bin/sh
set -eu
H="$1"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/base/init.d/openrc" "$tmp/base/init.d/openwrt" "$tmp/etc/init.d" "$tmp/bin"
export QZAPRET_TEST_BASE="$tmp/base"
export QZAPRET_TEST_SERVICE_DIR="$tmp/etc/init.d"
export QZAPRET_TEST_SERVICE_ARGS="$tmp/service.args"
export QZAPRET_TEST_RC_SERVICE_ARGS="$tmp/rc-service.args"
export QZAPRET_TEST_RC_UPDATE_ARGS="$tmp/rc-update.args"
PATH="$tmp/bin:$PATH"
export PATH

cat > "$tmp/bin/rc-service" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" > "$QZAPRET_TEST_RC_SERVICE_ARGS"
[ "$1" = zapret2 ] || exit 9
case "$2" in
    status) [ "${QZAPRET_TEST_RUNNING:-1}" = 1 ] ;;
    start|stop) echo "rc-service $2"; exit 0 ;;
    *) exit 8 ;;
esac
EOF
cat > "$tmp/bin/rc-update" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" > "$QZAPRET_TEST_RC_UPDATE_ARGS"
case "$1" in
    show) [ "${QZAPRET_TEST_ENABLED:-1}" = 1 ] && echo 'zapret2 | default'; exit 0 ;;
    add|del) [ "$2" = zapret2 ] && [ "$3" = default ] && exit 0 ;;
esac
exit 8
EOF
chmod 755 "$tmp/bin/rc-service" "$tmp/bin/rc-update"

cat > "$tmp/base/init.d/openrc/zapret2" <<'EOF'
#!/bin/sh
# fake upstream OpenRC adapter for helper tests
exit 0
EOF
chmod 755 "$tmp/base/init.d/openrc/zapret2"
cp "$tmp/base/init.d/openrc/zapret2" "$tmp/etc/init.d/zapret2"
chmod 755 "$tmp/etc/init.d/zapret2"

[ "$("$H" service openrc status)" = active ]
[ "$("$H" service openrc enabled)" = enabled ]
state=$("$H" service openrc state)
[ "$(printf '%s\n' "$state" | sed -n '1p')" = active ]
[ "$(printf '%s\n' "$state" | sed -n '2p')" = enabled ]
grep -qx zapret2 "$tmp/rc-service.args"
grep -qx status "$tmp/rc-service.args"
grep -qx show "$tmp/rc-update.args"
grep -qx default "$tmp/rc-update.args"
echo "ok: OpenRC status and runlevel are queried through fixed service identifiers"

QZAPRET_TEST_RUNNING=0 "$H" service openrc status | grep -qx inactive
QZAPRET_TEST_ENABLED=0 "$H" service openrc enabled | grep -qx disabled
"$H" service openrc start >/dev/null
grep -qx zapret2 "$tmp/rc-service.args"
grep -qx start "$tmp/rc-service.args"
"$H" service openrc stop >/dev/null
"$H" service openrc enable >/dev/null
"$H" service openrc disable >/dev/null
echo "ok: OpenRC lifecycle and boot enablement actions are allowlisted"

cat > "$tmp/base/init.d/openwrt/zapret2" <<'EOF'
#!/bin/sh
# fake upstream OpenWrt procd adapter for helper tests
printf '%s\n' "$@" > "$QZAPRET_TEST_SERVICE_ARGS"
case "$1" in
    running) [ "${QZAPRET_TEST_RUNNING:-1}" = 1 ] ;;
    enabled) [ "${QZAPRET_TEST_ENABLED:-1}" = 1 ] ;;
    start|stop|enable|disable) echo "openwrt $1"; exit 0 ;;
    *) exit 8 ;;
esac
EOF
chmod 755 "$tmp/base/init.d/openwrt/zapret2"
cp "$tmp/base/init.d/openwrt/zapret2" "$tmp/etc/init.d/zapret2"
chmod 755 "$tmp/etc/init.d/zapret2"

[ "$("$H" service openwrt status)" = active ]
[ "$("$H" service openwrt enabled)" = enabled ]
state=$("$H" service openwrt state)
[ "$(printf '%s\n' "$state" | sed -n '1p')" = active ]
[ "$(printf '%s\n' "$state" | sed -n '2p')" = enabled ]
QZAPRET_TEST_RUNNING=0 "$H" service openwrt status | grep -qx inactive
QZAPRET_TEST_ENABLED=0 "$H" service openwrt enabled | grep -qx disabled
"$H" service openwrt start >/dev/null
grep -qx start "$tmp/service.args"
"$H" service openwrt stop >/dev/null
"$H" service openwrt enable >/dev/null
"$H" service openwrt disable >/dev/null
echo "ok: OpenWrt procd lifecycle and boot enablement actions are allowlisted"

if "$H" service systemd start >/dev/null 2>&1; then
    echo "FAIL: unsupported backend was accepted"; exit 1
fi
if "$H" service openwrt reboot >/dev/null 2>&1; then
    echo "FAIL: unsupported service action was accepted"; exit 1
fi
if "$H" service openwrt start extra >/dev/null 2>&1; then
    echo "FAIL: extra service arguments were accepted"; exit 1
fi
echo "ok: rejects unsupported backends, actions, and extra arguments"

printf '# tampered init script\n' > "$tmp/etc/init.d/zapret2"
if "$H" service openwrt start >/dev/null 2>&1; then
    echo "FAIL: modified init script was accepted"; exit 1
fi
echo "ok: rejects init scripts that differ from bundled upstream adapters"
