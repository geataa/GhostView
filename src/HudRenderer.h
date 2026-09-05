#pragma once
#include <windows.h>
#include <d2d1_2.h>
#include <dwrite.h>
#include <string>
#include <vector>

enum class AspectMode {
    Fit,      // En-boy oranı koru, ekrana sığdır
    Fill,     // En-boy oranı koru, ekranı tamamen doldur (kırp)
    Stretch,  // Ekrana tam yay (en-boy esnet)
    Original  // %100 gerçek piksel (1:1)
};

enum class HudAction {
    None,
    Prev,
    Next,
    ZoomOut,
    ZoomIn,
    ActualSize,
    CycleAspectMode,
    RotateLeft,
    RotateRight,
    Crop,
    MagicErase,
    AutoBgRemove,
    Undo,
    SaveAs,
    ToggleLanguage,
    ToggleFullscreen,
    Close
};

struct HudButton {
    HudAction action = HudAction::None;
    D2D1_RECT_F rect = {};
    std::wstring label;
    std::wstring tooltip;
    bool isHovered = false;
    bool isPressed = false;
    bool isActive = false;
};

class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    bool Initialize(ID2D1DeviceContext* d2dContext, float dpiScale = 1.0f);
    void SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale);
    void SetFullscreenState(bool isFullscreen) { m_isFullscreen = isFullscreen; }
    void SetAspectMode(AspectMode mode) { m_aspectMode = mode; m_needsRedraw = true; }
    void SetCropActive(bool active) { m_isCropActive = active; m_needsRedraw = true; }
    void SetEraseActive(bool active) { m_isEraseActive = active; m_needsRedraw = true; }
    void SetCanUndo(bool canUndo) { m_canUndo = canUndo; m_needsRedraw = true; }
    void ShowToast(const std::wstring& message, float duration = 2.5f);

    void Update(float deltaTimeSeconds);
    void Render(
        ID2D1DeviceContext* d2dContext,
        float screenWidth,
        float screenHeight,
        const std::wstring& imageInfo
    );

    void OnMouseMove(float mouseX, float mouseY);
    bool OnMouseDown(float mouseX, float mouseY);
    HudAction OnMouseUp(float mouseX, float mouseY);

    bool IsMouseOverHud(float mouseX, float mouseY) const;
    void ResetIdleTimer();
    bool NeedsRedraw() const { return m_needsRedraw; }
    void ClearNeedsRedraw() { m_needsRedraw = false; }
    bool IsAnimating() const { return (m_alpha > 0.0f && m_alpha < 1.0f) || (m_idleSeconds > 2.5f && m_alpha > 0.0f) || (m_toastTimer > 0.0f); }
    float GetHudTop() const { return m_hudRect.top; }
    float GetAlpha() const { return m_alpha; }

private:
    IDWriteFactory* m_dwriteFactory = nullptr;
    IDWriteTextFormat* m_textFormatText = nullptr;
    IDWriteTextFormat* m_textFormatIcons = nullptr;
    IDWriteTextFormat* m_textFormatSmall = nullptr;
    IDWriteTextFormat* m_textFormatTop = nullptr;

    ID2D1SolidColorBrush* m_bgBrush = nullptr;
    ID2D1SolidColorBrush* m_borderBrush = nullptr;
    ID2D1SolidColorBrush* m_textBrush = nullptr;
    ID2D1SolidColorBrush* m_hoverBrush = nullptr;
    ID2D1SolidColorBrush* m_pressBrush = nullptr;
    ID2D1SolidColorBrush* m_dimTextBrush = nullptr;
    ID2D1SolidColorBrush* m_activeBrush = nullptr;
    ID2D1SolidColorBrush* m_activeBorderBrush = nullptr;

    std::vector<HudButton> m_buttons;
    HudButton m_topCloseButton;
    HudButton m_topFullscreenButton;

    D2D1_RECT_F m_hudRect = {};
    float m_idleSeconds = 0.0f;
    float m_alpha = 1.0f;
    bool m_needsRedraw = true;
    bool m_isFullscreen = true;
    AspectMode m_aspectMode = AspectMode::Fit;
    float m_dpiScale = 1.0f;

    bool m_isCropActive = false;
    bool m_isEraseActive = false;
    bool m_canUndo = false;
    std::wstring m_toastMessage;
    float m_toastTimer = 0.0f;

    void CreateResources(ID2D1DeviceContext* d2dContext);
    void CreateTextFormats();
    void LayoutButtons(float screenWidth, float screenHeight);
};
