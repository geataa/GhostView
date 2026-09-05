#include "ThumbnailBar.h"
#include <algorithm>
#include <chrono>

ThumbnailBar::ThumbnailBar() = default;

ThumbnailBar::~ThumbnailBar() {
    StopWorkerThread();

    for (auto& entry : m_thumbnails) {
        if (entry.bitmap) {
            entry.bitmap->Release();
            entry.bitmap = nullptr;
        }
    }

    if (m_trayBgBrush) m_trayBgBrush->Release();
    if (m_trayBorderBrush) m_trayBorderBrush->Release();
    if (m_thumbBorderBrush) m_thumbBorderBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();
    if (m_hoverBorderBrush) m_hoverBorderBrush->Release();
    if (m_placeholderBrush) m_placeholderBrush->Release();
}

bool ThumbnailBar::Initialize(ID2D1DeviceContext* d2dContext, float dpiScale) {
    m_dpiScale = (dpiScale > 0.5f) ? dpiScale : 1.0f;

    CreateResources(d2dContext);
    StartWorkerThread();
    return true;
}

void ThumbnailBar::SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale) {
    if (dpiScale <= 0.5f) dpiScale = 1.0f;
    if (std::abs(m_dpiScale - dpiScale) > 0.01f) {
        m_dpiScale = dpiScale;
        CreateResources(d2dContext);
    }
}

void ThumbnailBar::CreateResources(ID2D1DeviceContext* d2dContext) {
    if (!d2dContext) return;

    if (m_trayBgBrush) m_trayBgBrush->Release();
    if (m_trayBorderBrush) m_trayBorderBrush->Release();
    if (m_thumbBorderBrush) m_thumbBorderBrush->Release();
    if (m_activeBorderBrush) m_activeBorderBrush->Release();
    if (m_hoverBorderBrush) m_hoverBorderBrush->Release();
    if (m_placeholderBrush) m_placeholderBrush->Release();

    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.08f, 0.10f, 0.88f), &m_trayBgBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.40f, 0.40f, 0.45f, 0.45f), &m_trayBorderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.35f, 0.40f, 0.50f), &m_thumbBorderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.75f, 1.00f, 1.00f), &m_activeBorderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.00f, 1.00f, 1.00f, 0.50f), &m_hoverBorderBrush);
    d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.20f, 0.20f, 0.25f, 0.85f), &m_placeholderBrush);
}

void ThumbnailBar::StartWorkerThread() {
    m_stopWorker = false;
    m_workerThread = std::thread(&ThumbnailBar::WorkerLoop, this);
}

void ThumbnailBar::StopWorkerThread() {
    m_stopWorker = true;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void ThumbnailBar::UpdateTargetScrollForCurrentIndex(float screenWidth) {
    if (m_thumbnails.empty()) {
        m_targetScrollX = 0.0f;
        m_scrollX = 0.0f;
        return;
    }

    float thumbW = 46.0f * m_dpiScale;
    float gap = 6.0f * m_dpiScale;
    float padding = 10.0f * m_dpiScale;

    float totalContentWidth = m_thumbnails.size() * (thumbW + gap) - gap + padding * 2.0f;
    float maxTrayWidth = screenWidth - 40.0f * m_dpiScale;
    float trayW = (std::min)(maxTrayWidth, totalContentWidth);
    float visibleWidth = trayW - padding * 2.0f;

    if (totalContentWidth > trayW && visibleWidth > 0.0f) {
        float activeItemCenterX = m_currentIndex * (thumbW + gap) + thumbW / 2.0f;
        float maxScroll = totalContentWidth - padding * 2.0f - visibleWidth;
        if (maxScroll < 0.0f) maxScroll = 0.0f;
        m_targetScrollX = std::clamp(activeItemCenterX - visibleWidth / 2.0f, 0.0f, maxScroll);
    } else {
        m_targetScrollX = 0.0f;
    }
}

void ThumbnailBar::SetFileList(const std::vector<std::wstring>& files, size_t currentIndex, float screenWidth) {
    StopWorkerThread();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_requestedIndices.clear();
        m_loadedResults.clear();
    }

    for (auto& entry : m_thumbnails) {
        if (entry.bitmap) {
            entry.bitmap->Release();
            entry.bitmap = nullptr;
        }
    }

    m_thumbnails.clear();
    m_thumbnails.resize(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        m_thumbnails[i].filePath = files[i];
        m_thumbnails[i].isLoaded = false;
    }

    m_currentIndex = (currentIndex < files.size()) ? currentIndex : 0;
    UpdateTargetScrollForCurrentIndex(screenWidth);
    m_scrollX = m_targetScrollX;

    // Queue ALL thumbnails prioritized outward from currentIndex
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        int center = static_cast<int>(m_currentIndex);
        int total = static_cast<int>(m_thumbnails.size());

        std::vector<bool> inQueue(total, false);
        for (int radius = 0; radius < total; ++radius) {
            int idx1 = center + radius;
            if (idx1 < total && !inQueue[idx1]) {
                m_requestedIndices.push_back(idx1);
                inQueue[idx1] = true;
            }
            if (radius > 0) {
                int idx2 = center - radius;
                if (idx2 >= 0 && !inQueue[idx2]) {
                    m_requestedIndices.push_back(idx2);
                    inQueue[idx2] = true;
                }
            }
        }
    }

    m_needsRedraw = true;
    StartWorkerThread();
}

void ThumbnailBar::SetCurrentIndex(size_t index, float screenWidth) {
    if (m_thumbnails.empty()) return;
    if (index >= m_thumbnails.size()) index = m_thumbnails.size() - 1;
    m_currentIndex = index;

    UpdateTargetScrollForCurrentIndex(screenWidth);

    // Re-prioritize pending unrendered thumbnails around new index WITHOUT losing any
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::vector<size_t> remaining;
        std::vector<bool> inRemaining(m_thumbnails.size(), false);

        for (size_t idx : m_requestedIndices) {
            if (idx < m_thumbnails.size() && !m_thumbnails[idx].isLoaded && !inRemaining[idx]) {
                remaining.push_back(idx);
                inRemaining[idx] = true;
            }
        }
        for (size_t i = 0; i < m_thumbnails.size(); ++i) {
            if (!m_thumbnails[i].isLoaded && !inRemaining[i]) {
                remaining.push_back(i);
                inRemaining[i] = true;
            }
        }

        int center = static_cast<int>(m_currentIndex);
        std::sort(remaining.begin(), remaining.end(), [center](size_t a, size_t b) {
            return std::abs(static_cast<int>(a) - center) < std::abs(static_cast<int>(b) - center);
        });

        m_requestedIndices.clear();
        for (size_t idx : remaining) {
            m_requestedIndices.push_back(idx);
        }
    }

    m_needsRedraw = true;
}

bool ThumbnailBar::GenerateThumbnail(
    IWICImagingFactory* factory,
    const std::wstring& path,
    UINT targetH,
    ThumbnailData& outData
) {
    if (!factory) return false;

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = factory->CreateDecoderFromFilename(
        path.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );
    if (FAILED(hr) || !decoder) return false;

    IWICBitmapSource* source = nullptr;

    // 1. Try embedded EXIF thumbnail
    if (SUCCEEDED(decoder->GetThumbnail(&source)) && source) {
        // Thumbnail found
    } else {
        // 2. Decode frame 0 and scale it
        IWICBitmapFrameDecode* frame = nullptr;
        if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
            UINT origW = 0, origH = 0;
            frame->GetSize(&origW, &origH);

            if (origW > 0 && origH > 0) {
                UINT targetW = (origW * targetH) / origH;
                if (targetW < targetH / 2) targetW = targetH / 2;
                if (targetW > targetH * 2) targetW = targetH * 2;

                IWICBitmapScaler* scaler = nullptr;
                if (SUCCEEDED(factory->CreateBitmapScaler(&scaler))) {
                    if (SUCCEEDED(scaler->Initialize(frame, targetW, targetH, WICBitmapInterpolationModeLinear))) {
                        source = scaler;
                        source->AddRef();
                    }
                    scaler->Release();
                }
            }
            frame->Release();
        }
    }

    bool success = false;
    if (source) {
        // Convert to 32bpp PBGRA
        IWICFormatConverter* converter = nullptr;
        if (SUCCEEDED(factory->CreateFormatConverter(&converter))) {
            if (SUCCEEDED(converter->Initialize(
                source,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr, 0.0f,
                WICBitmapPaletteTypeCustom
            ))) {
                UINT w = 0, h = 0;
                converter->GetSize(&w, &h);
                if (w > 0 && h > 0) {
                    UINT stride = w * 4;
                    UINT bufferSize = stride * h;
                    outData.pixels.resize(bufferSize);
                    if (SUCCEEDED(converter->CopyPixels(nullptr, stride, bufferSize, outData.pixels.data()))) {
                        outData.width = w;
                        outData.height = h;
                        outData.filePath = path;
                        success = true;
                    } else {
                        outData.pixels.clear();
                    }
                }
            }
            converter->Release();
        }
        source->Release();
    }

    decoder->Release();
    return success;
}

void ThumbnailBar::WorkerLoop() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IWICImagingFactory* localFactory = nullptr;
    CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&localFactory)
    );

    while (!m_stopWorker) {
        size_t nextIndex = 0;
        std::wstring path = L"";
        bool hasTask = false;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_requestedIndices.empty()) {
                nextIndex = m_requestedIndices.front();
                m_requestedIndices.pop_front();
                if (nextIndex < m_thumbnails.size()) {
                    path = m_thumbnails[nextIndex].filePath;
                    hasTask = true;
                }
            }
        }

        if (hasTask && !path.empty() && localFactory) {
            ThumbnailData data;
            if (GenerateThumbnail(localFactory, path, 60, data)) {
                data.index = nextIndex;
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_loadedResults.push_back(std::move(data));
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    if (localFactory) {
        localFactory->Release();
        localFactory = nullptr;
    }

    CoUninitialize();
}

void ThumbnailBar::Update(float deltaTimeSeconds, ID2D1DeviceContext* d2dContext) {
    std::vector<ThumbnailData> ready;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (!m_loadedResults.empty()) {
            ready.swap(m_loadedResults);
        }
    }

    if (!ready.empty() && d2dContext) {
        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        for (auto& item : ready) {
            if (item.index < m_thumbnails.size() && !m_thumbnails[item.index].bitmap && !item.pixels.empty()) {
                ID2D1Bitmap* bmp = nullptr;
                UINT stride = item.width * 4;
                if (SUCCEEDED(d2dContext->CreateBitmap(
                    D2D1::SizeU(item.width, item.height),
                    item.pixels.data(),
                    stride,
                    props,
                    &bmp
                ))) {
                    m_thumbnails[item.index].bitmap = bmp;
                    m_thumbnails[item.index].isLoaded = true;
                    m_needsRedraw = true;
                }
            }
        }
    }

    // Smooth scroll
    float diff = m_targetScrollX - m_scrollX;
    if (std::abs(diff) > 0.2f) {
        float step = diff * (std::min)(1.0f, deltaTimeSeconds * 16.0f);
        m_scrollX += step;
        m_needsRedraw = true;
    } else {
        m_scrollX = m_targetScrollX;
    }
}

void ThumbnailBar::Render(
    ID2D1DeviceContext* d2dContext,
    float screenWidth,
    float screenHeight,
    float bottomBarTop,
    float alpha
) {
    if (!d2dContext || m_thumbnails.empty() || alpha <= 0.01f) return;

    float thumbH = 46.0f * m_dpiScale;
    float thumbW = 46.0f * m_dpiScale;
    float gap = 6.0f * m_dpiScale;
    float padding = 10.0f * m_dpiScale;

    float trayH = thumbH + padding * 2.0f;
    float trayMarginBottom = 10.0f * m_dpiScale;
    float trayTop = bottomBarTop - trayH - trayMarginBottom;
    float trayBottom = trayTop + trayH;

    float totalContentWidth = m_thumbnails.size() * (thumbW + gap) - gap + padding * 2.0f;
    float maxTrayWidth = screenWidth - 40.0f * m_dpiScale;
    float trayW = (std::min)(maxTrayWidth, totalContentWidth);

    float trayLeft = (screenWidth - trayW) / 2.0f;
    float trayRight = trayLeft + trayW;

    m_trayRect = D2D1::RectF(trayLeft, trayTop, trayRight, trayBottom);

    m_trayBgBrush->SetOpacity(0.88f * alpha);
    m_trayBorderBrush->SetOpacity(0.45f * alpha);
    m_thumbBorderBrush->SetOpacity(0.40f * alpha);
    m_activeBorderBrush->SetOpacity(1.00f * alpha);
    m_hoverBorderBrush->SetOpacity(0.70f * alpha);
    m_placeholderBrush->SetOpacity(0.80f * alpha);

    float trayR = trayH / 2.0f;
    D2D1_ROUNDED_RECT trayRRect = D2D1::RoundedRect(m_trayRect, trayR, trayR);
    d2dContext->FillRoundedRectangle(&trayRRect, m_trayBgBrush);
    d2dContext->DrawRoundedRectangle(&trayRRect, m_trayBorderBrush, 1.2f);

    D2D1_RECT_F clipRect = D2D1::RectF(trayLeft + 10.0f * m_dpiScale, trayTop, trayRight - 10.0f * m_dpiScale, trayBottom);
    d2dContext->PushAxisAlignedClip(&clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    float itemY = trayTop + padding;

    for (size_t i = 0; i < m_thumbnails.size(); ++i) {
        float itemX = trayLeft + padding + i * (thumbW + gap) - m_scrollX;

        // Skip items that are strictly off-screen
        if (itemX + thumbW < clipRect.left || itemX > clipRect.right) {
            continue;
        }

        D2D1_RECT_F itemRect = D2D1::RectF(itemX, itemY, itemX + thumbW, itemY + thumbH);
        D2D1_ROUNDED_RECT itemRRect = D2D1::RoundedRect(itemRect, 6.0f * m_dpiScale, 6.0f * m_dpiScale);

        // 1. Draw item background
        d2dContext->FillRoundedRectangle(&itemRRect, m_placeholderBrush);

        // 2. Draw thumbnail image
        if (m_thumbnails[i].bitmap) {
            D2D1_SIZE_F bmpSize = m_thumbnails[i].bitmap->GetSize();
            if (bmpSize.width > 0 && bmpSize.height > 0) {
                float s = (std::min)(thumbW / bmpSize.width, thumbH / bmpSize.height);
                float dw = bmpSize.width * s;
                float dh = bmpSize.height * s;
                float dx = itemX + (thumbW - dw) / 2.0f;
                float dy = itemY + (thumbH - dh) / 2.0f;

                D2D1_RECT_F dst = D2D1::RectF(dx, dy, dx + dw, dy + dh);
                d2dContext->DrawBitmap(m_thumbnails[i].bitmap, dst, alpha, D2D1_INTERPOLATION_MODE_LINEAR);
            }
        }

        // 3. Draw border highlights
        if (i == m_currentIndex) {
            d2dContext->DrawRoundedRectangle(&itemRRect, m_activeBorderBrush, 2.5f * m_dpiScale);
        } else if (static_cast<int>(i) == m_hoveredIndex) {
            d2dContext->DrawRoundedRectangle(&itemRRect, m_hoverBorderBrush, 1.8f * m_dpiScale);
        } else {
            d2dContext->DrawRoundedRectangle(&itemRRect, m_thumbBorderBrush, 1.0f);
        }
    }

    d2dContext->PopAxisAlignedClip();
}

bool ThumbnailBar::IsMouseOver(float mouseX, float mouseY) const {
    return (mouseX >= m_trayRect.left && mouseX <= m_trayRect.right &&
            mouseY >= m_trayRect.top && mouseY <= m_trayRect.bottom);
}

void ThumbnailBar::OnMouseMove(float mouseX, float mouseY) {
    m_hoveredIndex = -1;
    if (!IsMouseOver(mouseX, mouseY) || m_thumbnails.empty()) return;

    float thumbW = 46.0f * m_dpiScale;
    float gap = 6.0f * m_dpiScale;
    float padding = 10.0f * m_dpiScale;

    float relX = mouseX - (m_trayRect.left + padding) + m_scrollX;
    if (relX >= 0.0f) {
        int idx = static_cast<int>(relX / (thumbW + gap));
        float rem = relX - idx * (thumbW + gap);
        if (idx >= 0 && static_cast<size_t>(idx) < m_thumbnails.size() && rem <= thumbW) {
            m_hoveredIndex = idx;
        }
    }
}

int ThumbnailBar::OnMouseDown(float mouseX, float mouseY) {
    if (!IsMouseOver(mouseX, mouseY) || m_thumbnails.empty()) return -1;

    float thumbW = 46.0f * m_dpiScale;
    float gap = 6.0f * m_dpiScale;
    float padding = 10.0f * m_dpiScale;

    float relX = mouseX - (m_trayRect.left + padding) + m_scrollX;
    if (relX >= 0.0f) {
        int idx = static_cast<int>(relX / (thumbW + gap));
        float rem = relX - idx * (thumbW + gap);
        if (idx >= 0 && static_cast<size_t>(idx) < m_thumbnails.size() && rem <= thumbW) {
            return idx;
        }
    }
    return -1;
}

void ThumbnailBar::OnMouseWheel(short delta, float screenWidth) {
    if (m_thumbnails.empty()) return;

    float thumbW = 46.0f * m_dpiScale;
    float gap = 6.0f * m_dpiScale;
    float padding = 10.0f * m_dpiScale;

    float totalContentWidth = m_thumbnails.size() * (thumbW + gap) - gap + padding * 2.0f;
    float maxTrayWidth = screenWidth - 40.0f * m_dpiScale;
    float trayW = (std::min)(maxTrayWidth, totalContentWidth);
    float visibleWidth = trayW - padding * 2.0f;

    if (totalContentWidth > trayW && visibleWidth > 0.0f) {
        float maxScroll = totalContentWidth - padding * 2.0f - visibleWidth;
        float scrollStep = (delta / 120.0f) * 120.0f * m_dpiScale;
        m_targetScrollX = std::clamp(m_targetScrollX - scrollStep, 0.0f, maxScroll);
        m_needsRedraw = true;
    }
}
