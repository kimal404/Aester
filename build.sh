#!/bin/sh

REPO="kimal404/Aester"
COMMIT_MSG="${1:-chore: update backend implementation}"
CURRENT_DIR="$(pwd)"

echo "==========================================="
echo "   AESTER AUTOMATED CI & OTA DEPLOYMENT    "
echo "==========================================="

# 1. Stage & Commit
echo "\n[1/4] Menyiapkan commit..."
git add .
if git diff-index --quiet HEAD -- 2>/dev/null; then
    echo ">> Tidak ada perubahan file baru. Melanjutkan dengan commit terakhir."
else
    git commit -m "$COMMIT_MSG"
fi

# 2. Push ke GitHub
echo "\n[2/4] Mengunggah perubahan ke GitHub..."
git push origin main || {
    echo "\n[ERROR] Gagal push ke GitHub. Periksa koneksi atau kredensial Git."
    exit 1
}

COMMIT_SHA=$(git rev-parse HEAD)
SHORT_SHA=$(git rev-parse --short HEAD)
echo ">> Target SHA: $SHORT_SHA ($COMMIT_SHA)"

# 3. Pantau GitHub Actions
echo "\n[3/4] Memantau proses build di GitHub Actions..."
API_URL="https://api.github.com/repos/${REPO}/actions/runs?head_sha=${COMMIT_SHA}"

sleep 4

START_TIME=$(date +%s)
TIMEOUT=300

while true; do
    NOW=$(date +%s)
    ELAPSED=$((NOW - START_TIME))

    if [ "$ELAPSED" -ge "$TIMEOUT" ]; then
        echo "\n\n[ERROR] Timeout 5 menit tercapai."
        echo "Periksa status manual di: https://github.com/${REPO}/actions"
        exit 1
    fi

    RESP=$(curl -s -H "Accept: application/vnd.github+json" "$API_URL" 2>/dev/null || true)

    STATUS=$(echo "$RESP" | grep -m1 '"status":' | sed -n 's/.*"status": "\([^"]*\)".*/\1/p')
    CONCLUSION=$(echo "$RESP" | grep -m1 '"conclusion":' | sed -n 's/.*"conclusion": "\([^"]*\)".*/\1/p')
    RUN_URL=$(echo "$RESP" | grep -m1 '"html_url":' | sed -n 's/.*"html_url": "\([^"]*\)".*/\1/p')

    if [ -z "$STATUS" ]; then
        printf "\r>> Menunggu antrean workflow runner... (%ds)" "$ELAPSED"
    elif [ "$STATUS" != "completed" ]; then
        printf "\r>> Mengompilasi NDK arm64 [status: %s]... (%ds)" "$STATUS" "$ELAPSED"
    else
        echo ""
        if [ "$CONCLUSION" = "success" ]; then
            echo "\n>> [SUCCESS] Kompilasi NDK & rilis nightly berhasil!"
            break
        else
            echo "\n\n[ERROR] Kompilasi GAGAL! (Conclusion: $CONCLUSION)"
            echo "Lihat detail log error di: $RUN_URL"
            exit 1
        fi
    fi

    sleep 5
done

# 4. Unduh via Termux curl & Timpa Binary via Root
echo "\n[4/4] Mengunduh binary 'nightly' dan menimpa modul root..."
TMP_BIN="${CURRENT_DIR}/aesterctl_nightly"

curl -sL "https://github.com/${REPO}/releases/download/nightly/aesterctl" -o "$TMP_BIN" || {
    echo "\n[ERROR] Gagal mengunduh binary melalui curl Termux."
    exit 1
}

su -c "cp '$TMP_BIN' /data/adb/modules/aester/bin/aesterctl && chmod 755 /data/adb/modules/aester/bin/aesterctl && rm -f '$TMP_BIN'" || {
    echo "\n[ERROR] Gagal menyalin binary ke /data/adb/modules/aester/bin/."
    exit 1
}

echo ">> Verifikasi binary modul aktif:"
su -c "/data/adb/modules/aester/bin/aesterctl version"

echo "\nSemua proses selesai! Binary aesterctl terbaru siap digunakan."
