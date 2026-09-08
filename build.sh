#!/usr/bin/env bash
# ===================================================================
#   GhostView Linux Derleme Scripti (Linux Build Script)
# ===================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "==================================================================="
echo "  GhostView Linux Derleyici Scripti (Build Script)"
echo "==================================================================="

# Temizleme seçeneği (--clean veya clean)
if [ "$1" = "--clean" ] || [ "$1" = "clean" ]; then
    echo "Önceki derleme çıktıları temizleniyor (make clean)..."
    make -C "$SCRIPT_DIR" clean
fi

NUM_CORES=$(nproc 2>/dev/null || echo 2)
echo "Derleme başlatılıyor (${NUM_CORES} çekirdek)..."
make -C "$SCRIPT_DIR" -j"$NUM_CORES"

echo ""
echo "==================================================================="
echo "  ✓ DERLEME BAŞARIYLA TAMAMLANDI!"
echo "  İkili dosya: $SCRIPT_DIR/GhostView"
echo ""
echo "  - Çalıştırmak için: ./GhostView <resim_yolu>"
echo "  - Varsayılan uygulama olarak ayarlamak için: ./set_default.sh"
echo "==================================================================="
