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
