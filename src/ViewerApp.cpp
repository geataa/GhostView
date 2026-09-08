#include "ViewerApp.h"
#include <sstream>
#include <algorithm>
#include <deque>
#include <cmath>
#include <fstream>
#include <filesystem>

#if defined(_WIN32)
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

static const wchar_t* CLASS_NAME = L"GhostViewWindowClass";
#endif

ViewerApp::ViewerApp() = default;

ViewerApp::~ViewerApp() {
    SaveSettings();

    for (auto& f : m_gifFrames) {
        if (f.bitmap) {
            f.bitmap->Release();
            f.bitmap = nullptr;
        }
    }
    m_gifFrames.clear();

    if (m_emptyPromptBrush) m_emptyPromptBrush->Release();
    if (m_shadowBrush) m_shadowBrush->Release();
    if (m_selectionBorderBrush) m_selectionBorderBrush->Release();
    if (m_cropMaskBrush) m_cropMaskBrush->Release();
    if (m_cropBorderBrush) m_cropBorderBrush->Release();
    if (m_cropGridBrush) m_cropGridBrush->Release();
    if (m_cropHandleBrush) m_cropHandleBrush->Release();

    if (m_currentBitmap) {
        // If not in m_gifFrames (single frame)
        if (!m_isGif) m_currentBitmap->Release();
        m_currentBitmap = nullptr;
    }

#if defined(_WIN32)
    if (m_targetBitmap) m_targetBitmap->Release();
    if (m_d2dContext) m_d2dContext->Release();
    if (m_d2dDevice) m_d2dDevice->Release();
    if (m_d2dFactory) m_d2dFactory->Release();
    if (m_dcompVisual) m_dcompVisual->Release();
    if (m_dcompTarget) m_dcompTarget->Release();
    if (m_dcompDevice) m_dcompDevice->Release();
    if (m_swapChain) m_swapChain->Release();
    if (m_d3dContext) m_d3dContext->Release();
    if (m_d3dDevice) m_d3dDevice->Release();

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
#else
    if (m_d2dContext) {
        m_d2dContext->Release();
        m_d2dContext = nullptr;
    }
    if (m_platform) {
        delete m_platform;
        m_platform = nullptr;
    }
#endif
}

#if defined(_WIN32)
void ViewerApp::LoadSettings() {
    // Default language is automatically detected from system UI
    Language defaultLang = Localization::DetectSystemLanguage();
    Localization::SetLanguage(defaultLang);

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\GhostView\\Settings", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwVal = 0, dwSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"IsFullscreen", nullptr, nullptr, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS) {
            m_isFullscreen = (dwVal != 0);
        }

        DWORD dwLang = 0;
        dwSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"Language", nullptr, nullptr, (LPBYTE)&dwLang, &dwSize) == ERROR_SUCCESS) {
            Localization::SetLanguage(dwLang == 1 ? Language::English : Language::Turkish);
        }

        DWORD wx = 0, wy = 0, ww = 0, wh = 0;
        dwSize = sizeof(DWORD);
        bool hasRect = (RegQueryValueExW(hKey, L"WindowX", nullptr, nullptr, (LPBYTE)&wx, &dwSize) == ERROR_SUCCESS);
        dwSize = sizeof(DWORD);
        hasRect &= (RegQueryValueExW(hKey, L"WindowY", nullptr, nullptr, (LPBYTE)&wy, &dwSize) == ERROR_SUCCESS);
        dwSize = sizeof(DWORD);
        hasRect &= (RegQueryValueExW(hKey, L"WindowW", nullptr, nullptr, (LPBYTE)&ww, &dwSize) == ERROR_SUCCESS);
        dwSize = sizeof(DWORD);
        hasRect &= (RegQueryValueExW(hKey, L"WindowH", nullptr, nullptr, (LPBYTE)&wh, &dwSize) == ERROR_SUCCESS);

        if (hasRect && ww >= 300 && wh >= 200) {
            RECT r = { static_cast<LONG>(wx), static_cast<LONG>(wy),
                       static_cast<LONG>(wx + ww), static_cast<LONG>(wy + wh) };
            HMONITOR hMon = MonitorFromRect(&r, MONITOR_DEFAULTTONULL);
            if (hMon) {
                m_windowedRect = r;
            }
        }
        RegCloseKey(hKey);
    }
}

void ViewerApp::SaveSettings() {
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\GhostView\\Settings", 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {

        DWORD dwFullscreen = m_isFullscreen ? 1 : 0;
        RegSetValueExW(hKey, L"IsFullscreen", 0, REG_DWORD, (const BYTE*)&dwFullscreen, sizeof(DWORD));

        DWORD dwLang = (Localization::GetCurrentLanguage() == Language::English) ? 1 : 0;
        RegSetValueExW(hKey, L"Language", 0, REG_DWORD, (const BYTE*)&dwLang, sizeof(DWORD));

        RECT r = m_windowedRect;
        if (!m_isFullscreen && m_hwnd) {
            GetWindowRect(m_hwnd, &r);
        }
        DWORD wx = r.left;
        DWORD wy = r.top;
        DWORD ww = r.right - r.left;
        DWORD wh = r.bottom - r.top;

        RegSetValueExW(hKey, L"WindowX", 0, REG_DWORD, (const BYTE*)&wx, sizeof(DWORD));
        RegSetValueExW(hKey, L"WindowY", 0, REG_DWORD, (const BYTE*)&wy, sizeof(DWORD));
        RegSetValueExW(hKey, L"WindowW", 0, REG_DWORD, (const BYTE*)&ww, sizeof(DWORD));
        RegSetValueExW(hKey, L"WindowH", 0, REG_DWORD, (const BYTE*)&wh, sizeof(DWORD));

        RegCloseKey(hKey);
    }
}

void ViewerApp::UpdateDpiScale() {
    UINT dpi = 96;
    typedef UINT(WINAPI* GetDpiForWindowFn)(HWND);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pfn = (GetDpiForWindowFn)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pfn && m_hwnd) {
            dpi = pfn(m_hwnd);
        }
    }
    if (dpi == 0) dpi = 96;
    m_dpiScale = static_cast<float>(dpi) / 96.0f;
    if (m_d2dContext) {
        m_hud.SetDpiScale(m_d2dContext, m_dpiScale);
        m_thumbBar.SetDpiScale(m_d2dContext, m_dpiScale);
        m_cropToolbar.SetDpiScale(m_d2dContext, m_dpiScale);
    }
}
#else
void ViewerApp::LoadSettings() {
    Language defaultLang = Localization::DetectSystemLanguage();
    Localization::SetLanguage(defaultLang);

    const char* home = getenv("HOME");
    if (!home) return;
    std::string configPath = std::string(home) + "/.config/ghostview/settings.conf";
    std::ifstream f(configPath);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "Language") {
            int l = std::atoi(val.c_str());
            Localization::SetLanguage(l == 1 ? Language::English : Language::Turkish);
        } else if (key == "IsFullscreen") {
            m_isFullscreen = (std::atoi(val.c_str()) != 0);
        } else if (key == "BgOpacity") {
            m_bgOpacity = std::atof(val.c_str());
        }
    }
}

void ViewerApp::SaveSettings() {
    const char* home = getenv("HOME");
    if (!home) return;
    std::string dir = std::string(home) + "/.config/ghostview";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::string configPath = dir + "/settings.conf";
    std::ofstream f(configPath);
    if (f.is_open()) {
        f << "Language=" << (Localization::GetCurrentLanguage() == Language::English ? 1 : 0) << "\n";
        f << "IsFullscreen=" << (m_isFullscreen ? 1 : 0) << "\n";
        f << "BgOpacity=" << m_bgOpacity << "\n";
    }
}

void ViewerApp::UpdateDpiScale() {
    m_dpiScale = 1.0f;
    if (m_d2dContext) {
        m_hud.SetDpiScale(m_d2dContext, m_dpiScale);
        m_thumbBar.SetDpiScale(m_d2dContext, m_dpiScale);
        m_cropToolbar.SetDpiScale(m_d2dContext, m_dpiScale);
    }
}
#endif

#if defined(_WIN32)
bool ViewerApp::Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& initialFile) {
    m_hInstance = hInstance;

    // CoInitialize for WIC
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!m_imageLoader.Initialize()) {
        return false;
    }

    if (!CreateAppWindow(hInstance)) {
        return false;
    }

    UpdateDpiScale();

    if (!InitGraphics()) {
        return false;
    }

    if (!m_hud.Initialize(m_d2dContext, m_dpiScale)) {
        return false;
    }

    m_thumbBar.Initialize(m_d2dContext, m_dpiScale);
    m_cropToolbar.Initialize(m_d2dContext, m_dpiScale);

    m_hud.SetFullscreenState(m_isFullscreen);
    m_hud.SetAspectMode(m_aspectMode);

    // Set animation timer for HUD & GIF playback (smooth 60Hz)
    SetTimer(m_hwnd, 1, 16, nullptr);
    m_lastTick = GetTickCount();

    // Enable Drag & Drop
    DragAcceptFiles(m_hwnd, TRUE);

    // If an initial file was passed
    if (!initialFile.empty()) {
        m_folderNav.LoadFromInitialFile(initialFile);
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        LoadImage(initialFile);
    }

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    Render();
    return true;
}

bool ViewerApp::CreateAppWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = ViewerApp::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    if (!wc.hIcon) wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(hMon, &mi);

    m_screenX = mi.rcMonitor.left;
    m_screenY = mi.rcMonitor.top;
    m_screenWidth = mi.rcMonitor.right - mi.rcMonitor.left;
    m_screenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;

    int defW = (std::min)(1280, static_cast<int>(m_screenWidth * 0.75f));
    int defH = (std::min)(760, static_cast<int>(m_screenHeight * 0.75f));
    int defX = m_screenX + (m_screenWidth - defW) / 2;
    int defY = m_screenY + (m_screenHeight - defH) / 2;
    m_windowedRect = { defX, defY, defX + defW, defY + defH };

    LoadSettings();

    int initX = m_screenX;
    int initY = m_screenY;
    int initW = m_screenWidth;
    int initH = m_screenHeight;

    DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP;
    if (m_isFullscreen) {
        exStyle |= WS_EX_TOPMOST;
    } else {
        initX = m_windowedRect.left;
        initY = m_windowedRect.top;
        initW = m_windowedRect.right - m_windowedRect.left;
        initH = m_windowedRect.bottom - m_windowedRect.top;
        m_screenWidth = initW;
        m_screenHeight = initH;
    }

    m_hwnd = CreateWindowExW(
        exStyle,
        CLASS_NAME,
        L"GhostView",
        WS_POPUP,
        initX, initY, initW, initH,
        nullptr, nullptr, hInstance, this
    );

    if (m_hwnd) {
        if (wc.hIcon) SendMessageW(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
        if (wc.hIconSm) SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);
    }

    return (m_hwnd != nullptr);
}

bool ViewerApp::InitGraphics() {
    D3D_FEATURE_LEVEL featureLevel;
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &featureLevel,
        &m_d3dContext
    );
    if (FAILED(hr)) return false;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    IDXGIAdapter* dxgiAdapter = nullptr;
    dxgiDevice->GetAdapter(&dxgiAdapter);

    IDXGIFactory2* dxgiFactory = nullptr;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory);

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = m_screenWidth;
    scDesc.Height = m_screenHeight;
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.Stereo = FALSE;
    scDesc.SampleDesc.Count = 1;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    hr = dxgiFactory->CreateSwapChainForComposition(m_d3dDevice, &scDesc, nullptr, &m_swapChain);
    if (FAILED(hr)) {
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        hr = dxgiFactory->CreateSwapChainForComposition(m_d3dDevice, &scDesc, nullptr, &m_swapChain);
    }

    dxgiFactory->Release();
    dxgiAdapter->Release();
    if (FAILED(hr)) {
        dxgiDevice->Release();
        return false;
    }

    hr = DCompositionCreateDevice(dxgiDevice, __uuidof(IDCompositionDevice), (void**)&m_dcompDevice);
    if (FAILED(hr)) {
        dxgiDevice->Release();
        return false;
    }

    hr = m_dcompDevice->CreateTargetForHwnd(m_hwnd, TRUE, &m_dcompTarget);
    if (FAILED(hr)) {
        dxgiDevice->Release();
        return false;
    }

    m_dcompDevice->CreateVisual(&m_dcompVisual);
    m_dcompVisual->SetContent(m_swapChain);
    m_dcompTarget->SetRoot(m_dcompVisual);
    m_dcompDevice->Commit();

    D2D1_FACTORY_OPTIONS d2dOpts = {};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory2), &d2dOpts, (void**)&m_d2dFactory);
    m_d2dFactory->CreateDevice(dxgiDevice, &m_d2dDevice);
    m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);

    dxgiDevice->Release();

    IDXGISurface* surface = nullptr;
    m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);

    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    m_d2dContext->CreateBitmapFromDxgiSurface(surface, &bp, &m_targetBitmap);
    surface->Release();

    m_d2dContext->SetTarget(m_targetBitmap);
    m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f, 0.85f), &m_emptyPromptBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.16f), &m_shadowBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.45f), &m_selectionBorderBrush);

    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.65f), &m_cropMaskBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.85f, 1.0f, 0.95f), &m_cropBorderBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.35f), &m_cropGridBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f), &m_cropHandleBrush);

    return true;
}

void ViewerApp::ResizeBuffers(UINT width, UINT height) {
    if (!m_swapChain || width == 0 || height == 0) return;

    if (m_targetBitmap != nullptr) {
        D2D1_SIZE_U curSize = m_targetBitmap->GetPixelSize();
        if (curSize.width == width && curSize.height == height) {
            return;
        }
    }

    m_screenWidth = static_cast<int>(width);
    m_screenHeight = static_cast<int>(height);

    if (m_d2dContext) {
        m_d2dContext->SetTarget(nullptr);
    }
    if (m_targetBitmap) {
        m_targetBitmap->Release();
        m_targetBitmap = nullptr;
    }

    HRESULT hr = m_swapChain->ResizeBuffers(
        2,
        width,
        height,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        0
    );

    if (SUCCEEDED(hr)) {
        IDXGISurface* surface = nullptr;
        m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface);
        if (surface) {
            D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );
            m_d2dContext->CreateBitmapFromDxgiSurface(surface, &bp, &m_targetBitmap);
            surface->Release();

            m_d2dContext->SetTarget(m_targetBitmap);
            m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
        if (m_dcompDevice) {
            m_dcompDevice->Commit();
        }
    }
}

void ViewerApp::ToggleFullscreen() {
    if (m_isFullscreen) {
        m_isFullscreen = false;
        m_hud.SetFullscreenState(false);

        HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);

        int workW = mi.rcWork.right - mi.rcWork.left;
        int workH = mi.rcWork.bottom - mi.rcWork.top;

        int w = m_windowedRect.right - m_windowedRect.left;
        int h = m_windowedRect.bottom - m_windowedRect.top;

        if (w < 400 || h < 300 || w > workW || h > workH) {
            w = (std::min)(1280, static_cast<int>(workW * 0.85f));
            h = (std::min)(760, static_cast<int>(workH * 0.85f));
            m_windowedRect.left = mi.rcWork.left + (workW - w) / 2;
            m_windowedRect.top = mi.rcWork.top + (workH - h) / 2;
            m_windowedRect.right = m_windowedRect.left + w;
            m_windowedRect.bottom = m_windowedRect.top + h;
        } else {
            if (m_windowedRect.left < mi.rcWork.left || m_windowedRect.right > mi.rcWork.right ||
                m_windowedRect.top < mi.rcWork.top || m_windowedRect.bottom > mi.rcWork.bottom) {
                m_windowedRect.left = mi.rcWork.left + (workW - w) / 2;
                m_windowedRect.top = mi.rcWork.top + (workH - h) / 2;
                m_windowedRect.right = m_windowedRect.left + w;
                m_windowedRect.bottom = m_windowedRect.top + h;
            }
        }

        SetWindowPos(
            m_hwnd,
            HWND_NOTOPMOST,
            m_windowedRect.left,
            m_windowedRect.top,
            w,
            h,
            SWP_SHOWWINDOW | SWP_FRAMECHANGED
        );
    } else {
        GetWindowRect(m_hwnd, &m_windowedRect);

        HMONITOR hMon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);

        m_screenX = mi.rcMonitor.left;
        m_screenY = mi.rcMonitor.top;
        int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
        int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;

        m_isFullscreen = true;
        m_hud.SetFullscreenState(true);

        SetWindowPos(
            m_hwnd,
            HWND_TOPMOST,
            m_screenX,
            m_screenY,
            fullW,
            fullH,
            SWP_SHOWWINDOW | SWP_FRAMECHANGED
        );
    }

    SaveSettings();
}
#else
bool ViewerApp::Initialize(const std::wstring& initialFile) {
    LoadSettings();

    if (!m_imageLoader.Initialize()) {
        return false;
    }

    m_platform = new PlatformLinux();
    if (!m_platform->Initialize(1280, 720, m_isFullscreen)) {
        return false;
    }

    m_platform->GetWindowSize(m_screenWidth, m_screenHeight);

    if (!InitGraphics()) {
        return false;
    }

    m_hud.Initialize(m_d2dContext, m_dpiScale);
    m_thumbBar.Initialize(m_d2dContext, m_dpiScale);
    m_cropToolbar.Initialize(m_d2dContext, m_dpiScale);

    m_hud.SetFullscreenState(m_isFullscreen);
    m_hud.SetAspectMode(m_aspectMode);

    m_platform->onResize = [this](int w, int h) {
        if (w > 0 && h > 0) {
            ResizeBuffers(w, h);
            UpdateDpiScale();
            m_thumbBar.SetCurrentIndex(m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
            ResetViewToFit();
            Render();
        }
    };

    m_platform->onPaint = [this]() {
        Render();
    };

    m_platform->onUpdate = [this](float dt) {
        OnUpdate(dt);
    };

    m_platform->onMouseMove = [this](float x, float y) {
        OnMouseMove(x, y);
    };

    m_platform->onMouseDown = [this](int b, float x, float y, bool s, bool a, bool c) {
        OnMouseDown(b, x, y, s, a, c);
    };

    m_platform->onMouseUp = [this](int b, float x, float y) {
        OnMouseUp(b, x, y);
    };

    m_platform->onMouseWheel = [this](short d, float x, float y, bool shift, bool alt, bool ctrl) {
        OnMouseWheel(d, x, y, shift, alt, ctrl);
    };

    m_platform->onKeyDown = [this](int k, wchar_t c, bool s, bool a, bool ct) {
        OnKeyDown(k, c, s, a, ct);
    };

    m_platform->onFileDrop = [this](const std::wstring& path) {
        m_folderNav.LoadFromInitialFile(path);
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        LoadImage(path);
    };

    if (!initialFile.empty()) {
        m_folderNav.LoadFromInitialFile(initialFile);
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        LoadImage(initialFile);
    } else {
        m_folderNav.ScanFolder(L".");
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        if (m_folderNav.HasImages()) {
            LoadImage(m_folderNav.GetCurrentPath());
        }
    }

    m_lastTick = 0;
    Render();
    return true;
}

bool ViewerApp::Initialize(HINSTANCE hInstance, int nCmdShow, const std::wstring& initialFile) {
    (void)hInstance;
    (void)nCmdShow;
    return Initialize(initialFile);
}

bool ViewerApp::InitGraphics() {
    m_d2dContext = new ID2D1DeviceContext();
    m_d2dContext->SetViewport(m_screenWidth, m_screenHeight);

    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.85f, 0.85f, 0.85f), &m_emptyPromptBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.16f), &m_shadowBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.45f), &m_selectionBorderBrush);

    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.65f), &m_cropMaskBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.85f, 1.0f, 0.95f), &m_cropBorderBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.35f), &m_cropGridBrush);
    m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f), &m_cropHandleBrush);
    return true;
}

void ViewerApp::ResizeBuffers(UINT width, UINT height) {
    if (width == 0 || height == 0) return;
    m_screenWidth = static_cast<int>(width);
    m_screenHeight = static_cast<int>(height);
    if (m_d2dContext) {
        m_d2dContext->SetViewport(m_screenWidth, m_screenHeight);
    }
}

void ViewerApp::ToggleFullscreen() {
    m_isFullscreen = !m_isFullscreen;
    if (m_platform) {
        m_platform->SetFullscreen(m_isFullscreen);
    }
    m_hud.SetFullscreenState(m_isFullscreen);
    SaveSettings();
    ResetViewToFit();
    Invalidate();
}
#endif

void ViewerApp::CycleAspectMode() {
    switch (m_aspectMode) {
    case AspectMode::Fit:      m_aspectMode = AspectMode::Fill; break;
    case AspectMode::Fill:     m_aspectMode = AspectMode::Stretch; break;
    case AspectMode::Stretch:  m_aspectMode = AspectMode::Original; break;
    case AspectMode::Original: m_aspectMode = AspectMode::Fit; break;
    }
    m_hud.SetAspectMode(m_aspectMode);
    ResetViewToFit();
    Render();
}

void ViewerApp::SetAspectMode(AspectMode mode) {
    m_aspectMode = mode;
    m_hud.SetAspectMode(m_aspectMode);
    ResetViewToFit();
    Render();
}

void ViewerApp::ToggleLanguage() {
    Localization::ToggleLanguage();
    SaveSettings();
    if (m_d2dContext) {
        m_cropToolbar.SetDpiScale(m_d2dContext, m_dpiScale);
    }
    m_hud.ShowToast(Localization::Get(StringId::ToastLangSwitched));
    m_hud.ResetIdleTimer();
    Render();
}

void ViewerApp::Invalidate() {
    m_needsRepaint = true;
#if !defined(_WIN32)
    if (m_platform) {
        m_platform->Invalidate();
    }
#endif
}

void ViewerApp::LoadImage(const std::wstring& path) {
    // Release previous GIF frames
    for (auto& f : m_gifFrames) {
        if (f.bitmap) {
            f.bitmap->Release();
        }
    }
    m_gifFrames.clear();

    if (m_currentBitmap && !m_isGif) {
        m_currentBitmap->Release();
    }
    m_currentBitmap = nullptr;
    m_imageWidth = 0;
    m_imageHeight = 0;
    m_isGif = false;
    m_currentGifFrame = 0;
    m_gifTimer = 0.0f;
    m_imagePixels.clear();
    m_undoStack.clear();
    m_isCropping = false;
    m_isErasing = false;
    m_hud.SetCropActive(false);
    m_hud.SetEraseActive(false);
    m_hud.SetCanUndo(false);

    if (m_imageLoader.LoadImageFromFile(m_d2dContext, path, &m_currentBitmap, &m_imageWidth, &m_imageHeight, &m_gifFrames, &m_imagePixels)) {
        m_isGif = (m_gifFrames.size() > 1);
        m_rotation = 0.0f;
        ResetViewToFit();
        UpdateImageInfoString();
        m_thumbBar.SetCurrentIndex(m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
    } else {
        m_imageInfoString = std::wstring(Localization::Get(StringId::ImageLoadError)) + path;
    }

    m_hud.ResetIdleTimer();
    Render();
}

void ViewerApp::UpdateImageInfoString() {
    std::wstring name = m_folderNav.GetCurrentFileName();
    if (m_imageWidth > 0 && m_imageHeight > 0) {
        wchar_t buf[128];
        swprintf_s(buf, L" - %u \x00D7 %u", m_imageWidth, m_imageHeight);
        name += buf;
    }
    m_imageInfoString = name;
}

void ViewerApp::UpdateBitmapFromPixels(UINT w, UINT h) {
    if (!m_d2dContext || m_imagePixels.empty() || w == 0 || h == 0) return;
    if (m_currentBitmap && !m_isGif) {
        m_currentBitmap->Release();
        m_currentBitmap = nullptr;
    }
    m_imageWidth = w;
    m_imageHeight = h;
    UINT stride = w * sizeof(uint32_t);
    D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_NONE,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );
    m_d2dContext->CreateBitmap(
        D2D1::SizeU(w, h),
        m_imagePixels.data(),
        stride,
        &bp,
        &m_currentBitmap
    );
    UpdateImageInfoString();
    m_hud.SetCanUndo(!m_undoStack.empty());
    Invalidate();
}

void ViewerApp::PushUndoState(const std::wstring& note) {
    if (m_imagePixels.empty() || m_imageWidth == 0 || m_imageHeight == 0) return;
    if (m_undoStack.size() >= 15) {
        m_undoStack.erase(m_undoStack.begin());
    }
    ImageHistoryState state;
    state.pixels = m_imagePixels;
    state.width = m_imageWidth;
    state.height = m_imageHeight;
    state.note = note;
    m_undoStack.push_back(std::move(state));
    m_hud.SetCanUndo(true);
}

void ViewerApp::Undo() {
    if (m_undoStack.empty()) {
        m_hud.ShowToast(Localization::Get(StringId::ToastNoUndo));
        return;
    }
    ImageHistoryState prev = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    m_imagePixels = std::move(prev.pixels);
    UpdateBitmapFromPixels(prev.width, prev.height);
    m_hud.SetCanUndo(!m_undoStack.empty());
    m_hud.ShowToast(Localization::Get(StringId::ToastUndone));
    Render();
}

bool ViewerApp::ScreenToImagePixel(float screenX, float screenY, int& outPx, int& outPy) const {
    if (!m_currentBitmap || m_imageWidth == 0 || m_imageHeight == 0) return false;
    float cx = m_screenWidth / 2.0f + m_panX;
    float cy = m_screenHeight / 2.0f + m_panY;
    float dx = screenX - cx;
    float dy = screenY - cy;

    float rad = -m_rotation * 3.1415926535f / 180.0f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);

    float scaleX = m_zoom;
    float scaleY = m_zoom;
    if (m_aspectMode == AspectMode::Stretch) {
        scaleX = static_cast<float>(m_screenWidth) / m_imageWidth;
        scaleY = static_cast<float>(m_screenHeight) / m_imageHeight;
    }

    float halfW = (m_imageWidth * scaleX) / 2.0f;
    float halfH = (m_imageHeight * scaleY) / 2.0f;

    if (rx < -halfW || rx > halfW || ry < -halfH || ry > halfH) return false;

    float normX = (rx + halfW) / (m_imageWidth * scaleX);
    float normY = (ry + halfH) / (m_imageHeight * scaleY);

    outPx = std::clamp(static_cast<int>(normX * m_imageWidth), 0, static_cast<int>(m_imageWidth - 1));
    outPy = std::clamp(static_cast<int>(normY * m_imageHeight), 0, static_cast<int>(m_imageHeight - 1));
    return true;
}

D2D1_POINT_2F ViewerApp::ImagePixelToScreen(float px, float py) const {
    float scaleX = m_zoom;
    float scaleY = m_zoom;
    if (m_aspectMode == AspectMode::Stretch) {
        scaleX = static_cast<float>(m_screenWidth) / m_imageWidth;
        scaleY = static_cast<float>(m_screenHeight) / m_imageHeight;
    }

    float halfW = (m_imageWidth * scaleX) / 2.0f;
    float halfH = (m_imageHeight * scaleY) / 2.0f;

    float rx = (px / (m_imageWidth > 0 ? m_imageWidth : 1.0f)) * (m_imageWidth * scaleX) - halfW;
    float ry = (py / (m_imageHeight > 0 ? m_imageHeight : 1.0f)) * (m_imageHeight * scaleY) - halfH;

    float rad = m_rotation * 3.1415926535f / 180.0f;
    float sx = rx * cosf(rad) - ry * sinf(rad);
    float sy = rx * sinf(rad) + ry * cosf(rad);

    return D2D1::Point2F(m_screenWidth / 2.0f + m_panX + sx, m_screenHeight / 2.0f + m_panY + sy);
}

void ViewerApp::ToggleCropMode() {
    if (m_isCropping) {
        CancelCrop();
    } else {
        if (m_imagePixels.empty() || m_imageWidth == 0 || m_imageHeight == 0) return;
        m_isCropping = true;
        m_isErasing = false;
        m_activeCropRatio = CropRatio::Free;
        m_isCropSymmetric = false;
        m_cropToolbar.SetCropRatio(CropRatio::Free);
        m_cropToolbar.SetSymmetric(false);
        m_cropNormRect = D2D1::RectF(0.08f, 0.08f, 0.92f, 0.92f);
        m_hud.SetCropActive(true);
        m_hud.SetEraseActive(false);
        m_hud.ShowToast(Localization::Get(StringId::ToastCropMode));
        Render();
    }
}

void ViewerApp::ResetCropBox() {
    if (!m_isCropping || m_imageWidth == 0 || m_imageHeight == 0) return;
    if (m_activeCropRatio == CropRatio::Free) {
        m_cropNormRect = D2D1::RectF(0.08f, 0.08f, 0.92f, 0.92f);
    } else {
        SetCropAspectRatio(m_activeCropRatio);
    }
    m_hud.ShowToast(Localization::Get(StringId::ToastCropReset));
    Render();
}

void ViewerApp::SetCropAspectRatio(CropRatio ratio) {
    if (!m_isCropping || m_imageWidth == 0 || m_imageHeight == 0) return;

    m_activeCropRatio = ratio;
    m_cropToolbar.SetCropRatio(ratio);

    if (ratio == CropRatio::Free) {
        m_hud.ShowToast(Localization::Get(StringId::CropRatioFree));
        Render();
        return;
    }

    float targetAspect = 1.0f;
    switch (ratio) {
    case CropRatio::Original:
        targetAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
        break;
    case CropRatio::Ratio1x1:
        targetAspect = 1.0f;
        break;
    case CropRatio::Ratio16x9:
        targetAspect = 16.0f / 9.0f;
        break;
    case CropRatio::Ratio9x16:
        targetAspect = 9.0f / 16.0f;
        break;
    case CropRatio::Ratio4x3:
        targetAspect = 4.0f / 3.0f;
        break;
    case CropRatio::Ratio3x2:
        targetAspect = 3.0f / 2.0f;
        break;
    default:
        break;
    }

    float imgAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
    float k = targetAspect / (imgAspect > 0.0001f ? imgAspect : 1.0f);

    float normW = 0.88f;
    float normH = 0.88f;

    if (k <= 1.0f) {
        normH = 0.88f;
        normW = normH * k;
    } else {
        normW = 0.88f;
        normH = normW / k;
    }

    float left = 0.5f - normW / 2.0f;
    float right = 0.5f + normW / 2.0f;
    float top = 0.5f - normH / 2.0f;
    float bottom = 0.5f + normH / 2.0f;

    m_cropNormRect = D2D1::RectF(left, top, right, bottom);
    m_hud.ShowToast(Localization::Get(StringId::ToastCropRatioSet));
    Render();
}

void ViewerApp::ApplyCrop() {
    if (!m_isCropping || m_imagePixels.empty() || m_imageWidth == 0 || m_imageHeight == 0) return;

    float leftNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
    float rightNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
    float topNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
    float botNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);

    int cx = std::clamp(static_cast<int>(leftNorm * m_imageWidth), 0, static_cast<int>(m_imageWidth - 1));
    int cy = std::clamp(static_cast<int>(topNorm * m_imageHeight), 0, static_cast<int>(m_imageHeight - 1));
    int cw = std::clamp(static_cast<int>((rightNorm - leftNorm) * m_imageWidth), 1, static_cast<int>(m_imageWidth - cx));
    int ch = std::clamp(static_cast<int>((botNorm - topNorm) * m_imageHeight), 1, static_cast<int>(m_imageHeight - cy));

    if (cw < 4 || ch < 4) {
        CancelCrop();
        return;
    }

    PushUndoState(L"Kırpma");

    std::vector<uint32_t> newPixels(cw * ch);
    for (int y = 0; y < ch; ++y) {
        for (int x = 0; x < cw; ++x) {
            newPixels[y * cw + x] = m_imagePixels[(cy + y) * m_imageWidth + (cx + x)];
        }
    }

    m_imagePixels = std::move(newPixels);
    m_isCropping = false;
    m_hud.SetCropActive(false);

    UpdateBitmapFromPixels(cw, ch);
    ResetViewToFit();
    m_hud.ShowToast(Localization::Get(StringId::ToastImageCropped));
    Render();
}

void ViewerApp::CancelCrop() {
    m_isCropping = false;
    m_hud.SetCropActive(false);
    Render();
}

void ViewerApp::ToggleEraseMode() {
    if (m_isErasing) {
        m_isErasing = false;
        m_hud.SetEraseActive(false);
    } else {
        if (m_imagePixels.empty()) return;
        m_isErasing = true;
        m_isCropping = false;
        m_hud.SetEraseActive(true);
        m_hud.SetCropActive(false);
        m_hud.ShowToast(Localization::Get(StringId::ToastEraseMode));
    }
    Render();
}

void ViewerApp::MagicEraseAt(float screenX, float screenY, bool globalAll) {
    int px = 0, py = 0;
    if (!ScreenToImagePixel(screenX, screenY, px, py)) return;
    if (px < 0 || px >= static_cast<int>(m_imageWidth) || py < 0 || py >= static_cast<int>(m_imageHeight)) return;

    uint32_t targetColor = m_imagePixels[py * m_imageWidth + px];
    BYTE targetB = static_cast<BYTE>(targetColor & 0xFF);
    BYTE targetG = static_cast<BYTE>((targetColor >> 8) & 0xFF);
    BYTE targetR = static_cast<BYTE>((targetColor >> 16) & 0xFF);
    BYTE targetA = static_cast<BYTE>((targetColor >> 24) & 0xFF);

    if (targetA == 0) return;

    PushUndoState(L"Sihirli Silgi");

    float tolSq = m_eraseTolerance * m_eraseTolerance;
    auto colorMatches = [&](uint32_t c) -> bool {
        BYTE a = static_cast<BYTE>((c >> 24) & 0xFF);
        if (a == 0) return false;
        BYTE b = static_cast<BYTE>(c & 0xFF);
        BYTE g = static_cast<BYTE>((c >> 8) & 0xFF);
        BYTE r = static_cast<BYTE>((c >> 16) & 0xFF);
        float db = static_cast<float>(b - targetB);
        float dg = static_cast<float>(g - targetG);
        float dr = static_cast<float>(r - targetR);
        return (dr * dr + dg * dg + db * db) <= tolSq;
    };

    int w = static_cast<int>(m_imageWidth);
    int h = static_cast<int>(m_imageHeight);

    if (globalAll) {
        for (int i = 0; i < w * h; ++i) {
            if (colorMatches(m_imagePixels[i])) {
                m_imagePixels[i] = 0;
            }
        }
    } else {
        std::vector<bool> visited(w * h, false);
        std::deque<std::pair<int, int>> queue;
        queue.push_back({ px, py });
        visited[py * w + px] = true;

        while (!queue.empty()) {
            auto [cx, cy] = queue.front();
            queue.pop_front();

            m_imagePixels[cy * w + cx] = 0;

            const int dx[] = { 1, -1, 0, 0 };
            const int dy[] = { 0, 0, 1, -1 };

            for (int d = 0; d < 4; ++d) {
                int nx = cx + dx[d];
                int ny = cy + dy[d];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int nidx = ny * w + nx;
                    if (!visited[nidx] && colorMatches(m_imagePixels[nidx])) {
                        visited[nidx] = true;
                        queue.push_back({ nx, ny });
                    }
                }
            }
        }
    }

    UpdateBitmapFromPixels(m_imageWidth, m_imageHeight);
    m_hud.ShowToast(Localization::Get(StringId::ToastAreaErased));
    Render();
}

void ViewerApp::AutoRemoveBackground() {
    if (m_imagePixels.empty() || m_imageWidth == 0 || m_imageHeight == 0) return;
    PushUndoState(L"Otomatik Arka Plan Sil");

    int w = static_cast<int>(m_imageWidth);
    int h = static_cast<int>(m_imageHeight);

    std::vector<std::pair<int, int>> corners = {
        { 0, 0 }, { w - 1, 0 }, { 0, h - 1 }, { w - 1, h - 1 }
    };

    std::vector<bool> visited(w * h, false);
    float tolSq = m_eraseTolerance * m_eraseTolerance;

    for (auto& corner : corners) {
        int startX = corner.first;
        int startY = corner.second;
        int startIdx = startY * w + startX;
        if (visited[startIdx]) continue;

        uint32_t seedColor = m_imagePixels[startIdx];
        BYTE seedB = static_cast<BYTE>(seedColor & 0xFF);
        BYTE seedG = static_cast<BYTE>((seedColor >> 8) & 0xFF);
        BYTE seedR = static_cast<BYTE>((seedColor >> 16) & 0xFF);
        BYTE seedA = static_cast<BYTE>((seedColor >> 24) & 0xFF);
        if (seedA == 0) continue;

        std::deque<std::pair<int, int>> queue;
        queue.push_back({ startX, startY });
        visited[startIdx] = true;

        while (!queue.empty()) {
            auto [cx, cy] = queue.front();
            queue.pop_front();

            m_imagePixels[cy * w + cx] = 0;

            const int dx[] = { 1, -1, 0, 0 };
            const int dy[] = { 0, 0, 1, -1 };

            for (int d = 0; d < 4; ++d) {
                int nx = cx + dx[d];
                int ny = cy + dy[d];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int nidx = ny * w + nx;
                    if (!visited[nidx]) {
                        uint32_t c = m_imagePixels[nidx];
                        BYTE b = static_cast<BYTE>(c & 0xFF);
                        BYTE g = static_cast<BYTE>((c >> 8) & 0xFF);
                        BYTE r = static_cast<BYTE>((c >> 16) & 0xFF);
                        BYTE a = static_cast<BYTE>((c >> 24) & 0xFF);
                        if (a > 0) {
                            float db = static_cast<float>(b - seedB);
                            float dg = static_cast<float>(g - seedG);
                            float dr = static_cast<float>(r - seedR);
                            if ((dr * dr + dg * dg + db * db) <= tolSq) {
                                visited[nidx] = true;
                                queue.push_back({ nx, ny });
                            }
                        }
                    }
                }
            }
        }
    }

    UpdateBitmapFromPixels(m_imageWidth, m_imageHeight);
    m_hud.ShowToast(Localization::Get(StringId::ToastBgRemoved));
    Render();
}

void ViewerApp::SaveAs() {
    if (m_imagePixels.empty() || m_imageWidth == 0 || m_imageHeight == 0) {
        m_hud.ShowToast(Localization::Get(StringId::ToastNoImage));
        return;
    }

#if defined(_WIN32)
    std::wstring currentPath = m_folderNav.GetCurrentPath();
    wchar_t szFile[MAX_PATH] = {};
    std::wstring defName = L"image_edited.png";

    if (!currentPath.empty()) {
        wchar_t fname[MAX_PATH] = {};
        wchar_t ext[MAX_PATH] = {};
        _wsplitpath_s(currentPath.c_str(), nullptr, 0, nullptr, 0, fname, MAX_PATH, ext, MAX_PATH);
        defName = std::wstring(fname) + L"_edited.png";
    }
    wcscpy_s(szFile, defName.c_str());

    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = Localization::Get(StringId::DialogSaveFilter);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"png";
    ofn.lpstrTitle = Localization::Get(StringId::DialogSaveTitle);

    if (GetSaveFileNameW(&ofn)) {
        std::wstring savePath = szFile;
        GUID format = GUID_ContainerFormatPng;
        if (savePath.length() >= 4) {
            std::wstring extLower = savePath.substr(savePath.length() - 4);
            for (auto& c : extLower) c = towlower(c);
            if (extLower == L".jpg" || (savePath.length() >= 5 && savePath.substr(savePath.length() - 5) == L".jpeg")) {
                format = GUID_ContainerFormatJpeg;
            }
        }

        if (m_imageLoader.SavePixelsToFile(savePath, m_imagePixels.data(), m_imageWidth, m_imageHeight, format)) {
            m_folderNav.LoadFromInitialFile(savePath);
            m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
            m_hud.ShowToast(Localization::Get(StringId::ToastSavedSuccess));
        } else {
            m_hud.ShowToast(Localization::Get(StringId::ToastSavedError));
        }
        Render();
    }
#else
    if (!m_platform) return;
    std::wstring savePath = m_platform->SaveFileDialog(Localization::Get(StringId::DialogSaveTitle), L"png");
    if (!savePath.empty()) {
        GUID format = GUID_ContainerFormatPng;
        if (savePath.length() >= 4) {
            std::wstring extLower = savePath.substr(savePath.length() - 4);
            for (auto& c : extLower) c = towlower(c);
            if (extLower == L".jpg" || (savePath.length() >= 5 && savePath.substr(savePath.length() - 5) == L".jpeg")) {
                format = GUID_ContainerFormatJpeg;
            }
        }

        if (m_imageLoader.SavePixelsToFile(savePath, m_imagePixels.data(), m_imageWidth, m_imageHeight, format)) {
            m_folderNav.LoadFromInitialFile(savePath);
            m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
            m_hud.ShowToast(Localization::Get(StringId::ToastSavedSuccess));
        } else {
            m_hud.ShowToast(Localization::Get(StringId::ToastSavedError));
        }
        Render();
    }
#endif
}

float ViewerApp::CalculateFitScale() const {
    if (m_imageWidth == 0 || m_imageHeight == 0) return 1.0f;

    float marginX = 40.0f * m_dpiScale;
    float marginY = 100.0f * m_dpiScale;

    float effectiveW = static_cast<float>(m_imageWidth);
    float effectiveH = static_cast<float>(m_imageHeight);

    int rotInt = (static_cast<int>(m_rotation) % 360 + 360) % 360;
    if (rotInt == 90 || rotInt == 270) {
        std::swap(effectiveW, effectiveH);
    }

    float availW = static_cast<float>(m_screenWidth) - marginX;
    float availH = static_cast<float>(m_screenHeight) - marginY;
    if (availW <= 0.0f || availH <= 0.0f) return 1.0f;

    float scaleX = availW / effectiveW;
    float scaleY = availH / effectiveH;

    switch (m_aspectMode) {
    case AspectMode::Fit: {
        float maxFit = (std::min)(scaleX, scaleY);
        return (maxFit >= 1.0f) ? 1.0f : maxFit;
    }
    case AspectMode::Fill: {
        // En-boy oranı koru, ekranı tamamen doldur (kırp)
        return (std::max)(scaleX, scaleY);
    }
    case AspectMode::Original: {
        return 1.0f;
    }
    case AspectMode::Stretch: {
        return 1.0f; // Handled as non-uniform scale in Render()
    }
    }

    return 1.0f;
}

void ViewerApp::ResetViewToFit() {
    m_zoom = CalculateFitScale();
    m_panX = 0.0f;
    m_panY = 0.0f;
    Invalidate();
}

void ViewerApp::SetActualSize(float targetScreenX, float targetScreenY) {
    if (targetScreenX < 0 || targetScreenY < 0) {
        targetScreenX = m_screenWidth / 2.0f;
        targetScreenY = m_screenHeight / 2.0f;
    }

    float newZoom = 1.0f;
    if (std::abs(m_zoom - 1.0f) < 0.05f) {
        ResetViewToFit();
        return;
    }

    float ratio = newZoom / m_zoom;
    float origCx = m_screenWidth / 2.0f + m_panX;
    float origCy = m_screenHeight / 2.0f + m_panY;
    float dx = targetScreenX - origCx;
    float dy = targetScreenY - origCy;

    m_panX = targetScreenX - (m_screenWidth / 2.0f) - dx * ratio;
    m_panY = targetScreenY - (m_screenHeight / 2.0f) - dy * ratio;
    m_zoom = newZoom;
    Invalidate();
}

void ViewerApp::ZoomAt(float factor, float cursorX, float cursorY) {
    float newZoom = std::clamp(m_zoom * factor, 0.02f, 40.0f);
    if (newZoom == m_zoom) return;

    float ratio = newZoom / m_zoom;
    float origCx = m_screenWidth / 2.0f + m_panX;
    float origCy = m_screenHeight / 2.0f + m_panY;
    float dx = cursorX - origCx;
    float dy = cursorY - origCy;

    m_panX = cursorX - (m_screenWidth / 2.0f) - dx * ratio;
    m_panY = cursorY - (m_screenHeight / 2.0f) - dy * ratio;
    m_zoom = newZoom;

    m_hud.ResetIdleTimer();
    Render();
}

void ViewerApp::Rotate(float angleDelta) {
    m_rotation += angleDelta;
    while (m_rotation >= 360.0f) m_rotation -= 360.0f;
    while (m_rotation < 0.0f) m_rotation += 360.0f;

    ResetViewToFit();
    m_hud.ResetIdleTimer();
    Render();
}

void ViewerApp::NextImage() {
    if (m_folderNav.Next()) {
        LoadImage(m_folderNav.GetCurrentPath());
    }
}

void ViewerApp::PrevImage() {
    if (m_folderNav.Prev()) {
        LoadImage(m_folderNav.GetCurrentPath());
    }
}

void ViewerApp::FirstImage() {
    if (m_folderNav.First()) {
        LoadImage(m_folderNav.GetCurrentPath());
    }
}

void ViewerApp::LastImage() {
    if (m_folderNav.Last()) {
        LoadImage(m_folderNav.GetCurrentPath());
    }
}

void ViewerApp::OpenFileDialog() {
#if defined(_WIN32)
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = Localization::Get(StringId::DialogOpenFilter);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = Localization::Get(StringId::DialogOpenTitle);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        m_folderNav.LoadFromInitialFile(szFile);
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        LoadImage(szFile);
    }
#else
    if (!m_platform) return;
    std::wstring openPath = m_platform->OpenFileDialog(Localization::Get(StringId::DialogOpenTitle));
    if (!openPath.empty()) {
        m_folderNav.LoadFromInitialFile(openPath);
        m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
        LoadImage(openPath);
    }
#endif
}

bool ViewerApp::IsPointInsideImage(float x, float y) const {
    if (!m_currentBitmap || m_imageWidth == 0 || m_imageHeight == 0) return false;

    float cx = m_screenWidth / 2.0f + m_panX;
    float cy = m_screenHeight / 2.0f + m_panY;
    float dx = x - cx;
    float dy = y - cy;

    float rad = -m_rotation * 3.1415926535f / 180.0f;
    float rx = dx * cosf(rad) - dy * sinf(rad);
    float ry = dx * sinf(rad) + dy * cosf(rad);

    float scaleX = m_zoom;
    float scaleY = m_zoom;
    if (m_aspectMode == AspectMode::Stretch) {
        scaleX = static_cast<float>(m_screenWidth) / m_imageWidth;
        scaleY = static_cast<float>(m_screenHeight) / m_imageHeight;
    }

    float halfW = (m_imageWidth * scaleX) / 2.0f;
    float halfH = (m_imageHeight * scaleY) / 2.0f;

    return (rx >= -halfW && rx <= halfW && ry >= -halfH && ry <= halfH);
}

void ViewerApp::Render() {
#if defined(_WIN32)
    if (!m_d2dContext || !m_swapChain) return;
#else
    if (!m_d2dContext || !m_platform) return;
#endif

    m_d2dContext->BeginDraw();

    // 1. Clear background with Picasa-style dark translucent overlay
    m_d2dContext->Clear(D2D1::ColorF(0.012f, 0.012f, 0.015f, m_bgOpacity));

    if (!m_isFullscreen) {
        D2D1_RECT_F winBorder = D2D1::RectF(0.5f, 0.5f, m_screenWidth - 0.5f, m_screenHeight - 0.5f);
        ID2D1SolidColorBrush* borderBrush = nullptr;
        m_d2dContext->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.35f, 0.40f, 0.60f), &borderBrush);
        if (borderBrush) {
            m_d2dContext->DrawRectangle(&winBorder, borderBrush, 1.0f);
            borderBrush->Release();
        }
    }

    // 2. Draw Image with Elevation Shadow and Hover Glow
    if (m_currentBitmap && m_imageWidth > 0 && m_imageHeight > 0) {
        float cx = m_screenWidth / 2.0f + m_panX;
        float cy = m_screenHeight / 2.0f + m_panY;

        float scaleX = m_zoom;
        float scaleY = m_zoom;
        if (m_aspectMode == AspectMode::Stretch) {
            scaleX = static_cast<float>(m_screenWidth - 20.0f) / m_imageWidth;
            scaleY = static_cast<float>(m_screenHeight - 80.0f) / m_imageHeight;
        }

        D2D1_MATRIX_3X2_F transform =
            D2D1::Matrix3x2F::Rotation(m_rotation, D2D1::Point2F(0.0f, 0.0f)) *
            D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
            D2D1::Matrix3x2F::Translation(cx, cy);

        m_d2dContext->SetTransform(transform);

        float halfW = m_imageWidth / 2.0f;
        float halfH = m_imageHeight / 2.0f;
        D2D1_RECT_F imgRect = D2D1::RectF(-halfW, -halfH, halfW, halfH);

        // A) Picasa 3 Ambient Drop-Shadow (soft multi-layered elevation shadow)
        if (m_shadowBrush) {
            float shadowOffsets[] = { 16.0f, 10.0f, 6.0f, 3.0f };
            float shadowAlphas[]  = { 0.04f,  0.08f, 0.12f, 0.20f };
            for (int s = 0; s < 4; ++s) {
                float off = shadowOffsets[s] / (scaleX > 0.1f ? scaleX : 1.0f);
                D2D1_RECT_F sRect = D2D1::RectF(-halfW - off, -halfH - off + off * 0.5f, halfW + off, halfH + off + off * 0.5f);
                D2D1_ROUNDED_RECT sRRect = D2D1::RoundedRect(sRect, off * 0.8f, off * 0.8f);
                m_shadowBrush->SetOpacity(shadowAlphas[s]);
                m_d2dContext->FillRoundedRectangle(&sRRect, m_shadowBrush);
            }
        }

        // B) Draw Photo (100% opaque, linear interpolation)
        m_d2dContext->DrawBitmap(
            m_currentBitmap,
            imgRect,
            1.0f,
            D2D1_INTERPOLATION_MODE_LINEAR
        );

        // C) Hover Selection Glow / Border (cool visual feedback!)
        if (m_isHoveringImage && m_selectionBorderBrush) {
            float strokeW = 1.5f / (scaleX > 0.1f ? scaleX : 1.0f);
            m_d2dContext->DrawRectangle(&imgRect, m_selectionBorderBrush, strokeW);
        }

        m_d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

        // D) Interactive Crop Overlay
        if (m_isCropping && m_imageWidth > 0 && m_imageHeight > 0) {
            float lNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
            float rNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
            float tNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
            float bNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);

            D2D1_POINT_2F ptTL = ImagePixelToScreen(lNorm * m_imageWidth, tNorm * m_imageHeight);
            D2D1_POINT_2F ptBR = ImagePixelToScreen(rNorm * m_imageWidth, bNorm * m_imageHeight);

            float scrL = (std::min)(ptTL.x, ptBR.x);
            float scrR = (std::max)(ptTL.x, ptBR.x);
            float scrT = (std::min)(ptTL.y, ptBR.y);
            float scrB = (std::max)(ptTL.y, ptBR.y);

            D2D1_RECT_F cropRect = D2D1::RectF(scrL, scrT, scrR, scrB);

            // Dark mask outside crop box
            if (m_cropMaskBrush) {
                D2D1_RECT_F maskTop = D2D1::RectF(0.0f, 0.0f, static_cast<float>(m_screenWidth), scrT);
                D2D1_RECT_F maskBottom = D2D1::RectF(0.0f, scrB, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));
                D2D1_RECT_F maskLeft = D2D1::RectF(0.0f, scrT, scrL, scrB);
                D2D1_RECT_F maskRight = D2D1::RectF(scrR, scrT, static_cast<float>(m_screenWidth), scrB);

                m_d2dContext->FillRectangle(&maskTop, m_cropMaskBrush);
                m_d2dContext->FillRectangle(&maskBottom, m_cropMaskBrush);
                m_d2dContext->FillRectangle(&maskLeft, m_cropMaskBrush);
                m_d2dContext->FillRectangle(&maskRight, m_cropMaskBrush);
            }

            // Rule of thirds grid
            if (m_cropGridBrush) {
                float thirdW = (scrR - scrL) / 3.0f;
                float thirdH = (scrB - scrT) / 3.0f;
                m_d2dContext->DrawLine(D2D1::Point2F(scrL + thirdW, scrT), D2D1::Point2F(scrL + thirdW, scrB), m_cropGridBrush, 1.0f);
                m_d2dContext->DrawLine(D2D1::Point2F(scrL + thirdW * 2.0f, scrT), D2D1::Point2F(scrL + thirdW * 2.0f, scrB), m_cropGridBrush, 1.0f);
                m_d2dContext->DrawLine(D2D1::Point2F(scrL, scrT + thirdH), D2D1::Point2F(scrR, scrT + thirdH), m_cropGridBrush, 1.0f);
                m_d2dContext->DrawLine(D2D1::Point2F(scrL, scrT + thirdH * 2.0f), D2D1::Point2F(scrR, scrT + thirdH * 2.0f), m_cropGridBrush, 1.0f);
            }

            // Crop box border
            if (m_cropBorderBrush) {
                m_d2dContext->DrawRectangle(&cropRect, m_cropBorderBrush, 2.0f);
            }

            // 8 Handles
            if (m_cropHandleBrush) {
                float hSize = 7.0f * m_dpiScale;
                auto drawHandle = [&](float x, float y) {
                    D2D1_RECT_F hRect = D2D1::RectF(x - hSize, y - hSize, x + hSize, y + hSize);
                    m_d2dContext->FillRectangle(&hRect, m_cropHandleBrush);
                    if (m_cropBorderBrush) {
                        m_d2dContext->DrawRectangle(&hRect, m_cropBorderBrush, 1.5f);
                    }
                };

                float midX = (scrL + scrR) / 2.0f;
                float midY = (scrT + scrB) / 2.0f;
                drawHandle(scrL, scrT);
                drawHandle(scrR, scrT);
                drawHandle(scrR, scrB);
                drawHandle(scrL, scrB);
                drawHandle(midX, scrT);
                drawHandle(scrR, midY);
                drawHandle(midX, scrB);
                drawHandle(scrL, midY);
            }
        }
    }

    // 3. Render Thumbnail Filmstrip (Picasa style)
    m_thumbBar.Render(
        m_d2dContext,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight),
        m_hud.GetHudTop(),
        m_hud.GetAlpha()
    );

    // 4. Render HUD (controls + info string)
    m_hud.Render(
        m_d2dContext,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight),
        m_imageInfoString
    );

    // 5. Render Crop Toolbar if cropping is active
    if (m_isCropping) {
        m_cropToolbar.Render(
            m_d2dContext,
            static_cast<float>(m_screenWidth),
            static_cast<float>(m_screenHeight)
        );
    }

    m_d2dContext->EndDraw();

#if defined(_WIN32)
    m_swapChain->Present(0, 0);
#else
    if (m_platform) {
        m_platform->SwapBuffers();
    }
#endif
    m_needsRepaint = false;
}

int ViewerApp::Run() {
#if defined(_WIN32)
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
#else
    while (m_platform && m_platform->PollEvents()) {
    }
    return 0;
#endif
}

static int HitTestCropHandles(float mouseX, float mouseY, float scrL, float scrT, float scrR, float scrB, float dpiScale) {
    float hSize = 12.0f * dpiScale;
    float midX = (scrL + scrR) / 2.0f;
    float midY = (scrT + scrB) / 2.0f;

    auto hit = [&](float x, float y) {
        return (mouseX >= x - hSize && mouseX <= x + hSize && mouseY >= y - hSize && mouseY <= y + hSize);
    };

    if (hit(scrL, scrT)) return 0;
    if (hit(scrR, scrT)) return 1;
    if (hit(scrR, scrB)) return 2;
    if (hit(scrL, scrB)) return 3;
    if (hit(midX, scrT)) return 4;
    if (hit(scrR, midY)) return 5;
    if (hit(midX, scrB)) return 6;
    if (hit(scrL, midY)) return 7;

    if (mouseX >= scrL && mouseX <= scrR && mouseY >= scrT && mouseY <= scrB) {
        return 8; // Move inside crop box
    }
    return -1;
}

#if defined(_WIN32)
LRESULT CALLBACK ViewerApp::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ViewerApp* app = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = reinterpret_cast<ViewerApp*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<ViewerApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (app) {
        return app->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT ViewerApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        Render();
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            float mx = static_cast<float>(pt.x);
            float my = static_cast<float>(pt.y);

            if (m_isCropping && m_cropToolbar.IsMouseOver(mx, my)) {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }

            if (m_isCropping && m_imageWidth > 0 && m_imageHeight > 0 && !m_hud.IsMouseOverHud(mx, my) && !m_thumbBar.IsMouseOver(mx, my)) {
                float lNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
                float rNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
                float tNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
                float bNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);
                D2D1_POINT_2F p0 = ImagePixelToScreen(lNorm * m_imageWidth, tNorm * m_imageHeight);
                D2D1_POINT_2F p1 = ImagePixelToScreen(rNorm * m_imageWidth, bNorm * m_imageHeight);
                float scrL = (std::min)(p0.x, p1.x);
                float scrR = (std::max)(p0.x, p1.x);
                float scrT = (std::min)(p0.y, p1.y);
                float scrB = (std::max)(p0.y, p1.y);

                int handle = HitTestCropHandles(mx, my, scrL, scrT, scrR, scrB, m_dpiScale);
                if (handle == 0 || handle == 2) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                    return TRUE;
                } else if (handle == 1 || handle == 3) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
                    return TRUE;
                } else if (handle == 4 || handle == 6) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    return TRUE;
                } else if (handle == 5 || handle == 7) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    return TRUE;
                } else if (handle == 8) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                    return TRUE;
                }
            }

            if (m_isErasing && IsPointInsideImage(mx, my) && !m_hud.IsMouseOverHud(mx, my) && !m_thumbBar.IsMouseOver(mx, my)) {
                SetCursor(LoadCursor(nullptr, IDC_CROSS));
                return TRUE;
            }

            // 4-way move arrow cursor when hovering or dragging the image!
            if (m_isDragging || (IsPointInsideImage(mx, my) && !m_hud.IsMouseOverHud(mx, my) && !m_thumbBar.IsMouseOver(mx, my))) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                return TRUE;
            } else {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }
        }
        break;
    }

    case WM_SIZE: {
        UINT w = LOWORD(lParam);
        UINT h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            ResizeBuffers(w, h);
            UpdateDpiScale();
            if (!m_isFullscreen) {
                GetWindowRect(hwnd, &m_windowedRect);
            }
            m_thumbBar.SetCurrentIndex(m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
            ResetViewToFit();
            Render();
        }
        return 0;
    }

    case WM_MOVE: {
        if (!m_isFullscreen) {
            GetWindowRect(hwnd, &m_windowedRect);
        }
        return 0;
    }

    case WM_DPICHANGED: {
        UINT newDpi = LOWORD(wParam);
        if (newDpi == 0) newDpi = 96;
        m_dpiScale = static_cast<float>(newDpi) / 96.0f;
        if (m_d2dContext) {
            m_hud.SetDpiScale(m_d2dContext, m_dpiScale);
            m_thumbBar.SetDpiScale(m_d2dContext, m_dpiScale);
            m_cropToolbar.SetDpiScale(m_d2dContext, m_dpiScale);
        }

        if (m_isFullscreen) {
            HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfoW(hMon, &mi);
            m_screenX = mi.rcMonitor.left;
            m_screenY = mi.rcMonitor.top;
            int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
            int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            SetWindowPos(hwnd, HWND_TOPMOST, m_screenX, m_screenY, fullW, fullH, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        } else {
            auto lprc = reinterpret_cast<RECT*>(lParam);
            if (lprc) {
                SetWindowPos(hwnd, nullptr, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        return 0;
    }

    case WM_TIMER: {
        DWORD now = GetTickCount();
        float dt = (now - m_lastTick) / 1000.0f;
        m_lastTick = now;
        if (dt > 0.1f) dt = 0.1f;

        m_hud.Update(dt);
        m_thumbBar.Update(dt, m_d2dContext);

        bool needsRepaint = m_hud.NeedsRedraw() || m_thumbBar.NeedsRedraw() || m_needsRepaint;

        // Animate GIF playback
        if (m_isGif && m_gifFrames.size() > 1) {
            m_gifTimer += dt;
            if (m_gifTimer >= m_gifFrames[m_currentGifFrame].delaySeconds) {
                m_gifTimer = 0.0f;
                m_currentGifFrame = (m_currentGifFrame + 1) % m_gifFrames.size();
                m_currentBitmap = m_gifFrames[m_currentGifFrame].bitmap;
                needsRepaint = true;
            }
        }

        if (needsRepaint) {
            m_hud.ClearNeedsRedraw();
            m_thumbBar.ClearNeedsRedraw();
            Render();
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));

        if (m_activeCropHandle >= 0 && m_imageWidth > 0 && m_imageHeight > 0) {
            float dx = mouseX - m_cropDragStart.x;
            float dy = mouseY - m_cropDragStart.y;
            float imgScreenW = m_imageWidth * m_zoom;
            float imgScreenH = m_imageHeight * m_zoom;
            float dNormX = (imgScreenW > 1.0f) ? (dx / imgScreenW) : 0.0f;
            float dNormY = (imgScreenH > 1.0f) ? (dy / imgScreenH) : 0.0f;

            D2D1_RECT_F r = m_cropRectAtDragStart;
            bool isSymmetric = m_isCropSymmetric || ((GetKeyState(VK_MENU) & 0x8000) != 0);

            if (m_activeCropHandle == 8) {
                // Move whole box
                float w = r.right - r.left;
                float h = r.bottom - r.top;
                float newL = std::clamp(r.left + dNormX, 0.0f, 1.0f - w);
                float newT = std::clamp(r.top + dNormY, 0.0f, 1.0f - h);
                r.left = newL;
                r.top = newT;
                r.right = newL + w;
                r.bottom = newT + h;
            } else if (m_activeCropRatio == CropRatio::Free) {
                if (isSymmetric) {
                    float cx = (r.left + r.right) / 2.0f;
                    float cy = (r.top + r.bottom) / 2.0f;
                    float curHalfW = (r.right - r.left) / 2.0f;
                    float curHalfH = (r.bottom - r.top) / 2.0f;
                    float maxHalfW = (std::min)(cx, 1.0f - cx);
                    float maxHalfH = (std::min)(cy, 1.0f - cy);

                    float deltaX = 0.0f;
                    float deltaY = 0.0f;
                    switch (m_activeCropHandle) {
                    case 0: deltaX = -dNormX; deltaY = -dNormY; break; // TL
                    case 1: deltaX =  dNormX; deltaY = -dNormY; break; // TR
                    case 2: deltaX =  dNormX; deltaY =  dNormY; break; // BR
                    case 3: deltaX = -dNormX; deltaY =  dNormY; break; // BL
                    case 4: deltaX = 0.0f;    deltaY = -dNormY; break; // Top
                    case 5: deltaX =  dNormX; deltaY = 0.0f;    break; // Right
                    case 6: deltaX = 0.0f;    deltaY =  dNormY; break; // Bottom
                    case 7: deltaX = -dNormX; deltaY = 0.0f;    break; // Left
                    }

                    float newHalfW = (deltaX != 0.0f) ? std::clamp(curHalfW + deltaX, 0.02f, maxHalfW) : curHalfW;
                    float newHalfH = (deltaY != 0.0f) ? std::clamp(curHalfH + deltaY, 0.02f, maxHalfH) : curHalfH;

                    r.left = cx - newHalfW;
                    r.right = cx + newHalfW;
                    r.top = cy - newHalfH;
                    r.bottom = cy + newHalfH;
                } else {
                    switch (m_activeCropHandle) {
                    case 0: // TL
                        r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                        r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                        break;
                    case 1: // TR
                        r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                        r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                        break;
                    case 2: // BR
                        r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                        r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                        break;
                    case 3: // BL
                        r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                        r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                        break;
                    case 4: // Top
                        r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                        break;
                    case 5: // Right
                        r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                        break;
                    case 6: // Bottom
                        r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                        break;
                    case 7: // Left
                        r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                        break;
                    }
                }
            } else {
                // Locked aspect ratio mode
                float targetAspect = 1.0f;
                switch (m_activeCropRatio) {
                case CropRatio::Original:
                    targetAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
                    break;
                case CropRatio::Ratio1x1:  targetAspect = 1.0f; break;
                case CropRatio::Ratio16x9: targetAspect = 16.0f / 9.0f; break;
                case CropRatio::Ratio9x16: targetAspect = 9.0f / 16.0f; break;
                case CropRatio::Ratio4x3:  targetAspect = 4.0f / 3.0f; break;
                case CropRatio::Ratio3x2:  targetAspect = 3.0f / 2.0f; break;
                default: break;
                }
                float imgAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
                float k = targetAspect / (imgAspect > 0.0001f ? imgAspect : 1.0f);

                if (isSymmetric) {
                    float cx = (r.left + r.right) / 2.0f;
                    float cy = (r.top + r.bottom) / 2.0f;
                    float maxHalfW = (std::min)(cx, 1.0f - cx);
                    float maxHalfH = (std::min)(cy, 1.0f - cy);
                    float maxH = (std::min)(maxHalfH, maxHalfW / k);
                    float maxW = maxH * k;

                    float delta = 0.0f;
                    switch (m_activeCropHandle) {
                    case 0: delta = (-dNormX - dNormY * k) / 2.0f; break; // TL
                    case 1: delta = ( dNormX - dNormY * k) / 2.0f; break; // TR
                    case 2: delta = ( dNormX + dNormY * k) / 2.0f; break; // BR
                    case 3: delta = (-dNormX + dNormY * k) / 2.0f; break; // BL
                    case 4: delta = -dNormY * k; break; // Top
                    case 5: delta =  dNormX;     break; // Right
                    case 6: delta =  dNormY * k; break; // Bottom
                    case 7: delta = -dNormX;     break; // Left
                    }

                    float curHalfW = (r.right - r.left) / 2.0f;
                    float newHalfW = std::clamp(curHalfW + delta, 0.02f, maxW);
                    float newHalfH = newHalfW / k;

                    r.left = cx - newHalfW;
                    r.right = cx + newHalfW;
                    r.top = cy - newHalfH;
                    r.bottom = cy + newHalfH;
                } else {
                    float curW = r.right - r.left;
                    switch (m_activeCropHandle) {
                    case 2: // BR
                    case 5: // Right
                    case 6: { // Bottom
                        float maxW = (std::min)(1.0f - r.left, (1.0f - r.top) * k);
                        float delta = (m_activeCropHandle == 5) ? dNormX : (m_activeCropHandle == 6 ? dNormY * k : (dNormX + dNormY * k) / 2.0f);
                        float newW = std::clamp(curW + delta, 0.02f, maxW);
                        float newH = newW / k;
                        r.right = r.left + newW;
                        r.bottom = r.top + newH;
                        break;
                    }
                    case 0: // TL
                    case 4: // Top
                    case 7: { // Left
                        float maxW = (std::min)(r.right, r.bottom * k);
                        float delta = (m_activeCropHandle == 7) ? -dNormX : (m_activeCropHandle == 4 ? -dNormY * k : (-dNormX - dNormY * k) / 2.0f);
                        float newW = std::clamp(curW + delta, 0.02f, maxW);
                        float newH = newW / k;
                        r.left = r.right - newW;
                        r.top = r.bottom - newH;
                        break;
                    }
                    case 1: { // TR
                        float maxW = (std::min)(1.0f - r.left, r.bottom * k);
                        float newW = std::clamp(curW + (dNormX - dNormY * k) / 2.0f, 0.02f, maxW);
                        float newH = newW / k;
                        r.right = r.left + newW;
                        r.top = r.bottom - newH;
                        break;
                    }
                    case 3: { // BL
                        float maxW = (std::min)(r.right, (1.0f - r.top) * k);
                        float newW = std::clamp(curW + (-dNormX + dNormY * k) / 2.0f, 0.02f, maxW);
                        float newH = newW / k;
                        r.left = r.right - newW;
                        r.bottom = r.top + newH;
                        break;
                    }
                    }
                }
            }

            m_cropNormRect = r;
            Render();
            return 0;
        }

        // Update hover state for 4-way arrow cursor and subtle glow
        bool isHoverImg = IsPointInsideImage(mouseX, mouseY) && !m_hud.IsMouseOverHud(mouseX, mouseY) && !m_thumbBar.IsMouseOver(mouseX, mouseY) && (!m_isCropping || !m_cropToolbar.IsMouseOver(mouseX, mouseY));
        if (m_isHoveringImage != isHoverImg) {
            m_isHoveringImage = isHoverImg;
            Render();
        }

        if (m_isDragging) {
            m_panX = m_dragStartPanX + (mouseX - m_dragStartMouse.x);
            m_panY = m_dragStartPanY + (mouseY - m_dragStartMouse.y);
            m_hud.ResetIdleTimer();
            Render();
        } else {
            m_hud.OnMouseMove(mouseX, mouseY);
            m_thumbBar.OnMouseMove(mouseX, mouseY);
            if (m_isCropping) {
                m_cropToolbar.OnMouseMove(mouseX, mouseY);
            }
            if (m_hud.NeedsRedraw() || m_thumbBar.IsMouseOver(mouseX, mouseY) || (m_isCropping && m_cropToolbar.NeedsRedraw())) {
                m_hud.ClearNeedsRedraw();
                if (m_isCropping) m_cropToolbar.ClearNeedsRedraw();
                Render();
            }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));

        if (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY)) {
            if (m_cropToolbar.OnMouseDown(mouseX, mouseY)) {
                Render();
                return 0;
            }
        }

        if (m_hud.IsMouseOverHud(mouseX, mouseY)) {
            m_hud.OnMouseDown(mouseX, mouseY);
            Render();
            return 0;
        }

        if (m_thumbBar.IsMouseOver(mouseX, mouseY)) {
            int clickedIdx = m_thumbBar.OnMouseDown(mouseX, mouseY);
            if (clickedIdx >= 0) {
                m_folderNav.SetIndex(static_cast<size_t>(clickedIdx));
                LoadImage(m_folderNav.GetCurrentPath());
            }
            return 0;
        }

        if (m_isCropping && m_imageWidth > 0 && m_imageHeight > 0) {
            float lNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
            float rNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
            float tNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
            float bNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);
            D2D1_POINT_2F p0 = ImagePixelToScreen(lNorm * m_imageWidth, tNorm * m_imageHeight);
            D2D1_POINT_2F p1 = ImagePixelToScreen(rNorm * m_imageWidth, bNorm * m_imageHeight);
            float scrL = (std::min)(p0.x, p1.x);
            float scrR = (std::max)(p0.x, p1.x);
            float scrT = (std::min)(p0.y, p1.y);
            float scrB = (std::max)(p0.y, p1.y);

            int handle = HitTestCropHandles(mouseX, mouseY, scrL, scrT, scrR, scrB, m_dpiScale);
            if (handle >= 0) {
                m_activeCropHandle = handle;
                m_cropDragStart = D2D1::Point2F(mouseX, mouseY);
                m_cropRectAtDragStart = m_cropNormRect;
                SetCapture(hwnd);
                return 0;
            }
        }

        bool insideImage = IsPointInsideImage(mouseX, mouseY);

        if (m_isErasing && insideImage) {
            bool globalAll = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            MagicEraseAt(mouseX, mouseY, globalAll);
            return 0;
        }

        if (!m_isFullscreen && !insideImage) {
            ReleaseCapture();
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        m_isDragging = true;
        m_dragStartMouse.x = static_cast<LONG>(mouseX);
        m_dragStartMouse.y = static_cast<LONG>(mouseY);
        m_dragStartPanX = m_panX;
        m_dragStartPanY = m_panY;
        SetCapture(hwnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));

        if (m_activeCropHandle >= 0) {
            m_activeCropHandle = -1;
            ReleaseCapture();
            return 0;
        }

        if (m_isDragging) {
            m_isDragging = false;
            ReleaseCapture();
        }

        if (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY)) {
            CropRatio selectedRatio = CropRatio::Free;
            CropAction cAction = m_cropToolbar.OnMouseUp(mouseX, mouseY, &selectedRatio);
            switch (cAction) {
            case CropAction::SetRatio:
                SetCropAspectRatio(selectedRatio);
                break;
            case CropAction::ToggleSymmetric:
                m_isCropSymmetric = !m_isCropSymmetric;
                m_cropToolbar.SetSymmetric(m_isCropSymmetric);
                m_hud.ShowToast(Localization::Get(m_isCropSymmetric ? StringId::ToastCropSymmetricOn : StringId::ToastCropSymmetricOff));
                Render();
                break;
            case CropAction::Reset:
                ResetCropBox();
                break;
            case CropAction::Apply:
                ApplyCrop();
                break;
            case CropAction::Cancel:
                CancelCrop();
                break;
            default:
                break;
            }
            return 0;
        }

        HudAction action = m_hud.OnMouseUp(mouseX, mouseY);
        switch (action) {
        case HudAction::Prev: PrevImage(); break;
        case HudAction::Next: NextImage(); break;
        case HudAction::ZoomIn: ZoomAt(1.25f, m_screenWidth / 2.0f, m_screenHeight / 2.0f); break;
        case HudAction::ZoomOut: ZoomAt(1.0f / 1.25f, m_screenWidth / 2.0f, m_screenHeight / 2.0f); break;
        case HudAction::CycleAspectMode: CycleAspectMode(); break;
        case HudAction::ActualSize: SetActualSize(); Render(); break;
        case HudAction::RotateLeft: Rotate(-90.0f); break;
        case HudAction::RotateRight: Rotate(90.0f); break;
        case HudAction::Crop: ToggleCropMode(); break;
        case HudAction::MagicErase: ToggleEraseMode(); break;
        case HudAction::AutoBgRemove: AutoRemoveBackground(); break;
        case HudAction::Undo: Undo(); break;
        case HudAction::SaveAs: SaveAs(); break;
        case HudAction::ToggleLanguage: ToggleLanguage(); break;
        case HudAction::ToggleFullscreen: ToggleFullscreen(); break;
        case HudAction::Close: PostQuitMessage(0); break;
        default: break;
        }

        if (m_hud.NeedsRedraw()) {
            m_hud.ClearNeedsRedraw();
            Render();
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        float mouseX = static_cast<float>(GET_X_LPARAM(lParam));
        float mouseY = static_cast<float>(GET_Y_LPARAM(lParam));

        if (m_hud.IsMouseOverHud(mouseX, mouseY) || m_thumbBar.IsMouseOver(mouseX, mouseY) || (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY))) {
            return 0;
        }

        if (m_isCropping) {
            ApplyCrop();
            return 0;
        }

        if (IsPointInsideImage(mouseX, mouseY)) {
            SetActualSize(mouseX, mouseY);
            Render();
        } else {
            ToggleFullscreen();
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);

        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        OnMouseWheel(delta, static_cast<float>(pt.x), static_cast<float>(pt.y), shift, alt, ctrl);
        return 0;
    }

    case WM_KEYDOWN: {
        m_hud.ResetIdleTimer();

        if (GetKeyState(VK_CONTROL) & 0x8000) {
            if (wParam == 'S') {
                SaveAs();
                return 0;
            } else if (wParam == 'Z') {
                Undo();
                return 0;
            }
        }

        switch (wParam) {
        case 'C':
            ToggleCropMode();
            return 0;
        case 'S':
            if (m_isCropping) {
                m_isCropSymmetric = !m_isCropSymmetric;
                m_cropToolbar.SetSymmetric(m_isCropSymmetric);
                m_hud.ShowToast(Localization::Get(m_isCropSymmetric ? StringId::ToastCropSymmetricOn : StringId::ToastCropSymmetricOff));
                Render();
                return 0;
            }
            break;
        case 'E':
            ToggleEraseMode();
            return 0;
        case 'B':
            AutoRemoveBackground();
            return 0;
        case 'T':
            ToggleLanguage();
            return 0;
        case VK_ESCAPE:
            if (m_isCropping) {
                CancelCrop();
            } else if (m_isErasing) {
                ToggleEraseMode();
            } else {
                PostQuitMessage(0);
            }
            return 0;
        case VK_F11:
            ToggleFullscreen();
            return 0;
        case VK_RETURN:
            if (m_isCropping) {
                ApplyCrop();
            } else {
                ToggleFullscreen();
            }
            return 0;
        case 'M':
            CycleAspectMode();
            return 0;
        case VK_LEFT:
        case 'A':
            PrevImage();
            return 0;
        case VK_RIGHT:
        case 'D':
            NextImage();
            return 0;
        case VK_UP:
        case VK_OEM_PLUS:
        case VK_ADD:
            ZoomAt(1.20f, m_screenWidth / 2.0f, m_screenHeight / 2.0f);
            return 0;
        case VK_DOWN:
        case VK_OEM_MINUS:
        case VK_SUBTRACT:
            ZoomAt(1.0f / 1.20f, m_screenWidth / 2.0f, m_screenHeight / 2.0f);
            return 0;
        case 'R':
            Rotate(90.0f);
            return 0;
        case 'L':
            Rotate(-90.0f);
            return 0;
        case 'F':
        case '0':
            ResetViewToFit();
            Render();
            return 0;
        case '1':
            SetActualSize();
            Render();
            return 0;
        case 'O':
            OpenFileDialog();
            return 0;
        case VK_HOME:
            FirstImage();
            return 0;
        case VK_END:
            LastImage();
            return 0;
        case VK_OEM_4: // '[' - decrease darkness (more transparent)
            m_bgOpacity = (std::max)(0.05f, m_bgOpacity - 0.05f);
            m_hud.ShowToast(Localization::GetCurrentLanguage() == Language::Turkish ?
                (L"Saydamlık: %" + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f))) :
                (L"Transparency: " + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f)) + L"%"));
            SaveSettings();
            Render();
            return 0;
        case VK_OEM_6: // ']' - increase darkness
            m_bgOpacity = (std::min)(0.95f, m_bgOpacity + 0.05f);
            m_hud.ShowToast(Localization::GetCurrentLanguage() == Language::Turkish ?
                (L"Saydamlık: %" + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f))) :
                (L"Transparency: " + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f)) + L"%"));
            SaveSettings();
            Render();
            return 0;
        }
        break;
    }

    case WM_DROPFILES: {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        wchar_t filePath[MAX_PATH] = { 0 };
        if (DragQueryFileW(hDrop, 0, filePath, MAX_PATH) > 0) {
            m_folderNav.LoadFromInitialFile(filePath);
            m_thumbBar.SetFileList(m_folderNav.GetAllFiles(), m_folderNav.GetCurrentIndex(), static_cast<float>(m_screenWidth));
            LoadImage(filePath);
        }
        DragFinish(hDrop);
        return 0;
    }

    case WM_DESTROY: {
        SaveSettings();
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
#endif // _WIN32

void ViewerApp::OnUpdate(float dt) {
    m_hud.Update(dt);
    m_thumbBar.Update(dt, m_d2dContext);

    bool needsRepaint = m_hud.NeedsRedraw() || m_thumbBar.NeedsRedraw() || m_needsRepaint;

    // Animate GIF playback
    if (m_isGif && m_gifFrames.size() > 1) {
        m_gifTimer += dt;
        if (m_gifTimer >= m_gifFrames[m_currentGifFrame].delaySeconds) {
            m_gifTimer = 0.0f;
            m_currentGifFrame = (m_currentGifFrame + 1) % m_gifFrames.size();
            m_currentBitmap = m_gifFrames[m_currentGifFrame].bitmap;
            needsRepaint = true;
        }
    }

    if (needsRepaint) {
        m_hud.ClearNeedsRedraw();
        m_thumbBar.ClearNeedsRedraw();
        Render();
    }
}

void ViewerApp::OnMouseMove(float mouseX, float mouseY) {
#if !defined(_WIN32)
    if (m_platform) {
        if (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY)) {
            m_platform->SetCursor(LinuxCursor::Arrow);
        } else if (m_isCropping && m_imageWidth > 0 && m_imageHeight > 0 && !m_hud.IsMouseOverHud(mouseX, mouseY) && !m_thumbBar.IsMouseOver(mouseX, mouseY)) {
            float lNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
            float rNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
            float tNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
            float bNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);
            D2D1_POINT_2F p0 = ImagePixelToScreen(lNorm * m_imageWidth, tNorm * m_imageHeight);
            D2D1_POINT_2F p1 = ImagePixelToScreen(rNorm * m_imageWidth, bNorm * m_imageHeight);
            float scrL = (std::min)(p0.x, p1.x);
            float scrR = (std::max)(p0.x, p1.x);
            float scrT = (std::min)(p0.y, p1.y);
            float scrB = (std::max)(p0.y, p1.y);

            int handle = HitTestCropHandles(mouseX, mouseY, scrL, scrT, scrR, scrB, m_dpiScale);
            if (handle == 0 || handle == 2) m_platform->SetCursor(LinuxCursor::SizeNWSE);
            else if (handle == 1 || handle == 3) m_platform->SetCursor(LinuxCursor::SizeNESW);
            else if (handle == 4 || handle == 6) m_platform->SetCursor(LinuxCursor::SizeNS);
            else if (handle == 5 || handle == 7) m_platform->SetCursor(LinuxCursor::SizeWE);
            else if (handle == 8) m_platform->SetCursor(LinuxCursor::Move4Way);
            else m_platform->SetCursor(LinuxCursor::Arrow);
        } else if (m_isErasing && IsPointInsideImage(mouseX, mouseY) && !m_hud.IsMouseOverHud(mouseX, mouseY) && !m_thumbBar.IsMouseOver(mouseX, mouseY)) {
            m_platform->SetCursor(LinuxCursor::Crosshair);
        } else if (m_isDragging || (IsPointInsideImage(mouseX, mouseY) && !m_hud.IsMouseOverHud(mouseX, mouseY) && !m_thumbBar.IsMouseOver(mouseX, mouseY))) {
            m_platform->SetCursor(LinuxCursor::Move4Way);
        } else {
            m_platform->SetCursor(LinuxCursor::Arrow);
        }
    }
#endif

    if (m_activeCropHandle >= 0 && m_imageWidth > 0 && m_imageHeight > 0) {
        float dx = mouseX - m_cropDragStart.x;
        float dy = mouseY - m_cropDragStart.y;
        float imgScreenW = m_imageWidth * m_zoom;
        float imgScreenH = m_imageHeight * m_zoom;
        float dNormX = (imgScreenW > 1.0f) ? (dx / imgScreenW) : 0.0f;
        float dNormY = (imgScreenH > 1.0f) ? (dy / imgScreenH) : 0.0f;

        D2D1_RECT_F r = m_cropRectAtDragStart;
        bool isSymmetric = m_isCropSymmetric;

        if (m_activeCropHandle == 8) {
            float w = r.right - r.left;
            float h = r.bottom - r.top;
            float newL = std::clamp(r.left + dNormX, 0.0f, 1.0f - w);
            float newT = std::clamp(r.top + dNormY, 0.0f, 1.0f - h);
            r.left = newL;
            r.top = newT;
            r.right = newL + w;
            r.bottom = newT + h;
        } else if (m_activeCropRatio == CropRatio::Free) {
            if (isSymmetric) {
                float cx = (r.left + r.right) / 2.0f;
                float cy = (r.top + r.bottom) / 2.0f;
                float curHalfW = (r.right - r.left) / 2.0f;
                float curHalfH = (r.bottom - r.top) / 2.0f;
                float maxHalfW = (std::min)(cx, 1.0f - cx);
                float maxHalfH = (std::min)(cy, 1.0f - cy);

                float deltaX = 0.0f;
                float deltaY = 0.0f;
                switch (m_activeCropHandle) {
                case 0: deltaX = -dNormX; deltaY = -dNormY; break; // TL
                case 1: deltaX =  dNormX; deltaY = -dNormY; break; // TR
                case 2: deltaX =  dNormX; deltaY =  dNormY; break; // BR
                case 3: deltaX = -dNormX; deltaY =  dNormY; break; // BL
                case 4: deltaX = 0.0f;    deltaY = -dNormY; break; // Top
                case 5: deltaX =  dNormX; deltaY = 0.0f;    break; // Right
                case 6: deltaX = 0.0f;    deltaY =  dNormY; break; // Bottom
                case 7: deltaX = -dNormX; deltaY = 0.0f;    break; // Left
                }

                float newHalfW = (deltaX != 0.0f) ? std::clamp(curHalfW + deltaX, 0.02f, maxHalfW) : curHalfW;
                float newHalfH = (deltaY != 0.0f) ? std::clamp(curHalfH + deltaY, 0.02f, maxHalfH) : curHalfH;

                r.left = cx - newHalfW;
                r.right = cx + newHalfW;
                r.top = cy - newHalfH;
                r.bottom = cy + newHalfH;
            } else {
                switch (m_activeCropHandle) {
                case 0: // TL
                    r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                    r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                    break;
                case 1: // TR
                    r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                    r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                    break;
                case 2: // BR
                    r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                    r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                    break;
                case 3: // BL
                    r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                    r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                    break;
                case 4: // Top
                    r.top = std::clamp(r.top + dNormY, 0.0f, r.bottom - 0.02f);
                    break;
                case 5: // Right
                    r.right = std::clamp(r.right + dNormX, r.left + 0.02f, 1.0f);
                    break;
                case 6: // Bottom
                    r.bottom = std::clamp(r.bottom + dNormY, r.top + 0.02f, 1.0f);
                    break;
                case 7: // Left
                    r.left = std::clamp(r.left + dNormX, 0.0f, r.right - 0.02f);
                    break;
                }
            }
        } else {
            // Locked aspect ratio mode
            float targetAspect = 1.0f;
            switch (m_activeCropRatio) {
            case CropRatio::Original:
                targetAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
                break;
            case CropRatio::Ratio1x1:  targetAspect = 1.0f; break;
            case CropRatio::Ratio16x9: targetAspect = 16.0f / 9.0f; break;
            case CropRatio::Ratio9x16: targetAspect = 9.0f / 16.0f; break;
            case CropRatio::Ratio4x3:  targetAspect = 4.0f / 3.0f; break;
            case CropRatio::Ratio3x2:  targetAspect = 3.0f / 2.0f; break;
            default: break;
            }
            float imgAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
            float k = targetAspect / (imgAspect > 0.0001f ? imgAspect : 1.0f);

            if (isSymmetric) {
                float cx = (r.left + r.right) / 2.0f;
                float cy = (r.top + r.bottom) / 2.0f;
                float maxHalfW = (std::min)(cx, 1.0f - cx);
                float maxHalfH = (std::min)(cy, 1.0f - cy);
                float maxH = (std::min)(maxHalfH, maxHalfW / k);
                float maxW = maxH * k;

                float delta = 0.0f;
                switch (m_activeCropHandle) {
                case 0: delta = (-dNormX - dNormY * k) / 2.0f; break; // TL
                case 1: delta = ( dNormX - dNormY * k) / 2.0f; break; // TR
                case 2: delta = ( dNormX + dNormY * k) / 2.0f; break; // BR
                case 3: delta = (-dNormX + dNormY * k) / 2.0f; break; // BL
                case 4: delta = -dNormY * k; break; // Top
                case 5: delta =  dNormX;     break; // Right
                case 6: delta =  dNormY * k; break; // Bottom
                case 7: delta = -dNormX;     break; // Left
                }

                float curHalfW = (r.right - r.left) / 2.0f;
                float newHalfW = std::clamp(curHalfW + delta, 0.02f, maxW);
                float newHalfH = newHalfW / k;

                r.left = cx - newHalfW;
                r.right = cx + newHalfW;
                r.top = cy - newHalfH;
                r.bottom = cy + newHalfH;
            } else {
                float curW = r.right - r.left;
                switch (m_activeCropHandle) {
                case 2: // BR
                case 5: // Right
                case 6: { // Bottom
                    float maxW = (std::min)(1.0f - r.left, (1.0f - r.top) * k);
                    float delta = (m_activeCropHandle == 5) ? dNormX : (m_activeCropHandle == 6 ? dNormY * k : (dNormX + dNormY * k) / 2.0f);
                    float newW = std::clamp(curW + delta, 0.02f, maxW);
                    float newH = newW / k;
                    r.right = r.left + newW;
                    r.bottom = r.top + newH;
                    break;
                }
                case 0: // TL
                case 4: // Top
                case 7: { // Left
                    float maxW = (std::min)(r.right, r.bottom * k);
                    float delta = (m_activeCropHandle == 7) ? -dNormX : (m_activeCropHandle == 4 ? -dNormY * k : (-dNormX - dNormY * k) / 2.0f);
                    float newW = std::clamp(curW + delta, 0.02f, maxW);
                    float newH = newW / k;
                    r.left = r.right - newW;
                    r.top = r.bottom - newH;
                    break;
                }
                case 1: { // TR
                    float maxW = (std::min)(1.0f - r.left, r.bottom * k);
                    float newW = std::clamp(curW + (dNormX - dNormY * k) / 2.0f, 0.02f, maxW);
                    float newH = newW / k;
                    r.right = r.left + newW;
                    r.top = r.bottom - newH;
                    break;
                }
                case 3: { // BL
                    float maxW = (std::min)(r.right, (1.0f - r.top) * k);
                    float newW = std::clamp(curW + (-dNormX + dNormY * k) / 2.0f, 0.02f, maxW);
                    float newH = newW / k;
                    r.left = r.right - newW;
                    r.bottom = r.top + newH;
                    break;
                }
                }
            }
        }

        m_cropNormRect = r;
        Render();
        return;
    }

    bool isHoverImg = IsPointInsideImage(mouseX, mouseY) && !m_hud.IsMouseOverHud(mouseX, mouseY) && !m_thumbBar.IsMouseOver(mouseX, mouseY) && (!m_isCropping || !m_cropToolbar.IsMouseOver(mouseX, mouseY));
    if (m_isHoveringImage != isHoverImg) {
        m_isHoveringImage = isHoverImg;
        Render();
    }

    if (m_isDragging) {
        m_panX = m_dragStartPanX + (mouseX - m_dragStartMouse.x);
        m_panY = m_dragStartPanY + (mouseY - m_dragStartMouse.y);
        m_hud.ResetIdleTimer();
        Render();
    } else {
        m_hud.OnMouseMove(mouseX, mouseY);
        m_thumbBar.OnMouseMove(mouseX, mouseY);
        if (m_isCropping) {
            m_cropToolbar.OnMouseMove(mouseX, mouseY);
        }
        if (m_hud.NeedsRedraw() || m_thumbBar.IsMouseOver(mouseX, mouseY) || (m_isCropping && m_cropToolbar.NeedsRedraw())) {
            m_hud.ClearNeedsRedraw();
            if (m_isCropping) m_cropToolbar.ClearNeedsRedraw();
            Render();
        }
    }
}

void ViewerApp::OnMouseDown(int button, float mouseX, float mouseY, bool shift, bool alt, bool ctrl) {
    (void)alt;
    (void)ctrl;
    if (button != 0) return; // 0 = left button

    if (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY)) {
        if (m_cropToolbar.OnMouseDown(mouseX, mouseY)) {
            Render();
            return;
        }
    }

    if (m_hud.IsMouseOverHud(mouseX, mouseY)) {
        m_hud.OnMouseDown(mouseX, mouseY);
        Render();
        return;
    }

    if (m_thumbBar.IsMouseOver(mouseX, mouseY)) {
        int clickedIdx = m_thumbBar.OnMouseDown(mouseX, mouseY);
        if (clickedIdx >= 0) {
            m_folderNav.SetIndex(static_cast<size_t>(clickedIdx));
            LoadImage(m_folderNav.GetCurrentPath());
        }
        return;
    }

    if (m_isCropping && m_imageWidth > 0 && m_imageHeight > 0) {
        float lNorm = (std::min)(m_cropNormRect.left, m_cropNormRect.right);
        float rNorm = (std::max)(m_cropNormRect.left, m_cropNormRect.right);
        float tNorm = (std::min)(m_cropNormRect.top, m_cropNormRect.bottom);
        float bNorm = (std::max)(m_cropNormRect.top, m_cropNormRect.bottom);
        D2D1_POINT_2F p0 = ImagePixelToScreen(lNorm * m_imageWidth, tNorm * m_imageHeight);
        D2D1_POINT_2F p1 = ImagePixelToScreen(rNorm * m_imageWidth, bNorm * m_imageHeight);
        float scrL = (std::min)(p0.x, p1.x);
        float scrR = (std::max)(p0.x, p1.x);
        float scrT = (std::min)(p0.y, p1.y);
        float scrB = (std::max)(p0.y, p1.y);

        int handle = HitTestCropHandles(mouseX, mouseY, scrL, scrT, scrR, scrB, m_dpiScale);
        if (handle >= 0) {
            m_activeCropHandle = handle;
            m_cropDragStart = D2D1::Point2F(mouseX, mouseY);
            m_cropRectAtDragStart = m_cropNormRect;
            return;
        }
    }

    bool insideImage = IsPointInsideImage(mouseX, mouseY);

    if (m_isErasing && insideImage) {
        MagicEraseAt(mouseX, mouseY, shift);
        return;
    }

    m_isDragging = true;
    m_dragStartMouse.x = static_cast<LONG>(mouseX);
    m_dragStartMouse.y = static_cast<LONG>(mouseY);
    m_dragStartPanX = m_panX;
    m_dragStartPanY = m_panY;
}

void ViewerApp::OnMouseUp(int button, float mouseX, float mouseY) {
    if (button != 0) return;

    if (m_activeCropHandle >= 0) {
        m_activeCropHandle = -1;
        return;
    }

    if (m_isDragging) {
        m_isDragging = false;
    }

    if (m_isCropping && m_cropToolbar.IsMouseOver(mouseX, mouseY)) {
        CropRatio selectedRatio = CropRatio::Free;
        CropAction cAction = m_cropToolbar.OnMouseUp(mouseX, mouseY, &selectedRatio);
        switch (cAction) {
        case CropAction::SetRatio:
            SetCropAspectRatio(selectedRatio);
            break;
        case CropAction::ToggleSymmetric:
            m_isCropSymmetric = !m_isCropSymmetric;
            m_cropToolbar.SetSymmetric(m_isCropSymmetric);
            m_hud.ShowToast(Localization::Get(m_isCropSymmetric ? StringId::ToastCropSymmetricOn : StringId::ToastCropSymmetricOff));
            Render();
            break;
        case CropAction::Reset:
            ResetCropBox();
            break;
        case CropAction::Apply:
            ApplyCrop();
            break;
        case CropAction::Cancel:
            CancelCrop();
            break;
        default:
            break;
        }
        return;
    }

    HudAction action = m_hud.OnMouseUp(mouseX, mouseY);
    switch (action) {
    case HudAction::Prev: PrevImage(); break;
    case HudAction::Next: NextImage(); break;
    case HudAction::ZoomIn: ZoomAt(1.25f, m_screenWidth / 2.0f, m_screenHeight / 2.0f); break;
    case HudAction::ZoomOut: ZoomAt(1.0f / 1.25f, m_screenWidth / 2.0f, m_screenHeight / 2.0f); break;
    case HudAction::CycleAspectMode: CycleAspectMode(); break;
    case HudAction::ActualSize: SetActualSize(); Render(); break;
    case HudAction::RotateLeft: Rotate(-90.0f); break;
    case HudAction::RotateRight: Rotate(90.0f); break;
    case HudAction::Crop: ToggleCropMode(); break;
    case HudAction::MagicErase: ToggleEraseMode(); break;
    case HudAction::AutoBgRemove: AutoRemoveBackground(); break;
    case HudAction::Undo: Undo(); break;
    case HudAction::SaveAs: SaveAs(); break;
    case HudAction::ToggleLanguage: ToggleLanguage(); break;
    case HudAction::ToggleFullscreen: ToggleFullscreen(); break;
    case HudAction::Close:
#if defined(_WIN32)
        PostQuitMessage(0);
#else
        if (m_platform) m_platform->Shutdown();
#endif
        break;
    default: break;
    }

    if (m_hud.NeedsRedraw()) {
        m_hud.ClearNeedsRedraw();
        Render();
    }
}

void ViewerApp::OnMouseWheel(short delta, float mouseX, float mouseY, bool shift, bool alt, bool ctrl) {
    (void)shift;
    (void)alt;
    if (m_thumbBar.IsMouseOver(mouseX, mouseY)) {
        m_thumbBar.OnMouseWheel(delta, static_cast<float>(m_screenWidth));
        m_hud.ResetIdleTimer();
        Render();
        return;
    }

    if (ctrl) {
        float step = (delta > 0) ? -0.05f : 0.05f;
        m_bgOpacity = std::clamp(m_bgOpacity + step, 0.05f, 0.95f);
        m_hud.ShowToast(Localization::GetCurrentLanguage() == Language::Turkish ?
            (L"Saydamlık: %" + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f))) :
            (L"Transparency: " + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f)) + L"%"));
        SaveSettings();
        Render();
        return;
    }

    float factor = (delta > 0) ? 1.15f : (1.0f / 1.15f);
    ZoomAt(factor, mouseX, mouseY);
}

void ViewerApp::OnKeyDown(int keyCode, wchar_t keyChar, bool shift, bool alt, bool ctrl) {
    (void)alt;
    (void)shift;
    m_hud.ResetIdleTimer();

    if (ctrl) {
        if (keyCode == 'S') {
            SaveAs();
            return;
        } else if (keyCode == 'Z') {
            Undo();
            return;
        }
    }

    switch (keyCode) {
    case 'C':
        ToggleCropMode();
        return;
    case 'S':
        if (m_isCropping) {
            m_isCropSymmetric = !m_isCropSymmetric;
            m_cropToolbar.SetSymmetric(m_isCropSymmetric);
            m_hud.ShowToast(Localization::Get(m_isCropSymmetric ? StringId::ToastCropSymmetricOn : StringId::ToastCropSymmetricOff));
            Render();
            return;
        }
        break;
    case 'E':
        ToggleEraseMode();
        return;
    case 'B':
        AutoRemoveBackground();
        return;
    case 'T':
        ToggleLanguage();
        return;
    case 27: // VK_ESCAPE
        if (m_isCropping) {
            CancelCrop();
        } else if (m_isErasing) {
            ToggleEraseMode();
        } else {
#if defined(_WIN32)
            PostQuitMessage(0);
#else
            if (m_platform) m_platform->Shutdown();
#endif
        }
        return;
    case 122: // VK_F11
        ToggleFullscreen();
        return;
    case 13: // VK_RETURN
        if (m_isCropping) {
            ApplyCrop();
        } else {
            ToggleFullscreen();
        }
        return;
    case 'M':
        CycleAspectMode();
        return;
    case 37: // VK_LEFT
    case 'A':
        PrevImage();
        return;
    case 39: // VK_RIGHT
    case 'D':
        NextImage();
        return;
    case 38: // VK_UP
    case '+':
    case '=':
        ZoomAt(1.20f, m_screenWidth / 2.0f, m_screenHeight / 2.0f);
        return;
    case 40: // VK_DOWN
    case '-':
    case '_':
        ZoomAt(1.0f / 1.20f, m_screenWidth / 2.0f, m_screenHeight / 2.0f);
        return;
    case 'R':
        Rotate(90.0f);
        return;
    case 'L':
        Rotate(-90.0f);
        return;
    case 'F':
    case '0':
        ResetViewToFit();
        Render();
        return;
    case '1':
        SetActualSize();
        Render();
        return;
    case 'O':
        OpenFileDialog();
        return;
    case 36: // VK_HOME
        FirstImage();
        return;
    case 35: // VK_END
        LastImage();
        return;
    }

    if (keyChar == L'[' || keyCode == 219) {
        m_bgOpacity = (std::max)(0.05f, m_bgOpacity - 0.05f);
        m_hud.ShowToast(Localization::GetCurrentLanguage() == Language::Turkish ?
            (L"Saydamlık: %" + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f))) :
            (L"Transparency: " + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f)) + L"%"));
        SaveSettings();
        Render();
    } else if (keyChar == L']' || keyCode == 221) {
        m_bgOpacity = (std::min)(0.95f, m_bgOpacity + 0.05f);
        m_hud.ShowToast(Localization::GetCurrentLanguage() == Language::Turkish ?
            (L"Saydamlık: %" + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f))) :
            (L"Transparency: " + std::to_wstring(static_cast<int>((1.0f - m_bgOpacity) * 100.0f + 0.5f)) + L"%"));
        SaveSettings();
        Render();
    }
}
