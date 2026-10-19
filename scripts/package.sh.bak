#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"

echo "1. Membangun release binary..."
"$DIR/scripts/build.sh" release
"$DIR/scripts/check_deps.sh"

echo "2. Menyalin binary ke folder module..."
mkdir -p "$DIR/module/bin" "$DIR/dist"
cp "$DIR/build/aesterctl" "$DIR/module/bin/aesterctl"
chmod 755 "$DIR/module/bin/aesterctl"

VERSION="1.0.0"
ZIP_NAME="aester-backend-${VERSION}.zip"
ZIP_PATH="$DIR/dist/$ZIP_NAME"

echo "3. Mengemas flashable module ZIP..."
rm -f "$ZIP_PATH"
cd "$DIR/module"
zip -r9 "$ZIP_PATH" . -x "bin/.keep"

echo "4. Menyalin paket ke Download storage..."
TARGET_DIR="/storage/emulated/0/Download"
if [ -d "$TARGET_DIR" ] && [ -w "$TARGET_DIR" ]; then
    cp "$ZIP_PATH" "$TARGET_DIR/$ZIP_NAME"
    echo "ZIP berhasil disalin ke: $TARGET_DIR/$ZIP_NAME"
else
    echo "Catatan: Direktori $TARGET_DIR tidak dapat ditulis langsung, paket tersedia di$ZIP_PATH"
fi
