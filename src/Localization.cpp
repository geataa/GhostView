#include "Localization.h"

Language Localization::s_currentLang = Language::English;

Language Localization::DetectSystemLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(langId) == LANG_TURKISH) {
        return Language::Turkish;
    }
    return Language::English;
}

Language Localization::GetCurrentLanguage() {
    return s_currentLang;
}

void Localization::SetLanguage(Language lang) {
    s_currentLang = lang;
}

Language Localization::ToggleLanguage() {
    s_currentLang = (s_currentLang == Language::Turkish) ? Language::English : Language::Turkish;
    return s_currentLang;
}

const wchar_t* Localization::GetLanguageCode() {
    return (s_currentLang == Language::Turkish) ? L"TR" : L"EN";
}

const wchar_t* Localization::GetLanguageName() {
    return (s_currentLang == Language::Turkish) ? L"Türkçe" : L"English";
}

const wchar_t* Localization::Get(StringId id) {
    bool tr = (s_currentLang == Language::Turkish);
    switch (id) {
    case StringId::AspectFit:
        return tr ? L"Fit" : L"Fit";
    case StringId::AspectFill:
        return tr ? L"Doldur" : L"Fill";
    case StringId::AspectStretch:
        return tr ? L"Yay" : L"Stretch";
    case StringId::AspectOriginal:
        return tr ? L"1:1" : L"1:1";

    case StringId::TooltipPrev:
        return tr ? L"Önceki Resim (Sol Ok)" : L"Previous Image (Left Arrow)";
    case StringId::TooltipNext:
        return tr ? L"Sonraki Resim (Sağ Ok)" : L"Next Image (Right Arrow)";
    case StringId::TooltipZoomOut:
        return tr ? L"Uzaklaştır (-)" : L"Zoom Out (-)";
    case StringId::TooltipZoomIn:
        return tr ? L"Yakınlaştır (+)" : L"Zoom In (+)";
    case StringId::TooltipAspectFit:
        return tr ? L"Ekrana Sığdır (Fit)" : L"Fit to Screen (Fit)";
    case StringId::TooltipAspectFill:
        return tr ? L"Ekranı Doldur (Fill)" : L"Fill Screen (Fill)";
    case StringId::TooltipAspectStretch:
        return tr ? L"Ekrana Yay (Stretch)" : L"Stretch to Screen";
    case StringId::TooltipAspectOriginal:
        return tr ? L"Gerçek Boyut (%100) (1)" : L"Actual Size (100%) (1)";
    case StringId::TooltipActualSize:
        return tr ? L"Gerçek Boyut (%100) (1)" : L"Actual Size (100%) (1)";
    case StringId::TooltipRotateLeft:
        return tr ? L"Sola Döndür (L)" : L"Rotate Left (L)";
    case StringId::TooltipRotateRight:
        return tr ? L"Sağa Döndür (R)" : L"Rotate Right (R)";
    case StringId::TooltipCrop:
        return tr ? L"Kırpma Modu (C)" : L"Crop Tool (C)";
    case StringId::TooltipMagicErase:
        return tr ? L"Sihirli Silgi / Renk Silici (E)" : L"Magic Eraser (E)";
    case StringId::TooltipUndo:
        return tr ? L"Geri Al (Ctrl+Z)" : L"Undo (Ctrl+Z)";
    case StringId::TooltipSaveAs:
        return tr ? L"Farklı Kaydet (Ctrl+S)" : L"Save As (Ctrl+S)";
    case StringId::TooltipLanguage:
        return tr ? L"Dili Değiştir (TR / EN) (T)" : L"Change Language (EN / TR) (T)";
    case StringId::TooltipFullscreen:
        return tr ? L"Tam Ekran / Küçük Ekran (F11 / Enter)" : L"Fullscreen / Windowed (F11 / Enter)";
    case StringId::TooltipClose:
        return tr ? L"Kapat (ESC)" : L"Close (ESC)";

    case StringId::ToastCropMode:
        return tr ? L"Kırpma: Enter = Onayla | ESC = İptal" : L"Crop: Enter = Apply | ESC = Cancel";
    case StringId::ToastImageCropped:
        return tr ? L"Resim Kırpıldı (\x2702)" : L"Image Cropped (\x2702)";
    case StringId::ToastEraseMode:
        return tr ? L"Sihirli Silgi: Tıklanan rengi siler (E)" : L"Magic Eraser: Click a color to erase (E)";
    case StringId::ToastAreaErased:
        return tr ? L"Bölge Silindi (\x2728 Şeffaf)" : L"Area Erased (\x2728 Transparent)";
    case StringId::ToastBgRemoved:
        return tr ? L"Arka Plan Temizlendi (\x2728)" : L"Background Removed (\x2728)";
    case StringId::ToastSavedSuccess:
        return tr ? L"Dosya Başarıyla Kaydedildi! (\x2714)" : L"File Saved Successfully! (\x2714)";
    case StringId::ToastSavedError:
        return tr ? L"Hata: Dosya Kaydedilemedi!" : L"Error: Could not save file!";
    case StringId::ToastNoUndo:
        return tr ? L"Geri alınacak işlem yok!" : L"Nothing to undo!";
    case StringId::ToastUndone:
        return tr ? L"Geri Alındı (\x21A9)" : L"Undone (\x21A9)";
    case StringId::ToastLangSwitched:
        return tr ? L"Dil: Türkçe" : L"Language: English";
    case StringId::ToastNoImage:
        return tr ? L"İşlem yapılacak resim yok!" : L"No image loaded!";

    case StringId::CropRatioFree:
        return tr ? L"Serbest" : L"Free";
    case StringId::CropRatioOriginal:
        return tr ? L"Orijinal" : L"Original";
    case StringId::CropRatio1x1:
        return L"1:1";
    case StringId::CropRatio16x9:
        return L"16:9";
    case StringId::CropRatio9x16:
        return L"9:16";
    case StringId::CropRatio4x3:
        return L"4:3";
    case StringId::CropRatio3x2:
        return L"3:2";
    case StringId::CropSymmetric:
        return tr ? L"Simetrik" : L"Symmetric";
    case StringId::CropReset:
        return tr ? L"Sıfırla" : L"Reset";
    case StringId::CropApply:
        return tr ? L"Onayla (Enter)" : L"Apply (Enter)";
    case StringId::CropCancel:
        return tr ? L"İptal (ESC)" : L"Cancel (ESC)";
    case StringId::ToastCropSymmetricOn:
        return tr ? L"Simetrik Boyutlandırma: Açık (Alt)" : L"Symmetric Sizing: ON (Alt)";
    case StringId::ToastCropSymmetricOff:
        return tr ? L"Simetrik Boyutlandırma: Kapalı" : L"Symmetric Sizing: OFF";
    case StringId::ToastCropReset:
        return tr ? L"Kırpma Alanı Sıfırlandı" : L"Crop Box Reset";
    case StringId::ToastCropRatioSet:
        return tr ? L"Kırpma Oranı Ayarlandı ve Ortalandı" : L"Crop Ratio Set & Centered";

    case StringId::DialogSaveTitle:
        return tr ? L"Resmi Farklı Kaydet (GhostView)" : L"Save Image As (GhostView)";
    case StringId::DialogSaveFilter:
        return tr
            ? L"PNG Resim (*.png)\0*.png\0JPEG Resim (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0Tüm Dosyalar (*.*)\0*.*\0"
            : L"PNG Image (*.png)\0*.png\0JPEG Image (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0All Files (*.*)\0*.*\0";
    case StringId::DialogOpenTitle:
        return tr ? L"Resim Seç (GhostView)" : L"Select Image (GhostView)";
    case StringId::DialogOpenFilter:
        return tr
            ? L"Resim ve Animasyon Dosyaları\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp;*.ico;*.tif;*.tiff;*.jfif\0Tüm Dosyalar\0*.*\0"
            : L"Image & Animation Files\0*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.webp;*.ico;*.tif;*.tiff;*.jfif\0All Files\0*.*\0";
    case StringId::ImageLoadError:
        return tr ? L"Resim yüklenemedi: " : L"Failed to load image: ";
    default:
        return L"";
    }
}
