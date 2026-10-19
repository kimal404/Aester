#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
FAKE_SYSFS="/tmp/aester_fake_sysfs_$$"
FAKE_STATE="/tmp/aester_fake_state_$$"

cleanup() {
    rm -rf "$FAKE_SYSFS" "$FAKE_STATE"
}
trap cleanup EXIT INT TERM

echo "[1/5] Membangun binary debug..."
"$DIR/scripts/build.sh" debug

echo "[2/5] Menyiapkan mock sysfs..."
"$DIR/tests/make_fake_sysfs.sh" "$FAKE_SYSFS"
mkdir -p "$FAKE_STATE"

export AESTER_SYSFS_ROOT="$FAKE_SYSFS"
export AESTER_STATE_DIR="$FAKE_STATE"
export AESTER_LOG="debug"

BIN="$DIR/build/aesterctl"

echo "[3/5] Menjalankan unit tests core..."
if [ -f "$DIR/build/test_validate" ]; then
    "$DIR/build/test_validate"
fi
if [ -f "$DIR/build/test_governor" ]; then
    "$DIR/build/test_governor"
fi
if [ -f "$DIR/build/test_io" ]; then
    "$DIR/build/test_io"
fi

echo "[4/5] Menguji fungsionalitas CLI dasar & format kontrak..."

# 1. Version
VER_OUT="$("$BIN" version)"
if [ "$VER_OUT" != "1.0.0 1" ]; then
    echo "FAILED: version output tidak cocok: '$VER_OUT'" >&2
    exit 1
fi
echo "PASSED: aesterctl version -> $VER_OUT"

# 2. I/O: mtdblock0 dilewati & device terurut alfabetis (sda, sdb)
IO_LIST="$("$BIN" io list)"
if echo "$IO_LIST" | grep -q "mtdblock"; then
    echo "FAILED: mtdblock tidak dilewati pada io list" >&2
    exit 1
fi
echo "PASSED: io list mengabaikan mtdblock"

# 3. Status key=value dan status --pretty
STATUS_KV="$("$BIN" status)"
for key in "cpufreq.requested=off" "cpufreq.policy0.max=2000000" "governor.requested=-" "governor.policy0=schedutil" "io.requested=-" "io.sda=mq-deadline" "io.sdb=mq-deadline"; do
    if ! echo "$STATUS_KV" | grep -q "^$key"; then
        echo "FAILED: status key=value tidak memuat: $key" >&2
        exit 1
    fi
done
if echo "$STATUS_KV" | grep -q "mtdblock"; then
    echo "FAILED: status key=value memuat mtdblock" >&2
    exit 1
fi
echo "PASSED: status format key=value sesuai kontrak"

STATUS_PRETTY="$("$BIN" status --pretty)"
if ! echo "$STATUS_PRETTY" | grep -q "Active Devices   : sda, sdb"; then
    echo "FAILED: status --pretty tidak menampilkan Active Devices terurut alfabet tanpa mtdblock" >&2
    exit 1
fi
echo "PASSED: status --pretty sesuai format manusiawi"

echo "[5/5] Menguji dry-run dan pemulihan partial failure..."

# 4. Mode Dry-Run: verifikasi dilewati, sysfs tidak berubah, stdout "dry-run <target>", exit 0
export AESTER_DRYRUN="1"

DRY_CPU="$("$BIN" cpufreq set 80)"
if [ "$DRY_CPU" != "dry-run 80" ]; then
    echo "FAILED: dry-run cpufreq set menghasilkan: '$DRY_CPU'" >&2
    exit 1
fi
if [ "$(cat "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy0/scaling_max_freq")" != "2000000" ]; then
    echo "FAILED: dry-run cpufreq memodifikasi sysfs!" >&2
    exit 1
fi

DRY_GOV="$("$BIN" governor set performance)"
if [ "$DRY_GOV" != "dry-run performance" ]; then
    echo "FAILED: dry-run governor set menghasilkan: '$DRY_GOV'" >&2
    exit 1
fi
if [ "$(cat "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy0/scaling_governor")" != "schedutil" ]; then
    echo "FAILED: dry-run governor memodifikasi sysfs!" >&2
    exit 1
fi

DRY_IO="$("$BIN" io set none)"
if [ "$DRY_IO" != "dry-run none" ]; then
    echo "FAILED: dry-run io set menghasilkan: '$DRY_IO'" >&2
    exit 1
fi
if ! grep -q "\[mq-deadline\]" "$FAKE_SYSFS/block/sda/queue/scheduler"; then
    echo "FAILED: dry-run io memodifikasi sysfs!" >&2
    exit 1
fi

unset AESTER_DRYRUN
echo "PASSED: mode dry-run exit 0 dan tidak memodifikasi sysfs"

# 5. Kegagalan sebagian (partial failure): tulis semua, rollback best-effort, exit 4, stderr memuat target gagal
chmod 444 "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy4/scaling_governor"

set +e
ERR_GOV="$("$BIN" governor set performance 2>&1 >/dev/null)"
GOV_CODE=$?
set -e

if [ "$GOV_CODE" -ne 4 ]; then
    echo "FAILED: governor set parsial gagal seharusnya exit 4, keluar: $GOV_CODE" >&2
    exit 1
fi
if ! echo "$ERR_GOV" | grep -q "policy4"; then
    echo "FAILED: stderr tidak menyebutkan policy4: '$ERR_GOV'" >&2
    exit 1
fi

# Pastikan rollback mengembalikan policy0 dan policy7 ke nilai awal
if [ "$(cat "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy0/scaling_governor")" != "schedutil" ]; then
    echo "FAILED: rollback governor policy0 gagal" >&2
    exit 1
fi
if [ "$(cat "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy7/scaling_governor")" != "schedutil" ]; then
    echo "FAILED: rollback governor policy7 gagal" >&2
    exit 1
fi

chmod 644 "$FAKE_SYSFS/devices/system/cpu/cpufreq/policy4/scaling_governor"
echo "PASSED: kegagalan parsial dicoba semua, dipulihkan, exit 4, stderr menyebut target gagal"

echo "\nSemua kasus uji perbaikan SUKSES."
