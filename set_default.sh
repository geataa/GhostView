#!/usr/bin/env bash
# ===================================================================
#   GhostView Linux Varsayılan Resim & GIF Görüntüleyici Ayarlayıcı
#   (GhostView Linux Default Image & GIF Viewer Setup)
#
#   Bu betik, derlenmiş 'GhostView' programı ile birlikte çalışır.
#   Makefile, derleyici veya geliştirici aracı gerektirmez!
# ===================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DEST="$HOME/.local/bin"
APP_DEST="$HOME/.local/share/applications"
ICON_DEST="$HOME/.local/share/icons/hicolor/256x256/apps"
SCALABLE_ICON_DEST="$HOME/.local/share/icons/hicolor/scalable/apps"
PIXMAP_DEST="$HOME/.local/share/pixmaps"
MIMEAPPS_CONFIG="$HOME/.config/mimeapps.list"
MIMEAPPS_LOCAL="$HOME/.local/share/applications/mimeapps.list"

echo "==================================================================="
echo "  GhostView Linux Varsayılan Resim & GIF Görüntüleyici Kurulumu"
echo "==================================================================="

# 1. Derlenmiş GhostView ikili dosyasını bul
TARGET_BIN=""
if [ -n "$1" ] && [ -f "$1" ]; then
    TARGET_BIN="$(readlink -f "$1")"
elif [ -f "$SCRIPT_DIR/GhostView" ]; then
    TARGET_BIN="$(readlink -f "$SCRIPT_DIR/GhostView")"
elif [ -f "$SCRIPT_DIR/ghostview" ]; then
    TARGET_BIN="$(readlink -f "$SCRIPT_DIR/ghostview")"
elif [ -f "./GhostView" ]; then
    TARGET_BIN="$(readlink -f "./GhostView")"
elif [ -f "./ghostview" ]; then
    TARGET_BIN="$(readlink -f "./ghostview")"
elif [ -f "$SCRIPT_DIR/bin/GhostView" ]; then
    TARGET_BIN="$(readlink -f "$SCRIPT_DIR/bin/GhostView")"
elif [ -f "$SCRIPT_DIR/bin/ghostview" ]; then
    TARGET_BIN="$(readlink -f "$SCRIPT_DIR/bin/ghostview")"
elif [ -f "$BIN_DEST/ghostview" ]; then
    TARGET_BIN="$BIN_DEST/ghostview"
elif command -v ghostview >/dev/null 2>&1; then
    TARGET_BIN="$(command -v ghostview)"
elif command -v GhostView >/dev/null 2>&1; then
    TARGET_BIN="$(command -v GhostView)"
fi

if [ -z "$TARGET_BIN" ] || [ ! -f "$TARGET_BIN" ]; then
    echo "==================================================================="
    echo "  [UYARI] Derlenmiş 'GhostView' programı bulunamadı!"
    echo ""
    echo "  Lütfen derlenmiş 'GhostView' dosyasını bu betik ile aynı klasöre"
    echo "  koyup tekrar çalıştırın."
    echo "==================================================================="
    exit 1
fi

echo "  -> Bulunan program: $TARGET_BIN"

# Çalıştırma yetkisini ver
chmod +x "$TARGET_BIN" 2>/dev/null || true

# 2. ~/.local/bin dizinine kopyalama
echo "[1/4] Dosyalar yükleniyor (~/.local/bin)..."
mkdir -p "$BIN_DEST"
if [ "$TARGET_BIN" != "$BIN_DEST/ghostview" ]; then
    cp -f "$TARGET_BIN" "$BIN_DEST/ghostview"
fi
chmod +x "$BIN_DEST/ghostview"
ln -sf "$BIN_DEST/ghostview" "$BIN_DEST/GhostView"

# Eksik kütüphane kontrolü (varsa uyar)
if command -v ldd >/dev/null 2>&1; then
    MISSING_LIBS=$(ldd "$BIN_DEST/ghostview" 2>/dev/null | grep "not found" || true)
    if [ -n "$MISSING_LIBS" ]; then
        echo "  [DİKKAT] Sisteminizde eksik kütüphane tespit edildi:"
        echo "$MISSING_LIBS"
    fi
fi

# 3. İkon Yükleme
mkdir -p "$ICON_DEST" "$SCALABLE_ICON_DEST" "$PIXMAP_DEST"
if [ -f "$SCRIPT_DIR/ghostview.png" ]; then
    cp -f "$SCRIPT_DIR/ghostview.png" "$ICON_DEST/ghostview.png"
    cp -f "$SCRIPT_DIR/ghostview.png" "$PIXMAP_DEST/ghostview.png"
elif [ -f "./ghostview.png" ]; then
    cp -f "./ghostview.png" "$ICON_DEST/ghostview.png"
    cp -f "./ghostview.png" "$PIXMAP_DEST/ghostview.png"
else
    # Gömülü SVG ikon oluştur (bağımsız çalışma garantisi)
    printf '%s
' '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="128" height="128"><defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stop-color="#ffffff"/><stop offset="100%" stop-color="#d0d5dd"/></linearGradient></defs><rect width="128" height="128" rx="28" fill="#1e1e2e"/><path d="M64 22 C42 22 26 38 26 60 C26 78 26 96 26 102 C26 106 32 104 36 99 C40 94 44 94 48 99 C52 104 56 104 60 99 C64 94 68 94 72 99 C76 104 80 104 84 99 C88 94 92 94 96 99 C100 104 106 106 106 102 C106 96 106 78 106 60 C106 38 86 22 64 22 Z" fill="url(#g)"/><ellipse cx="50" cy="54" rx="6" ry="9" fill="#1e1e2e"/><ellipse cx="78" cy="54" rx="6" ry="9" fill="#1e1e2e"/><ellipse cx="64" cy="70" rx="4" ry="6" fill="#1e1e2e"/></svg>' > "$SCALABLE_ICON_DEST/ghostview.svg"
    cp -f "$SCALABLE_ICON_DEST/ghostview.svg" "$PIXMAP_DEST/ghostview.svg"
fi

# 4. .desktop Girişi Oluşturma
echo "[2/4] Masaüstü entegrasyonu (ghostview.desktop) oluşturuluyor..."
mkdir -p "$APP_DEST"

MIME_LIST="image/jpeg;image/jpg;image/pjpeg;image/png;image/x-png;image/gif;image/webp;image/bmp;image/x-bmp;image/x-ms-bmp;image/tiff;image/x-tiff;image/vnd.microsoft.icon;image/x-icon;image/x-ico;image/x-tga;image/x-icb;image/x-pcx;image/x-portable-anymap;image/x-portable-bitmap;image/x-portable-graymap;image/x-portable-pixmap;image/x-xbitmap;image/x-xpixmap;image/svg+xml;image/avif;image/heif;image/heic;"

cat << DESKTOP_EOF > "$APP_DEST/ghostview.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=GhostView
GenericName=Image & GIF Viewer
GenericName[tr]=Resim ve GIF Görüntüleyici
Comment=Fast, Minimalist Cross-Platform Photo & GIF Viewer
Comment[tr]=Hızlı, Minimalist Fotoğraf ve GIF Görüntüleyici
TryExec=$BIN_DEST/ghostview
Exec="$BIN_DEST/ghostview" %U
Icon=ghostview
Terminal=false
Categories=Graphics;Viewer;2DGraphics;RasterGraphics;Photography;
StartupNotify=true
MimeType=$MIME_LIST
DESKTOP_EOF

chmod +x "$APP_DEST/ghostview.desktop"

# 5. MIME İlişkilendirmelerini Tanımlama
echo "[3/4] GIF dahil tüm resim formatları GhostView ile varsayılan yapılıyor..."

MIME_ARRAY=(
    "image/jpeg"
    "image/jpg"
    "image/pjpeg"
    "image/png"
    "image/x-png"
    "image/gif"
    "image/webp"
    "image/bmp"
    "image/x-bmp"
    "image/x-ms-bmp"
    "image/tiff"
    "image/x-tiff"
    "image/vnd.microsoft.icon"
    "image/x-icon"
    "image/x-ico"
    "image/x-tga"
    "image/x-icb"
    "image/x-pcx"
    "image/x-portable-anymap"
    "image/x-portable-bitmap"
    "image/x-portable-graymap"
    "image/x-portable-pixmap"
    "image/x-xbitmap"
    "image/x-xpixmap"
    "image/svg+xml"
    "image/avif"
    "image/heif"
    "image/heic"
)

# A. GIO ile ayarla (GNOME / Ubuntu Nautilus için EN ETKİLİ yöntem)
if command -v gio >/dev/null 2>&1; then
    for mime in "${MIME_ARRAY[@]}"; do
        gio mime "$mime" ghostview.desktop >/dev/null 2>&1 || true
    done
fi

# B. xdg-mime ile ayarla (XFCE, KDE, MATE vb. için)
if command -v xdg-mime >/dev/null 2>&1; then
    for mime in "${MIME_ARRAY[@]}"; do
        xdg-mime default ghostview.desktop "$mime" >/dev/null 2>&1 || true
    done
fi

# C. Hem ~/.config/mimeapps.list hem de ~/.local/share/applications/mimeapps.list dosyalarına doğrudan yaz
update_mimeapps() {
    local target="$1"
    [ -z "$target" ] && return
    mkdir -p "$(dirname "$target")"
    touch "$target"

    # [Default Applications] bölümü
    if ! grep -q "^\[Default Applications\]" "$target"; then
        printf '
[Default Applications]
' >> "$target"
    fi
    for mime in "${MIME_ARRAY[@]}"; do
        if grep -q "^$mime=" "$target"; then
            sed -i "s|^$mime=.*|$mime=ghostview.desktop|" "$target"
        else
            sed -i "/^\[Default Applications\]/a $mime=ghostview.desktop" "$target"
        fi
    done

    # [Added Associations] bölümü
    if ! grep -q "^\[Added Associations\]" "$target"; then
        printf '
[Added Associations]
' >> "$target"
    fi
    for mime in "${MIME_ARRAY[@]}"; do
        if grep -q "^$mime=" "$target"; then
            if ! grep -q "^$mime=.*ghostview\.desktop" "$target"; then
                sed -i "s|^$mime=|$mime=ghostview.desktop;|" "$target"
            fi
        else
            sed -i "/^\[Added Associations\]/a $mime=ghostview.desktop;" "$target"
        fi
    done
}

update_mimeapps "$MIMEAPPS_CONFIG"
update_mimeapps "$MIMEAPPS_LOCAL"

# D. Desktop veritabanı ve MIME önbelleğini yenile
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DEST" >/dev/null 2>&1 || true
    update-desktop-database "$HOME/.local/share/applications" >/dev/null 2>&1 || true
fi
if command -v update-mime-database >/dev/null 2>&1; then
    update-mime-database "$HOME/.local/share/mime" >/dev/null 2>&1 || true
fi

# E. Nautilus önbelleğini yenile (GNOME dosya yöneticisi hemen tanısın)
if command -v nautilus >/dev/null 2>&1; then
    nautilus -q >/dev/null 2>&1 || true
fi

echo "[4/4] Doğrulanıyor..."
SUCCESS_COUNT=0
for mime in "image/jpeg" "image/png" "image/gif" "image/webp"; do
    CUR_DEF=""
    if command -v gio >/dev/null 2>&1; then
        CUR_DEF=$(gio mime "$mime" 2>/dev/null | head -n 1 | awk '{print $NF}')
    elif command -v xdg-mime >/dev/null 2>&1; then
        CUR_DEF=$(xdg-mime query default "$mime" 2>/dev/null || true)
    fi
    if [ "$CUR_DEF" = "ghostview.desktop" ]; then
        echo "  ✓ $mime -> GhostView"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        echo "  - $mime -> $CUR_DEF"
    fi
done

echo "==================================================================="
if [ "$SUCCESS_COUNT" -gt 0 ]; then
    echo "  ✓ İŞLEM BAŞARIYLA TAMAMLANDI!"
    echo ""
    echo "  GhostView artık sisteminizde varsayılan resim görüntüleyicidir."
    echo "  Masaüstünüzde veya dosya yöneticinizde herhangi bir resme"
    echo "  çift tıkladığınızda GhostView açılacaktır."
    echo ""
    echo "  Program konumu : $BIN_DEST/ghostview"
    echo "  Masaüstü girişi: $APP_DEST/ghostview.desktop"
else
    echo "  [!] Kurulum tamamlandı ancak bazı formatlar eski uygulamada kalmış olabilir."
    echo "  Lütfen oturumu kapatıp açmayı deneyin."
fi
echo "==================================================================="
