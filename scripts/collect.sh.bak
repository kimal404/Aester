#!/bin/sh
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"

find "$DIR" -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.sh" -o -name "*.md" -o -name "*.prop" -o -name "CMakeLists.txt" \) ! -name "*.bak" | sort | while read -r f; do
    echo "===== $f ====="
    cat "$f"
    echo ""
done
