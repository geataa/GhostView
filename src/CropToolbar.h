#pragma once
#include <windows.h>
#include <d2d1_2.h>
#include <dwrite.h>
#include <string>
#include <vector>

enum class CropRatio {
    Free,
    Original,
    Ratio1x1,
    Ratio16x9,
    Ratio9x16,
    Ratio4x3,
    Ratio3x2
};

enum class CropAction {
    None,
    SetRatio,
    ToggleSymmetric,
    Reset,
    Apply,
    Cancel
};

struct CropBtn {
    CropAction action = CropAction::None;
    CropRatio ratio = CropRatio::Free;
    D2D1_RECT_F rect = {};
    std::wstring label;
    std::wstring tooltip;
    bool isHovered = false;
    bool isPressed = false;
    bool isActive = false;
    bool isAccent = false;
    bool isCancel = false;
};

class CropToolbar {
public:
    CropToolbar();
    ~CropToolbar();

    bool Initialize(ID2D1DeviceContext* d2dContext, float dpiScale = 1.0f);
    void SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale);

    void SetCropRatio(CropRatio ratio);
    CropRatio GetCropRatio() const { return m_activeRatio; }

    void SetSymmetric(bool symmetric);
    bool IsSymmetric() const { return m_isSymmetric; }

    void Render(
        ID2D1DeviceContext* d2dContext,
        float screenWidth,
        float screenHeight
    );

    void OnMouseMove(float mouseX, float mouseY);
    bool OnMouseDown(float mouseX, float mouseY);
    CropAction OnMouseUp(float mouseX, float mouseY, CropRatio* outRatio = nullptr);

    bool IsMouseOver(float mouseX, float mouseY) const;
    bool NeedsRedraw() const { return m_needsRedraw; }
    void ClearNeedsRedraw() { m_needsRedraw = false; }

private:
    void CreateResources(ID2D1DeviceContext* d2dContext);
    void CreateTextFormats();
    void LayoutButtons(float screenWidth);

    IDWriteFactory* m_dwriteFactory = nullptr;
    IDWriteTextFormat* m_textFormatMain = nullptr;
    IDWriteTextFormat* m_textFormatIcons = nullptr;
    IDWriteTextFormat* m_textFormatTooltip = nullptr;

    ID2D1SolidColorBrush* m_bgBrush = nullptr;
    ID2D1SolidColorBrush* m_borderBrush = nullptr;
    ID2D1SolidColorBrush* m_dividerBrush = nullptr;
    ID2D1SolidColorBrush* m_textBrush = nullptr;
    ID2D1SolidColorBrush* m_hoverBrush = nullptr;
    ID2D1SolidColorBrush* m_pressBrush = nullptr;
    ID2D1SolidColorBrush* m_activeBrush = nullptr;
    ID2D1SolidColorBrush* m_activeBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_accentBrush = nullptr;
    ID2D1SolidColorBrush* m_cancelBrush = nullptr;

    std::vector<CropBtn> m_buttons;
    D2D1_RECT_F m_barRect = {};
    float m_divider1X = 0.0f;
    float m_divider2X = 0.0f;

    CropRatio m_activeRatio = CropRatio::Free;
    bool m_isSymmetric = false;
    float m_dpiScale = 1.0f;
    bool m_needsRedraw = true;
    std::wstring m_hoverTooltip;
};
