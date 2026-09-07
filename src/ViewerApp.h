#pragma once
#include "platform/PlatformDefs.h"
#include "platform/D2DCompat.h"

#if defined(_WIN32)
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <dcomp.h>
#else
#include "platform/PlatformLinux.h"
#endif

#include <string>
#include <vector>
#include <memory>

#include "ImageLoader.h"
#include "FolderNavigator.h"
#include "HudRenderer.h"
#include "ThumbnailBar.h"
#include "Localization.h"
#include "CropToolbar.h"

class ViewerApp {
public:
    ViewerApp();
    ~ViewerApp();

    bool Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& initialFile);
#if !defined(_WIN32)
    bool Initialize(const std::wstring& initialFile);
#endif
    int Run();

#if defined(_WIN32)
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    // Platform-independent event handling
    void OnMouseMove(float mouseX, float mouseY);
    void OnMouseDown(int button, float mouseX, float mouseY, bool shift, bool alt, bool ctrl);
    void OnMouseUp(int button, float mouseX, float mouseY);
    void OnMouseWheel(short delta, float mouseX, float mouseY);
    void OnKeyDown(int keyCode, wchar_t keyChar, bool shift, bool alt, bool ctrl);
    void OnUpdate(float dt);

private:
#if defined(_WIN32)
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool CreateAppWindow(HINSTANCE hInstance);
#endif
    bool InitGraphics();
    void ResizeBuffers(UINT width, UINT height);

    void Render();
    void Invalidate();

    void LoadImage(const std::wstring& path);
    void NextImage();
    void PrevImage();
    void FirstImage();
    void LastImage();

    void ResetViewToFit();
    void SetActualSize(float targetScreenX = -1.0f, float targetScreenY = -1.0f);
    void ZoomAt(float factor, float cursorX, float cursorY);
    void Rotate(float angleDelta);
    void OpenFileDialog();
    void ToggleFullscreen();
    void UpdateDpiScale();
    void CycleAspectMode();
    void SetAspectMode(AspectMode mode);
    void ToggleLanguage();

    void ToggleCropMode();
    void ApplyCrop();
    void CancelCrop();
    void SetCropAspectRatio(CropRatio ratio);
    void ResetCropBox();

    void ToggleEraseMode();
    void MagicEraseAt(float screenX, float screenY, bool globalAll = false);
    void AutoRemoveBackground();

    void Undo();
    void PushUndoState(const std::wstring& note = L"");

    void SaveAs();

    bool ScreenToImagePixel(float screenX, float screenY, int& outPx, int& outPy) const;
    D2D1_POINT_2F ImagePixelToScreen(float px, float py) const;

    float CalculateFitScale() const;
    bool IsPointInsideImage(float x, float y) const;
    void UpdateImageInfoString();
    void UpdateBitmapFromPixels(UINT w, UINT h);

    void LoadSettings();
    void SaveSettings();

private:
#if defined(_WIN32)
    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
#else
    PlatformLinux* m_platform = nullptr;
#endif
    int m_screenX = 0;
    int m_screenY = 0;
    int m_screenWidth = 1920;
    int m_screenHeight = 1080;

    // Windowed / Fullscreen state
    bool m_isFullscreen = true;
    RECT m_windowedRect = {};
    float m_dpiScale = 1.0f;
    AspectMode m_aspectMode = AspectMode::Fit;

#if defined(_WIN32)
    // Graphics handles
    ID3D11Device* m_d3dDevice = nullptr;
    ID3D11DeviceContext* m_d3dContext = nullptr;
    IDXGISwapChain1* m_swapChain = nullptr;
    IDCompositionDevice* m_dcompDevice = nullptr;
    IDCompositionTarget* m_dcompTarget = nullptr;
    IDCompositionVisual* m_dcompVisual = nullptr;
    ID2D1Factory2* m_d2dFactory = nullptr;
    ID2D1Device1* m_d2dDevice = nullptr;
#endif
    ID2D1DeviceContext* m_d2dContext = nullptr;
    ID2D1Bitmap1* m_targetBitmap = nullptr;
    ID2D1SolidColorBrush* m_emptyPromptBrush = nullptr;
    ID2D1SolidColorBrush* m_shadowBrush = nullptr;
    ID2D1SolidColorBrush* m_selectionBorderBrush = nullptr;

    // Editing Brushes
    ID2D1SolidColorBrush* m_cropMaskBrush = nullptr;
    ID2D1SolidColorBrush* m_cropBorderBrush = nullptr;
    ID2D1SolidColorBrush* m_cropGridBrush = nullptr;
    ID2D1SolidColorBrush* m_cropHandleBrush = nullptr;

    // Subsystems
    ImageLoader m_imageLoader;
    FolderNavigator m_folderNav;
    HudRenderer m_hud;
    ThumbnailBar m_thumbBar;
    CropToolbar m_cropToolbar;

    // Current Image state & Pixel buffer
    ID2D1Bitmap1* m_currentBitmap = nullptr;
    UINT m_imageWidth = 0;
    UINT m_imageHeight = 0;
    std::wstring m_imageInfoString;
    std::vector<uint32_t> m_imagePixels;

    // Undo stack
    struct ImageHistoryState {
        std::vector<uint32_t> pixels;
        UINT width = 0;
        UINT height = 0;
        std::wstring note;
    };
    std::vector<ImageHistoryState> m_undoStack;

    // Crop Tool state
    bool m_isCropping = false;
    D2D1_RECT_F m_cropNormRect = { 0.05f, 0.05f, 0.95f, 0.95f };
    int m_activeCropHandle = -1;
    D2D1_POINT_2F m_cropDragStart = {};
    D2D1_RECT_F m_cropRectAtDragStart = {};
    CropRatio m_activeCropRatio = CropRatio::Free;
    bool m_isCropSymmetric = false;

    // Magic Eraser Tool state
    bool m_isErasing = false;
    float m_eraseTolerance = 34.0f;

    // Animated GIF state
    std::vector<FrameData> m_gifFrames;
    size_t m_currentGifFrame = 0;
    float m_gifTimer = 0.0f;
    bool m_isGif = false;

    // Transform state
    float m_zoom = 1.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;
    float m_rotation = 0.0f; // degrees (0, 90, 180, 270)
    float m_bgOpacity = 0.78f; // 78% dark overlay, like Picasa

    // Mouse drag & hover state
    bool m_isDragging = false;
    bool m_isHoveringImage = false;
    POINT m_dragStartMouse = {};
    float m_dragStartPanX = 0.0f;
    float m_dragStartPanY = 0.0f;

    // Animation & update loop
    DWORD m_lastTick = 0;
    bool m_needsRepaint = true;
};
