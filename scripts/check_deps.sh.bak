#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$DIR/build/aesterctl"

if [ ! -f "$BIN" ]; then
    echo "Binary $BIN belum dibangun. Jalankan scripts/build.sh terlebih dahulu." >&2
    exit 1
fi

echo "Memeriksa dependensi binary aesterctl..."
if command -v readelf >/dev/null 2>&1; then
    NEEDED="$(readelf -d "$BIN" 2>/dev/null | grep NEEDED || true)"
    if [ -z "$NEEDED" ]; then
        echo "OK: Binary aesterctl murni statis (tidak memiliki dependensi dinamis)."
    else
        echo "Peringatan: Ditemukan dynamic library dependencies:"
        echo "$NEEDED"
        if echo "$NEEDED" | grep -q "libc++_shared.so"; then
            echo "Error: Binary masih bergantung pada libc++_shared.so milik Termux." >&2
            exit 1
        fi
    fi
else
    echo "readelf tidak ditemukan, verifikasi dilewati."
fi
