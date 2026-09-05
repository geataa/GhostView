#include "HudRenderer.h"
#include "Localization.h"
#include <algorithm>

#pragma comment(lib, "dwrite.lib")

HudRenderer::HudRenderer() = default;

HudRenderer::~HudRenderer() {
    if (m_bgBrush) m_bgBrush->Release();
    if (m_borderBrush) m_borderBrush->Release();
    if (m_textBrush) m_textBrush->Release();
    if (m_hoverBrush) m_hoverBrush->Release();
    if (m_pressBrush) m_pressBrush->Release();
    if (m_dimTextBrush) m_dimTextBrush->Release();
    if (m_activeBrush) m_activeBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();

    if (m_textFormatText) m_textFormatText->Release();
    if (m_textFormatIcons) m_textFormatIcons->Release();
    if (m_textFormatSmall) m_textFormatSmall->Release();
    if (m_textFormatTop) m_textFormatTop->Release();
    if (m_dwriteFactory) m_dwriteFactory->Release();
}

bool HudRenderer::Initialize(ID2D1DeviceContext* d2dContext, float dpiScale) {
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

void HudRenderer::SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale) {
    if (dpiScale <= 0.5f) dpiScale = 1.0f;
    if (std::abs(m_dpiScale - dpiScale) > 0.01f) {
        m_dpiScale = dpiScale;
        CreateTextFormats();
        CreateResources(d2dContext);
        m_needsRedraw = true;
    }
}

void HudRenderer::CreateTextFormats() {
    if (!m_dwriteFactory) return;

    if (m_textFormatText) { m_textFormatText->Release(); m_textFormatText = nullptr; }
    if (m_textFormatIcons) { m_textFormatIcons->Release(); m_textFormatIcons = nullptr; }
    if (m_textFormatSmall) { m_textFormatSmall->Release(); m_textFormatSmall = nullptr; }
    if (m_textFormatTop) { m_textFormatTop->Release(); m_textFormatTop = nullptr; }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.5f * m_dpiScale,
        L"tr-TR",
        &m_textFormatText
    );
    if (m_textFormatText) {
        m_textFormatText->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatText->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Symbol",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        21.0f * m_dpiScale,
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
        13.5f * m_dpiScale,
        L"tr-TR",
        &m_textFormatSmall
    );
    if (m_textFormatSmall) {
        m_textFormatSmall->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatSmall->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Symbol",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f * m_dpiScale,
        L"tr-TR",
        &m_textFormatTop
    );
    if (m_textFormatTop) {
        m_textFormatTop->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_textFormatTop->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void HudRenderer::CreateResources(ID2D1DeviceContext* d2dContext) {
    if (!d2dContext) return;

    if (m_bgBrush) m_bgBrush->Release();
    if (m_borderBrush) m_borderBrush->Release();
    if (m_textBrush) m_textBrush->Release();
    if (m_hoverBrush) m_hoverBrush->Release();
    if (m_pressBrush) m_pressBrush->Release();
    if (m_dimTextBrush) m_dimTextBrush->Release();
    if (m_activeBrush) m_activeBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();

    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.09f, 0.09f, 0.11f, 0.92f), &m_bgBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.45f, 0.45f, 0.50f, 0.45f), &m_borderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.96f, 0.98f, 1.00f), &m_textBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.00f, 1.00f, 1.00f, 0.22f), &m_hoverBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.65f, 1.00f, 0.45f), &m_pressBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.80f, 0.82f, 0.86f, 0.95f), &m_dimTextBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.55f, 0.95f, 0.45f), &m_activeBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.85f, 1.00f, 0.90f), &m_activeBorderBrush);
}

void HudRenderer::ShowToast(const std::wstring& message, float duration) {
    m_toastMessage = message;
    m_toastTimer = duration;
    m_needsRedraw = true;
}

void HudRenderer::ResetIdleTimer() {
    m_idleSeconds = 0.0f;
    if (m_alpha < 1.0f) {
        m_alpha = 1.0f;
        m_needsRedraw = true;
    }
}

void HudRenderer::Update(float deltaTimeSeconds) {
    if (m_toastTimer > 0.0f) {
        m_toastTimer -= deltaTimeSeconds;
        if (m_toastTimer < 0.0f) m_toastTimer = 0.0f;
        m_needsRedraw = true;
    }

    m_idleSeconds += deltaTimeSeconds;
    if (m_idleSeconds > 2.5f) {
        if (m_alpha > 0.0f) {
            m_alpha = (std::max)(0.0f, m_alpha - deltaTimeSeconds * 3.0f);
            m_needsRedraw = true;
        }
    } else {
        if (m_alpha < 1.0f) {
            m_alpha = (std::min)(1.0f, m_alpha + deltaTimeSeconds * 6.0f);
            m_needsRedraw = true;
        }
    }
}

void HudRenderer::LayoutButtons(float screenWidth, float screenHeight) {
    float barHeight = 56.0f * m_dpiScale;
    float bottomMargin = 30.0f * m_dpiScale;

    std::wstring fsIcon = m_isFullscreen ? L"\x2750" : L"\x26F6";

    std::wstring aspectLabel = Localization::Get(StringId::AspectFit);
    std::wstring aspectTooltip = Localization::Get(StringId::TooltipAspectFit);
    switch (m_aspectMode) {
    case AspectMode::Fit:
        aspectLabel = Localization::Get(StringId::AspectFit);
        aspectTooltip = Localization::Get(StringId::TooltipAspectFit);
        break;
    case AspectMode::Fill:
        aspectLabel = Localization::Get(StringId::AspectFill);
        aspectTooltip = Localization::Get(StringId::TooltipAspectFill);
        break;
    case AspectMode::Stretch:
        aspectLabel = Localization::Get(StringId::AspectStretch);
        aspectTooltip = Localization::Get(StringId::TooltipAspectStretch);
        break;
    case AspectMode::Original:
        aspectLabel = Localization::Get(StringId::AspectOriginal);
        aspectTooltip = Localization::Get(StringId::TooltipAspectOriginal);
        break;
    }

    struct BtnDef {
        HudAction action;
        std::wstring label;
        std::wstring tooltip;
        float baseWidth;
    };

    BtnDef defs[] = {
        { HudAction::Prev,             L"\x25C0",       Localization::Get(StringId::TooltipPrev), 44.0f },
        { HudAction::Next,             L"\x25B6",       Localization::Get(StringId::TooltipNext), 44.0f },
        { HudAction::ZoomOut,          L"-",            Localization::Get(StringId::TooltipZoomOut), 38.0f },
        { HudAction::ZoomIn,           L"+",            Localization::Get(StringId::TooltipZoomIn), 38.0f },
        { HudAction::CycleAspectMode,  aspectLabel,     aspectTooltip, 52.0f },
        { HudAction::ActualSize,       L"1:1",          Localization::Get(StringId::TooltipActualSize), 44.0f },
        { HudAction::RotateLeft,       L"\x21BA",       Localization::Get(StringId::TooltipRotateLeft), 40.0f },
        { HudAction::RotateRight,      L"\x21BB",       Localization::Get(StringId::TooltipRotateRight), 40.0f },
        { HudAction::Crop,             L"\x2702",       Localization::Get(StringId::TooltipCrop), 44.0f },
        { HudAction::MagicErase,       L"\x2728",       Localization::Get(StringId::TooltipMagicErase), 44.0f },
        { HudAction::Undo,             L"\x21A9",       Localization::Get(StringId::TooltipUndo), 44.0f },
        { HudAction::SaveAs,           L"\xE105",       Localization::Get(StringId::TooltipSaveAs), 44.0f },
        { HudAction::ToggleLanguage,   Localization::GetLanguageCode(), Localization::Get(StringId::TooltipLanguage), 44.0f },
        { HudAction::ToggleFullscreen, fsIcon,          Localization::Get(StringId::TooltipFullscreen), 42.0f },
        { HudAction::Close,            L"\x2715",       Localization::Get(StringId::TooltipClose), 42.0f }
    };

    size_t count = sizeof(defs) / sizeof(defs[0]);
    if (m_buttons.size() != count) {
        m_buttons.resize(count);
    }

    float totalBtnWidth = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        totalBtnWidth += defs[i].baseWidth * m_dpiScale;
    }

    float sidePadding = 14.0f * m_dpiScale;
    float barWidth = totalBtnWidth + sidePadding * 2.0f + (count - 1) * (4.0f * m_dpiScale);

    if (barWidth > screenWidth - 20.0f) {
        barWidth = screenWidth - 20.0f;
    }

    float left = (screenWidth - barWidth) / 2.0f;
    float top = screenHeight - barHeight - bottomMargin;
    float right = left + barWidth;
    float bottom = top + barHeight;

    m_hudRect = D2D1::RectF(left, top, right, bottom);

    float spacing = (barWidth - sidePadding * 2.0f - totalBtnWidth) / (count > 1 ? (count - 1) : 1);
    float curX = left + sidePadding;

    for (size_t i = 0; i < count; ++i) {
        m_buttons[i].action = defs[i].action;
        m_buttons[i].label = defs[i].label;
        m_buttons[i].tooltip = defs[i].tooltip;

        if (defs[i].action == HudAction::Crop) {
            m_buttons[i].isActive = m_isCropActive;
        } else if (defs[i].action == HudAction::MagicErase) {
            m_buttons[i].isActive = m_isEraseActive;
        } else {
            m_buttons[i].isActive = false;
        }

        float btnW = defs[i].baseWidth * m_dpiScale;
        float btnH = 44.0f * m_dpiScale;
        float btnY = top + (barHeight - btnH) / 2.0f;

        m_buttons[i].rect = D2D1::RectF(curX, btnY, curX + btnW, btnY + btnH);
        curX += btnW + spacing;
    }

    // Top-right buttons (Fullscreen & Close)
    float trSize = 42.0f * m_dpiScale;
    float trMargin = 16.0f * m_dpiScale;
    float trGap = 8.0f * m_dpiScale;

    float closeR = screenWidth - trMargin;
    float closeL = closeR - trSize;
    m_topCloseButton.action = HudAction::Close;
    m_topCloseButton.label = L"\x2715";
    m_topCloseButton.tooltip = Localization::Get(StringId::TooltipClose);
    m_topCloseButton.rect = D2D1::RectF(closeL, trMargin, closeR, trMargin + trSize);

    float fsR = closeL - trGap;
    float fsL = fsR - trSize;
    m_topFullscreenButton.action = HudAction::ToggleFullscreen;
    m_topFullscreenButton.label = fsIcon;
    m_topFullscreenButton.tooltip = Localization::Get(StringId::TooltipFullscreen);
    m_topFullscreenButton.rect = D2D1::RectF(fsL, trMargin, fsR, trMargin + trSize);
}

bool HudRenderer::IsMouseOverHud(float mouseX, float mouseY) const {
    if (m_alpha <= 0.05f) return false;

    if (mouseX >= m_hudRect.left && mouseX <= m_hudRect.right &&
        mouseY >= m_hudRect.top - 38.0f * m_dpiScale && mouseY <= m_hudRect.bottom) {
        return true;
    }

    if (mouseX >= m_topFullscreenButton.rect.left && mouseX <= m_topCloseButton.rect.right &&
        mouseY >= m_topCloseButton.rect.top && mouseY <= m_topCloseButton.rect.bottom) {
        return true;
    }

    return false;
}

void HudRenderer::OnMouseMove(float mouseX, float mouseY) {
    ResetIdleTimer();

    bool changed = false;
    for (auto& btn : m_buttons) {
        bool hover = (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
                      mouseY >= btn.rect.top && mouseY <= btn.rect.bottom);
        if (btn.isHovered != hover) {
            btn.isHovered = hover;
            changed = true;
        }
    }

    bool trCloseHover = (mouseX >= m_topCloseButton.rect.left && mouseX <= m_topCloseButton.rect.right &&
                         mouseY >= m_topCloseButton.rect.top && mouseY <= m_topCloseButton.rect.bottom);
    if (m_topCloseButton.isHovered != trCloseHover) {
        m_topCloseButton.isHovered = trCloseHover;
        changed = true;
    }

    bool trFsHover = (mouseX >= m_topFullscreenButton.rect.left && mouseX <= m_topFullscreenButton.rect.right &&
                      mouseY >= m_topFullscreenButton.rect.top && mouseY <= m_topFullscreenButton.rect.bottom);
    if (m_topFullscreenButton.isHovered != trFsHover) {
        m_topFullscreenButton.isHovered = trFsHover;
        changed = true;
    }

    if (changed) m_needsRedraw = true;
}

bool HudRenderer::OnMouseDown(float mouseX, float mouseY) {
    ResetIdleTimer();
    bool handled = false;

    for (auto& btn : m_buttons) {
        if (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
            mouseY >= btn.rect.top && mouseY <= btn.rect.bottom) {
            btn.isPressed = true;
            handled = true;
            m_needsRedraw = true;
        }
    }

    if (mouseX >= m_topCloseButton.rect.left && mouseX <= m_topCloseButton.rect.right &&
        mouseY >= m_topCloseButton.rect.top && mouseY <= m_topCloseButton.rect.bottom) {
        m_topCloseButton.isPressed = true;
        handled = true;
        m_needsRedraw = true;
    }

    if (mouseX >= m_topFullscreenButton.rect.left && mouseX <= m_topFullscreenButton.rect.right &&
        mouseY >= m_topFullscreenButton.rect.top && mouseY <= m_topFullscreenButton.rect.bottom) {
        m_topFullscreenButton.isPressed = true;
        handled = true;
        m_needsRedraw = true;
    }

    return handled;
}

HudAction HudRenderer::OnMouseUp(float mouseX, float mouseY) {
    HudAction result = HudAction::None;

    for (auto& btn : m_buttons) {
        if (btn.isPressed) {
            btn.isPressed = false;
            m_needsRedraw = true;
            if (mouseX >= btn.rect.left && mouseX <= btn.rect.right &&
                mouseY >= btn.rect.top && mouseY <= btn.rect.bottom) {
                result = btn.action;
            }
        }
    }

    if (m_topCloseButton.isPressed) {
        m_topCloseButton.isPressed = false;
        m_needsRedraw = true;
        if (mouseX >= m_topCloseButton.rect.left && mouseX <= m_topCloseButton.rect.right &&
            mouseY >= m_topCloseButton.rect.top && mouseY <= m_topCloseButton.rect.bottom) {
            result = m_topCloseButton.action;
        }
    }

    if (m_topFullscreenButton.isPressed) {
        m_topFullscreenButton.isPressed = false;
        m_needsRedraw = true;
        if (mouseX >= m_topFullscreenButton.rect.left && mouseX <= m_topFullscreenButton.rect.right &&
            mouseY >= m_topFullscreenButton.rect.top && mouseY <= m_topFullscreenButton.rect.bottom) {
            result = m_topFullscreenButton.action;
        }
    }

    return result;
}

void HudRenderer::Render(
    ID2D1DeviceContext* d2dContext,
    float screenWidth,
    float screenHeight,
    const std::wstring& imageInfo
) {
    if (!d2dContext || m_alpha <= 0.01f) return;

    LayoutButtons(screenWidth, screenHeight);

    m_bgBrush->SetOpacity(0.90f * m_alpha);
    m_borderBrush->SetOpacity(0.45f * m_alpha);
    m_textBrush->SetOpacity(1.00f * m_alpha);
    m_dimTextBrush->SetOpacity(0.95f * m_alpha);
    m_hoverBrush->SetOpacity(0.24f * m_alpha);
    m_pressBrush->SetOpacity(0.48f * m_alpha);

    // 1. Top-right buttons
    auto drawTopBtn = [&](const HudButton& btn) {
        float r = 21.0f * m_dpiScale;
        D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(btn.rect, r, r);
        d2dContext->FillRoundedRectangle(&rrect, m_bgBrush);
        d2dContext->DrawRoundedRectangle(&rrect, m_borderBrush, 1.2f);
        if (btn.isPressed) {
            d2dContext->FillRoundedRectangle(&rrect, m_pressBrush);
        } else if (btn.isHovered) {
            d2dContext->FillRoundedRectangle(&rrect, m_hoverBrush);
        }
        d2dContext->DrawText(
            btn.label.c_str(),
            static_cast<UINT32>(btn.label.length()),
            m_textFormatTop,
            btn.rect,
            m_textBrush
        );
    };

    drawTopBtn(m_topFullscreenButton);
    drawTopBtn(m_topCloseButton);

    // 2. Info label
    if (!imageInfo.empty()) {
        float infoWidth = (std::min)(screenWidth - 40.0f, 650.0f * m_dpiScale);
        float infoHeight = 28.0f * m_dpiScale;
        D2D1_RECT_F infoRect = D2D1::RectF(
            (screenWidth - infoWidth) / 2.0f,
            m_hudRect.top - infoHeight - 10.0f * m_dpiScale,
            (screenWidth + infoWidth) / 2.0f,
            m_hudRect.top - 10.0f * m_dpiScale
        );

        D2D1_ROUNDED_RECT infoPill = D2D1::RoundedRect(infoRect, 14.0f * m_dpiScale, 14.0f * m_dpiScale);
        m_bgBrush->SetOpacity(0.75f * m_alpha);
        d2dContext->FillRoundedRectangle(&infoPill, m_bgBrush);
        d2dContext->DrawRoundedRectangle(&infoPill, m_borderBrush, 1.0f);

        d2dContext->DrawText(
            imageInfo.c_str(),
            static_cast<UINT32>(imageInfo.length()),
            m_textFormatSmall,
            infoRect,
            m_dimTextBrush
        );
    }

    // 3. Bottom HUD container
    float pillRadius = (m_hudRect.bottom - m_hudRect.top) / 2.0f;
    D2D1_ROUNDED_RECT hudRRect = D2D1::RoundedRect(m_hudRect, pillRadius, pillRadius);
    m_bgBrush->SetOpacity(0.92f * m_alpha);
    d2dContext->FillRoundedRectangle(&hudRRect, m_bgBrush);
    d2dContext->DrawRoundedRectangle(&hudRRect, m_borderBrush, 1.2f);

    // 0. Toast Notification
    if (m_toastTimer > 0.0f && !m_toastMessage.empty()) {
        float toastW = 340.0f * m_dpiScale;
        float toastH = 36.0f * m_dpiScale;
        D2D1_RECT_F tRect = D2D1::RectF(
            (screenWidth - toastW) / 2.0f,
            24.0f * m_dpiScale,
            (screenWidth + toastW) / 2.0f,
            24.0f * m_dpiScale + toastH
        );
        D2D1_ROUNDED_RECT tRRect = D2D1::RoundedRect(tRect, toastH / 2.0f, toastH / 2.0f);
        m_activeBrush->SetOpacity(0.92f);
        d2dContext->FillRoundedRectangle(&tRRect, m_activeBrush);
        m_activeBorderBrush->SetOpacity(1.0f);
        d2dContext->DrawRoundedRectangle(&tRRect, m_activeBorderBrush, 1.5f);
        d2dContext->DrawText(
            m_toastMessage.c_str(),
            static_cast<UINT32>(m_toastMessage.length()),
            m_textFormatText,
            tRect,
            m_textBrush
        );
    }

    // 4. Buttons
    for (const auto& btn : m_buttons) {
        float btnRadius = 9.0f * m_dpiScale;
        D2D1_ROUNDED_RECT btnRRect = D2D1::RoundedRect(btn.rect, btnRadius, btnRadius);

        if (btn.isActive) {
            d2dContext->FillRoundedRectangle(&btnRRect, m_activeBrush);
            d2dContext->DrawRoundedRectangle(&btnRRect, m_activeBorderBrush, 1.5f);
        } else if (btn.isPressed) {
            d2dContext->FillRoundedRectangle(&btnRRect, m_pressBrush);
        } else if (btn.isHovered) {
            d2dContext->FillRoundedRectangle(&btnRRect, m_hoverBrush);
        }

        IDWriteTextFormat* tf = m_textFormatIcons;
        if (btn.action == HudAction::CycleAspectMode || btn.action == HudAction::ActualSize ||
            btn.action == HudAction::ToggleLanguage) {
            tf = m_textFormatText;
        }

        ID2D1SolidColorBrush* textBrush = m_textBrush;
        if (btn.action == HudAction::Undo && !m_canUndo) {
            textBrush = m_dimTextBrush;
        }

        d2dContext->DrawText(
            btn.label.c_str(),
            static_cast<UINT32>(btn.label.length()),
            tf,
            btn.rect,
            textBrush
        );
    }

    // 5. Tooltip for hovered bottom button
    for (const auto& btn : m_buttons) {
        if (btn.isHovered && !btn.tooltip.empty()) {
            float textLen = static_cast<float>(btn.tooltip.length());
            float tipW = (textLen * 7.5f + 24.0f) * m_dpiScale;
            float tipH = 26.0f * m_dpiScale;
            float tipCenterX = (btn.rect.left + btn.rect.right) / 2.0f;
            float tipLeft = tipCenterX - tipW / 2.0f;
            float tipRight = tipLeft + tipW;
            float tipBottom = m_hudRect.top - 8.0f * m_dpiScale;
            float tipTop = tipBottom - tipH;

            if (tipLeft < 10.0f) {
                tipRight += (10.0f - tipLeft);
                tipLeft = 10.0f;
            } else if (tipRight > screenWidth - 10.0f) {
                tipLeft -= (tipRight - (screenWidth - 10.0f));
                tipRight = screenWidth - 10.0f;
            }

            D2D1_RECT_F tipRect = D2D1::RectF(tipLeft, tipTop, tipRight, tipBottom);
            D2D1_ROUNDED_RECT tipRRect = D2D1::RoundedRect(tipRect, 6.0f * m_dpiScale, 6.0f * m_dpiScale);

            m_bgBrush->SetOpacity(0.96f * m_alpha);
            d2dContext->FillRoundedRectangle(&tipRRect, m_bgBrush);
            d2dContext->DrawRoundedRectangle(&tipRRect, m_borderBrush, 1.0f);
            d2dContext->DrawText(
                btn.tooltip.c_str(),
                static_cast<UINT32>(btn.tooltip.length()),
                m_textFormatSmall,
                tipRect,
                m_textBrush
            );
            break;
        }
    }

    // Top button tooltips
    auto drawTopTooltip = [&](const HudButton& btn) {
        if (btn.isHovered && !btn.tooltip.empty()) {
            float textLen = static_cast<float>(btn.tooltip.length());
            float tipW = (textLen * 7.5f + 24.0f) * m_dpiScale;
            float tipH = 26.0f * m_dpiScale;
            float tipCenterX = (btn.rect.left + btn.rect.right) / 2.0f;
            float tipLeft = tipCenterX - tipW / 2.0f;
            float tipRight = tipLeft + tipW;
            float tipTop = btn.rect.bottom + 8.0f * m_dpiScale;
            float tipBottom = tipTop + tipH;

            if (tipRight > screenWidth - 10.0f) {
                tipLeft -= (tipRight - (screenWidth - 10.0f));
                tipRight = screenWidth - 10.0f;
            }

            D2D1_RECT_F tipRect = D2D1::RectF(tipLeft, tipTop, tipRight, tipBottom);
            D2D1_ROUNDED_RECT tipRRect = D2D1::RoundedRect(tipRect, 6.0f * m_dpiScale, 6.0f * m_dpiScale);

            m_bgBrush->SetOpacity(0.96f * m_alpha);
            d2dContext->FillRoundedRectangle(&tipRRect, m_bgBrush);
            d2dContext->DrawRoundedRectangle(&tipRRect, m_borderBrush, 1.0f);
            d2dContext->DrawText(
                btn.tooltip.c_str(),
                static_cast<UINT32>(btn.tooltip.length()),
                m_textFormatSmall,
                tipRect,
                m_textBrush
            );
        }
    };
    drawTopTooltip(m_topFullscreenButton);
    drawTopTooltip(m_topCloseButton);
}
