#!/system/bin/sh
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 2
done
sleep 5

BIN="/data/adb/modules/aester/bin/aesterctl"
if [ -x "$BIN" ]; then
    "$BIN" apply-saved
fi
