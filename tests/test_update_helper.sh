#!/bin/sh
# Офлайн-тест команды `update` хелпера: фейковый релиз по file://, без сети и без root.
# Переопределения QZAPRET_TEST_* работают, только если хелпер лежит вне системных каталогов.
H="${1:?usage: test_update_helper.sh /path/to/zapret2applet-helper}"
fail=0
W=$(mktemp -d "${TMPDIR:-/tmp}/qz-upd.XXXXXX") || exit 1
trap 'rm -rf "$W"' EXIT

ok()  { echo "ok: $*"; }
bad() { echo "FAIL: $*"; fail=1; }

# make_release <tag> [bad_sums] [unsafe] [symlink] [broken_bin]
make_release() {
    tag=$1; name=zapret2-$tag; rel=$W/rel/$tag
    rm -rf "$W/src" "$rel"; mkdir -p "$W/src/$name"/docs "$W/src/$name"/nfq2 "$W/src/$name"/binaries/linux-x86_64 "$rel"
    ver=${tag#v}
    printf '1.0.0\n\n* old\n\n%s\n\n* new\n' "$ver" > "$W/src/$name/docs/changes.txt"
    echo '#!/bin/sh' > "$W/src/$name/blockcheck2.sh"
    cat > "$W/src/$name/install_bin.sh" <<'EOS'
#!/bin/sh
D=$(cd "$(dirname "$0")" && pwd)
ln -fs ../binaries/linux-x86_64/nfqws2 "$D/nfq2/nfqws2"
EOS
    if [ "$5" = broken_bin ]; then
        printf '#!/bin/sh\nexit 1\n' > "$W/src/$name/binaries/linux-x86_64/nfqws2"
    else
        printf '#!/bin/sh\necho "nfqws2 %s"\n' "$ver" > "$W/src/$name/binaries/linux-x86_64/nfqws2"
    fi
    chmod +x "$W/src/$name"/*.sh "$W/src/$name/binaries/linux-x86_64/nfqws2"
    [ "$4" = symlink ] && ln -s /etc/passwd "$W/src/$name/docs/evil-link"
    ( cd "$W/src" && sha256sum "$name/binaries/linux-x86_64/nfqws2" > "$rel/sha256sum.txt" )
    [ "$2" = bad_sums ] && sed -i 's/^./0/' "$rel/sha256sum.txt"
    if [ "$3" = unsafe ]; then
        ( cd "$W/src" && tar -czf "$rel/$name.tar.gz" --transform "s|^$name/docs/changes.txt|$name/../evil|" "$name" )
    else
        ( cd "$W/src" && tar -czf "$rel/$name.tar.gz" "$name" )
    fi
}

# make_base — «установленный» старый zapret2 с пользовательскими файлами
make_base() {
    rm -rf "$W/opt" "$W/opt.bak"; B=$W/opt/zapret2; mkdir -p "$B/docs" "$B/ipset" "$B/nfq2"
    printf '1.0.0\n\n1.0.6\n' > "$B/docs/changes.txt"
    echo 'install_bin.sh' > "$B/install_bin.sh"
    echo 'MY_CONFIG=1' > "$B/config"
    echo 'mysite.example' > "$B/ipset/zapret-hosts-user.txt"
}

run_update() {
    QZAPRET_TEST_BASE=$W/opt/zapret2 QZAPRET_TEST_RUNDIR=$W/run QZAPRET_TEST_NO_SERVICE=1 \
    QZAPRET_TEST_URLBASE="file://$W/rel" "$H" update "$1" 2>&1
}

expect_untouched() {
    [ "$(cat "$W/opt/zapret2/config")" = MY_CONFIG=1 ] && grep -q '^1.0.6$' "$W/opt/zapret2/docs/changes.txt" \
        && [ ! -e "$W/opt/zapret2/nfq2/nfqws2" ] || bad "$1: installation was modified"
}

# --- 1. успешное обновление
make_base; make_release v9.9.9
out=$(run_update v9.9.9); rc=$?
[ $rc -eq 0 ] && ok "update succeeds" || bad "update rc=$rc: $out"
[ "$(cat "$W/opt/zapret2/config")" = MY_CONFIG=1 ] && ok "user config preserved" || bad "config lost"
grep -q mysite.example "$W/opt/zapret2/ipset/zapret-hosts-user.txt" && ok "user hostlist preserved" || bad "hostlist lost"
grep -q '^9.9.9$' "$W/opt/zapret2/docs/changes.txt" && ok "new version installed" || bad "version not updated"
[ "$("$W/opt/zapret2/nfq2/nfqws2")" = "nfqws2 9.9.9" ] && ok "binary linked and runs" || bad "binary link broken"
grep -q '^1.0.6$' "$W/opt/zapret2.bak/docs/changes.txt" && ok "backup of previous version kept" || bad "no backup"
[ ! -e "$W/run/update.lock" ] && ok "lock released" || bad "lock left behind"

# --- 2. подменённый sha256sum.txt: ничего не меняется
make_base; make_release v9.9.9 bad_sums
out=$(run_update v9.9.9); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q 'checksum' && ok "checksum mismatch rejected" || bad "bad sums rc=$rc: $out"
expect_untouched "bad sums"

# --- 3. небезопасные пути в архиве
make_base; make_release v9.9.9 "" unsafe
out=$(run_update v9.9.9); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q "unsafe paths" && ok "unsafe archive rejected" || bad "unsafe archive rc=$rc: $out"
expect_untouched "unsafe"

# --- 4. символические ссылки в архиве
make_base; make_release v9.9.9 "" "" symlink
out=$(run_update v9.9.9); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -q 'links' && ok "symlink in archive rejected" || bad "symlink rc=$rc: $out"
expect_untouched "symlink"

# --- 5. версия в архиве не совпадает с тегом
make_base; make_release v9.9.9; mkdir -p "$W/rel/v9.9.8"; cp "$W/rel/v9.9.9/"* "$W/rel/v9.9.8/"
out=$(run_update v9.9.8); rc=$?
[ $rc -eq 2 ] && ok "missing release rejected" || bad "missing release rc=$rc: $out"

# --- 6. бинарник не запускается -> откат на прежнюю версию
make_base; make_release v9.9.9 "" "" "" broken_bin
out=$(run_update v9.9.9); rc=$?
[ $rc -eq 2 ] && echo "$out" | grep -qi 'rolling back' && ok "broken binary triggers rollback" || bad "broken bin rc=$rc: $out"
[ "$(cat "$W/opt/zapret2/config")" = MY_CONFIG=1 ] && grep -q '^1.0.6$' "$W/opt/zapret2/docs/changes.txt" \
    && ok "previous version restored" || bad "rollback did not restore"

# --- 7. валидация тега и окружения
for t in "" v "1.0.7" "v1.0.7;id" 'v1.$(id)' "v1..7" "v1.0." "vv1" "v1v2" "../v1.0.7" "v1.0.7 x"; do
    out=$(run_update "$t"); rc=$?
    [ $rc -eq 2 ] && ok "bad tag rejected: '$t'" || bad "bad tag '$t' rc=$rc: $out"
done
out=$(QZAPRET_TEST_BASE=$W/nonexistent QZAPRET_TEST_RUNDIR=$W/run QZAPRET_TEST_NO_SERVICE=1 "$H" update v1.0.7 2>&1); rc=$?
[ $rc -eq 2 ] && ok "missing installation rejected" || bad "missing install rc=$rc"

exit $fail
