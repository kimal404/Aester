#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
FAKE_SYSFS="/tmp/aester_fake_sysfs_$$"
FAKE_STATE="/tmp/aester_fake_state_$$"

cleanup() {
    rm -rf "$FAKE_SYSFS" "$FAKE_STATE"
}
trap cleanup EXIT INT TERM

echo "[1/4] Membangun binary debug..."
"$DIR/scripts/build.sh" debug

echo "[2/4] Menyiapkan mock sysfs..."
"$DIR/tests/make_fake_sysfs.sh" "$FAKE_SYSFS"
mkdir -p "$FAKE_STATE"

export AESTER_SYSFS_ROOT="$FAKE_SYSFS"
export AESTER_STATE_DIR="$FAKE_STATE"
export AESTER_LOG="debug"

echo "[3/4] Menjalankan unit tests core..."
"$DIR/build/test_validate"

echo "[4/4] Menjalankan smoke test CLI aesterctl..."
BIN="$DIR/build/aesterctl"

VER_OUT="$("$BIN" version)"
if [ "$VER_OUT" != "1.0.0 1" ]; then
    echo "FAILED: version output tidak cocok: '$VER_OUT'" >&2
    exit 1
fi
echo "PASSED: aesterctl version -> $VER_OUT"

for cmd in "cpufreq get" "cpufreq set 70" "governor list" "governor get" "governor set schedutil" "io list" "io get" "io set none" "apply-saved" "status"; do
    set +e
    "$BIN" $cmd >/dev/null 2>/dev/null
    code=$?
    set -e
    if [ "$code" -ne 3 ]; then
        echo "FAILED: aesterctl $cmd seharusnya exit 3, tapi keluar $code" >&2
        exit 1
    fi
    echo "PASSED: aesterctl $cmd mengembalikan exit 3"
done

set +e
"$BIN" invalid-command >/dev/null 2>/dev/null
code=$?
set -e
if [ "$code" -ne 2 ]; then
    echo "FAILED: invalid-command seharusnya exit 2, tapi keluar $code" >&2
    exit 1
fi
echo "PASSED: aesterctl invalid-command mengembalikan exit 2"

echo "\nSemua tes Batch A SUKSES."
