#pragma once
#include "platform/PlatformDefs.h"
#include "platform/D2DCompat.h"

#if defined(_WIN32)
#include <wincodec.h>
#else
typedef void IWICImagingFactory;
#endif
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <deque>

struct ThumbnailData {
    size_t index = 0;
    std::wstring filePath;
    std::vector<BYTE> pixels;
    UINT width = 0;
    UINT height = 0;
};

struct ThumbnailEntry {
    std::wstring filePath;
    ID2D1Bitmap* bitmap = nullptr;
    bool isLoaded = false;
};

class ThumbnailBar {
public:
    ThumbnailBar();
    ~ThumbnailBar();

    bool Initialize(ID2D1DeviceContext* d2dContext, float dpiScale = 1.0f);
    void SetDpiScale(ID2D1DeviceContext* d2dContext, float dpiScale);
    void SetFileList(const std::vector<std::wstring>& files, size_t currentIndex, float screenWidth = 1920.0f);
    void SetCurrentIndex(size_t index, float screenWidth = 1920.0f);

    void Update(float deltaTimeSeconds, ID2D1DeviceContext* d2dContext);
    void Render(
        ID2D1DeviceContext* d2dContext,
        float screenWidth,
        float screenHeight,
        float bottomBarTop,
        float alpha
    );

    void OnMouseMove(float mouseX, float mouseY);
    int OnMouseDown(float mouseX, float mouseY); // Returns clicked index or -1
    void OnMouseWheel(short delta, float screenWidth = 1920.0f);
    bool IsMouseOver(float mouseX, float mouseY) const;

    bool NeedsRedraw() const { return m_needsRedraw; }
    void ClearNeedsRedraw() { m_needsRedraw = false; }

private:
    void StartWorkerThread();
    void StopWorkerThread();
    void WorkerLoop();
    bool GenerateThumbnail(IWICImagingFactory* factory, const std::wstring& path, UINT targetH, ThumbnailData& outData);
    void CreateResources(ID2D1DeviceContext* d2dContext);
    void UpdateTargetScrollForCurrentIndex(float screenWidth);

private:
    float m_dpiScale = 1.0f;
    bool m_needsRedraw = false;

    ID2D1SolidColorBrush* m_trayBgBrush = nullptr;
    ID2D1SolidColorBrush* m_trayBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_thumbBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_activeBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_hoverBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_placeholderBrush = nullptr;

    std::vector<ThumbnailEntry> m_thumbnails;
    size_t m_currentIndex = 0;
    int m_hoveredIndex = -1;

    // Scrolling
    float m_scrollX = 0.0f;
    float m_targetScrollX = 0.0f;

    D2D1_RECT_F m_trayRect = {};

    // Multithreaded thumbnail loading
    std::thread m_workerThread;
    std::atomic<bool> m_stopWorker{ false };
    std::mutex m_queueMutex;
    std::deque<size_t> m_requestedIndices;
    std::vector<ThumbnailData> m_loadedResults;
};
