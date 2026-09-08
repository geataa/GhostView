#!/usr/bin/env bash
# ===================================================================
#   GhostView Linux Varsayılan Resim & GIF Görüntüleyici Ayarlayıcı
#   (GhostView Linux Default Image & GIF Viewer Setup)
# ===================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DEST="$HOME/.local/bin"
APP_DEST="$HOME/.local/share/applications"
ICON_DEST="$HOME/.local/share/icons/hicolor/256x256/apps"
PIXMAP_DEST="$HOME/.local/share/pixmaps"
MIMEAPPS_LIST="$HOME/.config/mimeapps.list"

echo "==================================================================="
echo "  GhostView Linux Varsayılan Resim & GIF Görüntüleyici Kurulumu"
echo "==================================================================="

# 1. Binary Kontrolü (Derleme build.sh ile yapılmalıdır)
if [ ! -f "$SCRIPT_DIR/GhostView" ]; then
    echo "HATA: GhostView ikili dosyası bulunamadı!"
    echo "Lütfen önce programı derleyin: ./build.sh"
    exit 1
fi

# 2. ~/.local/bin dizinine kopyalama
echo "[1/4] Dosyalar yükleniyor..."
mkdir -p "$BIN_DEST"
cp -f "$SCRIPT_DIR/GhostView" "$BIN_DEST/ghostview"
chmod +x "$BIN_DEST/ghostview"
ln -sf "$BIN_DEST/ghostview" "$BIN_DEST/GhostView"

# 3. İkon Yükleme
mkdir -p "$ICON_DEST" "$PIXMAP_DEST"
if [ -f "$SCRIPT_DIR/ghostview.png" ]; then
    cp -f "$SCRIPT_DIR/ghostview.png" "$ICON_DEST/ghostview.png"
    cp -f "$SCRIPT_DIR/ghostview.png" "$PIXMAP_DEST/ghostview.png"
elif [ -f "$SCRIPT_DIR/app.ico" ] && command -v python3 >/dev/null 2>&1; then
    python3 -c "
from PIL import Image
img = Image.open('$SCRIPT_DIR/app.ico')
img.save('$ICON_DEST/ghostview.png')
img.save('$PIXMAP_DEST/ghostview.png')
" 2>/dev/null || true
fi

# 4. .desktop Girişi Oluşturma
echo "[2/4] Masaüstü entegrasyonu (ghostview.desktop) oluşturuluyor..."
mkdir -p "$APP_DEST"

MIME_LIST="image/gif;image/png;image/jpeg;image/jpg;image/pjpeg;image/bmp;image/x-bmp;image/x-ms-bmp;image/webp;image/tiff;image/x-tiff;image/vnd.microsoft.icon;image/x-icon;image/x-ico;image/x-tga;image/x-pcx;image/x-portable-anymap;image/x-portable-bitmap;image/x-portable-graymap;image/x-portable-pixmap;image/x-xbitmap;image/x-xpixmap;"

cat <<EOF > "$APP_DEST/ghostview.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=GhostView
GenericName=Image & GIF Viewer
GenericName[tr]=Resim ve GIF Görüntüleyici
Comment=Fast, Minimalist Cross-Platform Photo & GIF Viewer
Comment[tr]=Hızlı, Minimalist Fotoğraf ve GIF Görüntüleyici
Exec=$BIN_DEST/ghostview %F
Icon=ghostview
Terminal=false
Categories=Graphics;Viewer;2DGraphics;RasterGraphics;Photography;
StartupNotify=true
MimeType=$MIME_LIST
EOF

chmod +x "$APP_DEST/ghostview.desktop"

# Desktop veritabanını güncelle
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$APP_DEST" 2>/dev/null || true
fi

# 5. MIME İlişkilendirmelerini Tanımlama (xdg-mime + mimeapps.list)
echo "[3/4] GIF dahil tüm resim formatları GhostView ile ilişkilendiriliyor..."

MIME_ARRAY=(
    "image/gif"
    "image/png"
    "image/jpeg"
    "image/jpg"
    "image/pjpeg"
    "image/bmp"
    "image/x-bmp"
    "image/x-ms-bmp"
    "image/webp"
    "image/tiff"
    "image/x-tiff"
    "image/vnd.microsoft.icon"
    "image/x-icon"
    "image/x-ico"
    "image/x-tga"
    "image/x-pcx"
    "image/x-portable-anymap"
    "image/x-portable-bitmap"
    "image/x-portable-graymap"
    "image/x-portable-pixmap"
    "image/x-xbitmap"
    "image/x-xpixmap"
)

# xdg-mime ile ayarla
if command -v xdg-mime >/dev/null 2>&1; then
    for mime in "${MIME_ARRAY[@]}"; do
        xdg-mime default ghostview.desktop "$mime" 2>/dev/null || true
    done
fi

# ~/.config/mimeapps.list dosyasına doğrudan güvenilir şekilde yaz
mkdir -p "$HOME/.config"
touch "$MIMEAPPS_LIST"

# [Default Applications] bölümüne ekle
for mime in "${MIME_ARRAY[@]}"; do
    if grep -q "^\[Default Applications\]" "$MIMEAPPS_LIST"; then
        if grep -q "^$mime=" "$MIMEAPPS_LIST"; then
            sed -i "s|^$mime=.*|$mime=ghostview.desktop;|" "$MIMEAPPS_LIST"
        else
            sed -i "/^\[Default Applications\]/a $mime=ghostview.desktop;" "$MIMEAPPS_LIST"
        fi
    else
        echo -e "\n[Default Applications]\n$mime=ghostview.desktop;" >> "$MIMEAPPS_LIST"
    fi
done

# [Added Associations] bölümüne de ekle
for mime in "${MIME_ARRAY[@]}"; do
    if grep -q "^\[Added Associations\]" "$MIMEAPPS_LIST"; then
        if grep -q "^$mime=" "$MIMEAPPS_LIST"; then
            # Mevcut satıra en başa ekle (eğer yoksa)
            if ! grep -q "^$mime=.*ghostview\.desktop" "$MIMEAPPS_LIST"; then
                sed -i "s|^$mime=|$mime=ghostview.desktop;|" "$MIMEAPPS_LIST"
            fi
        else
            sed -i "/^\[Added Associations\]/a $mime=ghostview.desktop;" "$MIMEAPPS_LIST"
        fi
    else
        echo -e "\n[Added Associations]\n$mime=ghostview.desktop;" >> "$MIMEAPPS_LIST"
    fi
done

echo "[4/4] Doğrulanıyor..."
if command -v xdg-mime >/dev/null 2>&1; then
    DEF_GIF=$(xdg-mime query default image/gif 2>/dev/null || echo "ghostview.desktop")
    DEF_PNG=$(xdg-mime query default image/png 2>/dev/null || echo "ghostview.desktop")
    DEF_JPG=$(xdg-mime query default image/jpeg 2>/dev/null || echo "ghostview.desktop")
    echo "  - GIF Varsayılanı : $DEF_GIF"
    echo "  - PNG Varsayılanı : $DEF_PNG"
    echo "  - JPG Varsayılanı : $DEF_JPG"
fi

echo "==================================================================="
echo "  ✓ İŞLEM BAŞARIYLA TAMAMLANDI!"
echo ""
echo "  Artık Linux masaüstünüzde (GNOME, KDE, XFCE vb.) dosya yöneticisinden"
echo "  GIF, PNG, JPG, WEBP, BMP dahil herhangi bir resme çift tıkladığınızda"
echo "  otomatik olarak GhostView açılacaktır!"
echo ""
echo "  Terminalden çalıştırmak için: ghostview <resim_yolu>"
echo "==================================================================="
