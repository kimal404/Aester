#!/system/bin/sh
MODDIR=${0%/*}

while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done

# Jeda agar driver DVFS MediaTek selesai inisialisasi awal
sleep 5

if [ -f "$MODDIR/bin/aesterctl" ]; then
    chmod 755 "$MODDIR/bin/aesterctl"
    "$MODDIR/bin/aesterctl" apply-saved > /dev/kmsg 2>&1
fi
