#!/usr/bin/env bash
# ===================================================================
#   GhostView Linux Varsayılan Resim Görüntüleyici Kaldırıcı
# ===================================================================

set -e

BIN_DEST="$HOME/.local/bin"
APP_DEST="$HOME/.local/share/applications"
ICON_DEST="$HOME/.local/share/icons/hicolor/256x256/apps"
MIMEAPPS_LIST="$HOME/.config/mimeapps.list"

echo "GhostView sistem entegrasyonu kaldırılıyor..."

rm -f "$BIN_DEST/ghostview" "$BIN_DEST/GhostView"
rm -f "$APP_DEST/ghostview.desktop"
rm -f "$ICON_DEST/ghostview.png"

if [ -f "$MIMEAPPS_LIST" ]; then
    sed -i '/ghostview\.desktop/d' "$MIMEAPPS_LIST"
fi

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DEST" 2>/dev/null || true
fi

echo "GhostView entegrasyonu başarıyla kaldırıldı."
