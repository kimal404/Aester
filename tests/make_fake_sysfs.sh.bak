#!/bin/sh
set -e

DEST="${1:-/tmp/aester_fake_sysfs}"
rm -rf "$DEST"
mkdir -p "$DEST"

for p in 0 4 7; do
    POL="$DEST/devices/system/cpu/cpufreq/policy$p"
    mkdir -p "$POL"
    case $p in
        0)
            echo "schedutil performance powersave" > "$POL/scaling_available_governors"
            echo "schedutil" > "$POL/scaling_governor"
            echo "800000 1200000 1600000 2000000" > "$POL/scaling_available_frequencies"
            echo "2000000" > "$POL/scaling_max_freq"
            echo "2000000" > "$POL/cpuinfo_max_freq"
            ;;
        4)
            echo "schedutil performance powersave ondemand" > "$POL/scaling_available_governors"
            echo "schedutil" > "$POL/scaling_governor"
            echo "1000000 1500000 2000000 2500000 3000000" > "$POL/scaling_available_frequencies"
            echo "3000000" > "$POL/scaling_max_freq"
            echo "3000000" > "$POL/cpuinfo_max_freq"
            ;;
        7)
            echo "schedutil performance" > "$POL/scaling_available_governors"
            echo "schedutil" > "$POL/scaling_governor"
            echo "1200000 1800000 2400000 3250000" > "$POL/scaling_available_frequencies"
            echo "3250000" > "$POL/scaling_max_freq"
            echo "3250000" > "$POL/cpuinfo_max_freq"
            ;;
    esac
done

mkdir -p "$DEST/block/sda/queue" "$DEST/block/sdb/queue" "$DEST/block/loop0/queue" "$DEST/block/zram0"
echo "[mq-deadline] none" > "$DEST/block/sda/queue/scheduler"
echo "[mq-deadline] none kyber" > "$DEST/block/sdb/queue/scheduler"
echo "none" > "$DEST/block/loop0/queue/scheduler"

echo "Mock sysfs berhasil disiapkan di $DEST"
