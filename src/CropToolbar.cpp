#include "CropToolbar.h"
#include "Localization.h"
#include <algorithm>
#include <cmath>

CropToolbar::CropToolbar() = default;

CropToolbar::~CropToolbar() {
    if (m_bgBrush) m_bgBrush->Release();
    if (m_borderBrush) m_borderBrush->Release();
    if (m_dividerBrush) m_dividerBrush->Release();
    if (m_textBrush) m_textBrush->Release();
    if (m_hoverBrush) m_hoverBrush->Release();
    if (m_pressBrush) m_pressBrush->Release();
    if (m_activeBrush) m_activeBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();
    if (m_accentBrush) m_accentBrush->Release();
    if (m_cancelBrush) m_cancelBrush->Release();

    if (m_textFormatMain) m_textFormatMain->Release();
    if (m_textFormatIcons) m_textFormatIcons->Release();
    if (m_textFormatTooltip) m_textFormatTooltip->Release();
    if (m_dwriteFactory) m_dwriteFactory->Release();
}

bool CropToolbar::Initialize(ID2D1DeviceContext* d2dContext, float dpiScale) {
    m_dpiScale = (dpiScale > 0.5f) ? dpiScale : 1.0f;

    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_dwriteFactory)
    );
    if (FAILED(hr)) return false;

    CreateTextFormats();
    CreateResources(d2dContext);
    return true;
}

void CropToolbar::SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale) {
    if (dpiScale <= 0.5f) dpiScale = 1.0f;
    if (std::abs(m_dpiScale - dpiScale) > 0.01f) {
        m_dpiScale = dpiScale;
        CreateTextFormats();
        CreateResources(d2dContext);
        m_needsRedraw = true;
    }
}

void CropToolbar::CreateTextFormats() {
    if (!m_dwriteFactory) return;

    if (m_textFormatMain) { m_textFormatMain->Release(); m_textFormatMain = nullptr; }
    if (m_textFormatIcons) { m_textFormatIcons->Release(); m_textFormatIcons = nullptr; }
    if (m_textFormatTooltip) { m_textFormatTooltip->Release(); m_textFormatTooltip = nullptr; }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        13.0f * m_dpiScale,
        L"tr-TR",
        &m_textFormatMain
    );
    if (m_textFormatMain) {
        m_textFormatMain->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatMain->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Symbol",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.5f * m_dpiScale,
        L"tr-TR",
        &m_textFormatIcons
    );
    if (m_textFormatIcons) {
        m_textFormatIcons->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatIcons->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_MEDIUM,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        11.5f * m_dpiScale,
        L"tr-TR",
        &m_textFormatTooltip
    );
    if (m_textFormatTooltip) {
        m_textFormatTooltip->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatTooltip->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void CropToolbar::CreateResources(ID2D1DeviceContext* d2dContext) {
    if (!d2dContext) return;

    if (m_bgBrush) m_bgBrush->Release();
    if (m_borderBrush) m_borderBrush->Release();
    if (m_dividerBrush) m_dividerBrush->Release();
    if (m_textBrush) m_textBrush->Release();
    if (m_hoverBrush) m_hoverBrush->Release();
    if (m_pressBrush) m_pressBrush->Release();
    if (m_activeBrush) m_activeBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();
    if (m_accentBrush) m_accentBrush->Release();
    if (m_cancelBrush) m_cancelBrush->Release();

    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.08f, 0.11f, 0.94f), &m_bgBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.40f, 0.44f, 0.52f, 0.45f), &m_borderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.38f, 0.45f, 0.55f), &m_dividerBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.96f, 0.98f, 1.00f), &m_textBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.00f, 1.00f, 1.00f, 0.18f), &m_hoverBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.65f, 1.00f, 0.40f), &m_pressBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.55f, 0.95f, 0.40f), &m_activeBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.85f, 1.00f, 0.90f), &m_activeBorderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.70f, 0.40f, 0.50f), &m_accentBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.25f, 0.25f, 0.45f), &m_cancelBrush);
}

void CropToolbar::SetCropRatio(CropRatio ratio) {
    m_activeRatio = ratio;
    m_needsRedraw = true;
}

void CropToolbar::SetSymmetric(bool symmetric) {
    m_isSymmetric = symmetric;
    m_needsRedraw = true;
}

void CropToolbar::LayoutButtons(float screenWidth) {
    struct BtnTemplate {
        CropAction action;
        CropRatio ratio;
        std::wstring label;
        std::wstring tooltip;
        float baseWidth;
        bool isAccent = false;
        bool isCancel = false;
        bool isIcon = false;
    };

    BtnTemplate defsRatio[] = {
        { CropAction::SetRatio, CropRatio::Free,     Localization::Get(StringId::CropRatioFree),     Localization::Get(StringId::CropRatioFree),     62.0f },
        { CropAction::SetRatio, CropRatio::Original, Localization::Get(StringId::CropRatioOriginal), Localization::Get(StringId::CropRatioOriginal), 66.0f },
        { CropAction::SetRatio, CropRatio::Ratio1x1,  Localization::Get(StringId::CropRatio1x1),      L"1:1 (Kare / Square)",                         42.0f },
        { CropAction::SetRatio, CropRatio::Ratio16x9, Localization::Get(StringId::CropRatio16x9),     L"16:9 (Geni\x015F / Widescreen)",             50.0f },
        { CropAction::SetRatio, CropRatio::Ratio9x16, Localization::Get(StringId::CropRatio9x16),     L"9:16 (Dikey / Story)",                        50.0f },
        { CropAction::SetRatio, CropRatio::Ratio4x3,  Localization::Get(StringId::CropRatio4x3),      L"4:3 (Standart / Normal)",                     46.0f },
        { CropAction::SetRatio, CropRatio::Ratio3x2,  Localization::Get(StringId::CropRatio3x2),      L"3:2 (Foto\x011Fraf / DSLR)",                  46.0f }
    };

    BtnTemplate defsSym[] = {
        { CropAction::ToggleSymmetric, CropRatio::Free, Localization::Get(StringId::CropSymmetric), Localization::Get(StringId::CropSymmetric), 78.0f }
    };

    BtnTemplate defsAct[] = {
        { CropAction::Reset,  CropRatio::Free, L"\x21BA", Localization::Get(StringId::CropReset),  38.0f, false, false, true },
        { CropAction::Apply,  CropRatio::Free, L"\x2714", Localization::Get(StringId::CropApply),  44.0f, true,  false, true },
        { CropAction::Cancel, CropRatio::Free, L"\x2715", Localization::Get(StringId::CropCancel), 38.0f, false, true,  true }
    };

    size_t countRatio = sizeof(defsRatio) / sizeof(defsRatio[0]);
    size_t countSym = sizeof(defsSym) / sizeof(defsSym[0]);
    size_t countAct = sizeof(defsAct) / sizeof(defsAct[0]);
    size_t totalCount = countRatio + countSym + countAct;

    if (m_buttons.size() != totalCount) {
        m_buttons.resize(totalCount);
    }

    float padX = 10.0f * m_dpiScale;
    float gap = 3.0f * m_dpiScale;
    float divGap = 8.0f * m_dpiScale;
    float divWidth = 1.0f;

    float totalW = padX * 2.0f;
    for (size_t i = 0; i < countRatio; ++i) totalW += defsRatio[i].baseWidth * m_dpiScale + (i > 0 ? gap : 0.0f);
    totalW += divGap * 2.0f + divWidth;
    for (size_t i = 0; i < countSym; ++i) totalW += defsSym[i].baseWidth * m_dpiScale + (i > 0 ? gap : 0.0f);
    totalW += divGap * 2.0f + divWidth;
    for (size_t i = 0; i < countAct; ++i) totalW += defsAct[i].baseWidth * m_dpiScale + (i > 0 ? gap : 0.0f);

    float barH = 42.0f * m_dpiScale;
    float barTop = 18.0f * m_dpiScale;
    float barLeft = (screenWidth - totalW) / 2.0f;
    if (barLeft < 10.0f) barLeft = 10.0f;
    float barRight = barLeft + totalW;
    float barBottom = barTop + barH;

    m_barRect = D2D1::RectF(barLeft, barTop, barRight, barBottom);

    float curX = barLeft + padX;
    float btnH = 32.0f * m_dpiScale;
    float btnY = barTop + (barH - btnH) / 2.0f;

    size_t outIdx = 0;

    // 1. Ratio buttons
    for (size_t i = 0; i < countRatio; ++i, ++outIdx) {
        float w = defsRatio[i].baseWidth * m_dpiScale;
        m_buttons[outIdx].action = defsRatio[i].action;
        m_buttons[outIdx].ratio = defsRatio[i].ratio;
        m_buttons[outIdx].label = defsRatio[i].label;
        m_buttons[outIdx].tooltip = defsRatio[i].tooltip;
        m_buttons[outIdx].rect = D2D1::RectF(curX, btnY, curX + w, btnY + btnH);
        m_buttons[outIdx].isActive = (defsRatio[i].ratio == m_activeRatio);
        m_buttons[outIdx].isAccent = false;
        m_buttons[outIdx].isCancel = false;
        curX += w + gap;
    }

    curX -= gap;
    curX += divGap;
    m_divider1X = curX;
    curX += divWidth + divGap;

    // 2. Symmetric button
    for (size_t i = 0; i < countSym; ++i, ++outIdx) {
        float w = defsSym[i].baseWidth * m_dpiScale;
        m_buttons[outIdx].action = defsSym[i].action;
        m_buttons[outIdx].ratio = defsSym[i].ratio;
        m_buttons[outIdx].label = defsSym[i].label;
        m_buttons[outIdx].tooltip = defsSym[i].tooltip;
        m_buttons[outIdx].rect = D2D1::RectF(curX, btnY, curX + w, btnY + btnH);
        m_buttons[outIdx].isActive = m_isSymmetric;
        m_buttons[outIdx].isAccent = false;
        m_buttons[outIdx].isCancel = false;
        curX += w + gap;
    }

    curX -= gap;
    curX += divGap;
    m_divider2X = curX;
    curX += divWidth + divGap;

    // 3. Action buttons
    for (size_t i = 0; i < countAct; ++i, ++outIdx) {
        float w = defsAct[i].baseWidth * m_dpiScale;
        m_buttons[outIdx].action = defsAct[i].action;
        m_buttons[outIdx].ratio = defsAct[i].ratio;
        m_buttons[outIdx].label = defsAct[i].label;
        m_buttons[outIdx].tooltip = defsAct[i].tooltip;
        m_buttons[outIdx].rect = D2D1::RectF(curX, btnY, curX + w, btnY + btnH);
        m_buttons[outIdx].isActive = false;
        m_buttons[outIdx].isAccent = defsAct[i].isAccent;
        m_buttons[outIdx].isCancel = defsAct[i].isCancel;
        curX += w + gap;
    }
}

void CropToolbar::Render(
    ID2D1DeviceContext* d2dContext,
    float screenWidth,
    float screenHeight
) {
    if (!d2dContext) return;

    LayoutButtons(screenWidth);

    // Draw floating glass container
    float radius = 21.0f * m_dpiScale;
    D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(m_barRect, radius, radius);
    d2dContext->FillRoundedRectangle(&rrect, m_bgBrush);
    d2dContext->DrawRoundedRectangle(&rrect, m_borderBrush, 1.2f);

    // Draw dividers
    float divTop = m_barRect.top + 8.0f * m_dpiScale;
    float divBot = m_barRect.bottom - 8.0f * m_dpiScale;
    if (m_dividerBrush) {
        d2dContext->DrawLine(D2D1::Point2F(m_divider1X, divTop), D2D1::Point2F(m_divider1X, divBot), m_dividerBrush, 1.0f);
        d2dContext->DrawLine(D2D1::Point2F(m_divider2X, divTop), D2D1::Point2F(m_divider2X, divBot), m_dividerBrush, 1.0f);
    }

    // Draw Buttons
    float btnRadius = 8.0f * m_dpiScale;
    CropBtn* hoveredBtn = nullptr;

    for (auto& btn : m_buttons) {
        D2D1_ROUNDED_RECT brrect = D2D1::RoundedRect(btn.rect, btnRadius, btnRadius);

        if (btn.isActive) {
            d2dContext->FillRoundedRectangle(&brrect, m_activeBrush);
            d2dContext->DrawRoundedRectangle(&brrect, m_activeBorderBrush, 1.5f);
        } else if (btn.isPressed) {
            d2dContext->FillRoundedRectangle(&brrect, m_pressBrush);
        } else if (btn.isHovered) {
            if (btn.isAccent) {
                d2dContext->FillRoundedRectangle(&brrect, m_accentBrush);
            } else if (btn.isCancel) {
                d2dContext->FillRoundedRectangle(&brrect, m_cancelBrush);
            } else {
                d2dContext->FillRoundedRectangle(&brrect, m_hoverBrush);
            }
            hoveredBtn = &btn;
        } else if (btn.isAccent) {
            // Subtle highlight for apply button
            D2D1_ROUNDED_RECT accR = D2D1::RoundedRect(btn.rect, btnRadius, btnRadius);
            d2dContext->DrawRoundedRectangle(&accR, m_accentBrush, 1.0f);
        }

        // Draw label
        bool isSymbol = (btn.action == CropAction::Apply || btn.action == CropAction::Cancel || btn.action == CropAction::Reset);
        IDWriteTextFormat* fmt = isSymbol ? m_textFormatIcons : m_textFormatMain;

        d2dContext->DrawText(
            btn.label.c_str(),
            static_cast<UINT32>(btn.label.length()),
            fmt,
            btn.rect,
            m_textBrush
        );
    }

    // Draw tooltip if hovering
    if (hoveredBtn && !hoveredBtn->tooltip.empty() && m_textFormatTooltip) {
        float tipW = static_cast<float>(hoveredBtn->tooltip.length() * 7.5f * m_dpiScale + 18.0f * m_dpiScale);
        if (tipW < 60.0f * m_dpiScale) tipW = 60.0f * m_dpiScale;
        float tipH = 22.0f * m_dpiScale;
        float tipCenterX = (hoveredBtn->rect.left + hoveredBtn->rect.right) / 2.0f;
        float tipLeft = tipCenterX - tipW / 2.0f;
        float tipRight = tipLeft + tipW;
        float tipTop = m_barRect.bottom + 6.0f * m_dpiScale;
        float tipBottom = tipTop + tipH;

        D2D1_RECT_F tipRect = D2D1::RectF(tipLeft, tipTop, tipRight, tipBottom);
        D2D1_ROUNDED_RECT tipRRect = D2D1::RoundedRect(tipRect, 6.0f * m_dpiScale, 6.0f * m_dpiScale);

        d2dContext->FillRoundedRectangle(&tipRRect, m_bgBrush);
        d2dContext->DrawRoundedRectangle(&tipRRect, m_borderBrush, 1.0f);
        d2dContext->DrawText(
            hoveredBtn->tooltip.c_str(),
            static_cast<UINT32>(hoveredBtn->tooltip.length()),
            m_textFormatTooltip,
            tipRect,
            m_textBrush
        );
    }
}

bool CropToolbar::IsMouseOver(float mouseX, float mouseY) const {
    return (mouseX >= m_barRect.left && mouseX <= m_barRect.right &&
            mouseY >= m_barRect.top && mouseY <= m_barRect.bottom + 30.0f * m_dpiScale);
}

void CropToolbar::OnMouseMove(float mouseX, float mouseY) {
    bool changed = false;
    for (auto& btn : m_buttons) {
        bool hover = (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
                      mouseY >= btn.rect.top && mouseY <= btn.rect.bottom);
        if (btn.isHovered != hover) {
            btn.isHovered = hover;
            changed = true;
        }
    }
    if (changed) m_needsRedraw = true;
}

bool CropToolbar::OnMouseDown(float mouseX, float mouseY) {
    bool handled = false;
    for (auto& btn : m_buttons) {
        if (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
            mouseY >= btn.rect.top && mouseY <= btn.rect.bottom) {
            btn.isPressed = true;
            handled = true;
            m_needsRedraw = true;
        }
    }
    return handled;
}

CropAction CropToolbar::OnMouseUp(float mouseX, float mouseY, CropRatio* outRatio) {
    CropAction result = CropAction::None;

    for (auto& btn : m_buttons) {
        if (btn.isPressed) {
            btn.isPressed = false;
            m_needsRedraw = true;
            if (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
                mouseY >= btn.rect.top && mouseY <= btn.rect.bottom) {
                result = btn.action;
                if (outRatio) {
                    *outRatio = btn.ratio;
                }
            }
        }
    }

    return result;
}
