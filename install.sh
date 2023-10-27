#!/bin/sh

PREFIX=${PREFIX:-"/usr/local"}
BIN=${BIN:-"$PREFIX/bin"}
SYSTEMD=${SYSTEMD:-"/etc/systemd/system"}

set -e

fail() {
    echo "$1" 1>&2
    exit 1
}

check() {
    type "install_$1" >/dev/null 2>&1 || fail "\"$1\" is unknown"
}

config() {
    file="$1"
    dir="$2"
    sed=""
    while test -n "$3"; do
        var="$3"
        val="$4"
        sed="$sed -e \"s%^$var=.*%$var=$val%\""
        shift 2
    done
    eval sed $sed < "$file" > "$dir/$file"
}

enable() {
    service="$1"
    systemctl daemon-reload
    systemctl enable "$service"
    systemctl is-active "$service" && systemctl restart "$service" || :
}

install_gw() {
    install -d "$BIN"
    install -m 755 gw "$BIN"
    config gateway.sh "$BIN" "GW" "$BIN/gw"
    test -d "$SYSTEMD" || return
    config chaosnet-gateway.service "$SYSTEMD" "WorkingDirectory" "$BIN"
    enable chaosnet-gateway
}

install_shutdown() {
    install -d "$BIN"
    install -m 755 shutdown "$BIN"
}

while test -n "$1"; do
    check "$1"
    echo "Installing \"$1\""
    install_"$1"
    shift
done
