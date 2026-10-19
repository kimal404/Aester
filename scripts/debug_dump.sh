#!/bin/sh
set -e

OUT="/storage/emulated/0/Download/aester_dump.txt"
if ! touch "$OUT" 2>/dev/null; then
    OUT="/tmp/aester_dump.txt"
fi

{
    echo "=== AESTER HARDWARE & KERNEL DUMP ==="
    echo "Waktu: $(date)"
    echo "Kernel: $(uname -a)"
    echo ""

    echo "--- CPUFREQ POLICIES ---"
    for p in /sys/devices/system/cpu/cpufreq/policy*; do
        if [ -d "$p" ]; then
            echo "Policy: $p"
            [ -f "$p/cpuinfo_max_freq" ] && echo "  cpuinfo_max: $(cat "$p/cpuinfo_max_freq")"
            [ -f "$p/scaling_max_freq" ] && echo "  scaling_max: $(cat "$p/scaling_max_freq")"
            [ -f "$p/scaling_governor" ] && echo "  governor: $(cat "$p/scaling_governor")"
            [ -f "$p/scaling_available_governors" ] && echo "  avail_govs: $(cat "$p/scaling_available_governors")"
            [ -f "$p/scaling_available_frequencies" ] && echo "  avail_freqs: $(cat "$p/scaling_available_frequencies")"
        fi
    done
    echo ""

    echo "--- BLOCK SCHEDULERS ---"
    for s in /sys/block/*/queue/scheduler; do
        if [ -f "$s" ]; then
            dev="$(echo "$s" | cut -d/ -f4)"
            echo "Block dev: $dev -> $(cat "$s")"
        fi
    done
} > "$OUT"

echo "Dump sistem berhasil ditulis ke: $OUT"
