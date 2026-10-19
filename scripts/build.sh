#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_TYPE="${1:-release}"

case "$BUILD_TYPE" in
    debug|Debug)
        CMAKE_TYPE="Debug"
        ;;
    release|Release|*)
        CMAKE_TYPE="Release"
        ;;
esac

mkdir -p "$DIR/build"
cd "$DIR/build"

if ! command -v cmake >/dev/null 2>&1 || ! command -v make >/dev/null 2>&1; then
    echo "Error: toolchain cmake/make belum terpasang di Termux." >&2
    exit 1
fi

cmake -DCMAKE_BUILD_TYPE="$CMAKE_TYPE" ..
make -j"$(nproc 2>/dev/null || echo 2)"

echo "Kompilasi sukses ($CMAKE_TYPE):$DIR/build/aesterctl"
