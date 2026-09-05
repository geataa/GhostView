#pragma once
#include <windows.h>
#include <string>

enum class Language {
    Turkish,
    English
};

enum class StringId {
    AspectFit,
    AspectFill,
    AspectStretch,
    AspectOriginal,

    TooltipPrev,
    TooltipNext,
    TooltipZoomOut,
    TooltipZoomIn,
    TooltipAspectFit,
    TooltipAspectFill,
    TooltipAspectStretch,
    TooltipAspectOriginal,
    TooltipActualSize,
    TooltipRotateLeft,
    TooltipRotateRight,
    TooltipCrop,
    TooltipMagicErase,
    TooltipUndo,
    TooltipSaveAs,
    TooltipLanguage,
    TooltipFullscreen,
    TooltipClose,

    ToastCropMode,
    ToastImageCropped,
    ToastEraseMode,
    ToastAreaErased,
    ToastBgRemoved,
    ToastSavedSuccess,
    ToastSavedError,
    ToastNoUndo,
    ToastUndone,
    ToastLangSwitched,
    ToastNoImage,

    // Crop Toolbar
    CropRatioFree,
    CropRatioOriginal,
    CropRatio1x1,
    CropRatio16x9,
    CropRatio9x16,
    CropRatio4x3,
    CropRatio3x2,
    CropSymmetric,
    CropReset,
    CropApply,
    CropCancel,
    ToastCropSymmetricOn,
    ToastCropSymmetricOff,
    ToastCropReset,
    ToastCropRatioSet,

    DialogSaveTitle,
    DialogSaveFilter,
    DialogOpenTitle,
    DialogOpenFilter,
    ImageLoadError
};

class Localization {
public:
    static Language DetectSystemLanguage();
    static Language GetCurrentLanguage();
    static void SetLanguage(Language lang);
    static Language ToggleLanguage();
    static const wchar_t* Get(StringId id);
    static const wchar_t* GetLanguageCode(); // "TR" or "EN"
    static const wchar_t* GetLanguageName(); // "Türkçe" or "English"

private:
    static Language s_currentLang;
};
