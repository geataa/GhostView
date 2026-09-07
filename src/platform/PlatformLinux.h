#pragma once
#include "PlatformDefs.h"

#if !defined(_WIN32)
#include <string>
#include <functional>

// Opaque X11 types
typedef void Display;
typedef unsigned long Window;
typedef struct __GLXcontextRec *GLXContext;

enum class LinuxCursor {
    Arrow,
    Move4Way,
    Crosshair,
    SizeNWSE,
    SizeNESW,
    SizeNS,
    SizeWE
};

class PlatformLinux {
public:
    PlatformLinux();
    ~PlatformLinux();

    bool Initialize(int width, int height, bool fullscreen = true);
    void Shutdown();

    void SetWindowTitle(const std::string& title);
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_isFullscreen; }

    void GetWindowSize(int& outWidth, int& outHeight) const;
    void SwapBuffers();
    void Invalidate();

    void SetCursor(LinuxCursor cursor);

    // Dialogs
    std::wstring OpenFileDialog(const std::wstring& title);
    std::wstring SaveFileDialog(const std::wstring& title, const std::wstring& defaultExt = L"png");

    // Event polling: returns false when application should exit
    bool PollEvents();

    // Callbacks to ViewerApp
    std::function<void(int width, int height)> onResize;
    std::function<void()> onPaint;
    std::function<void(float dt)> onUpdate;
    std::function<void(float x, float y)> onMouseMove;
    std::function<void(int button, float x, float y, bool shift, bool alt, bool ctrl)> onMouseDown;
    std::function<void(int button, float x, float y)> onMouseUp;
    std::function<void(short delta, float x, float y)> onMouseWheel;
    std::function<void(int keyCode, wchar_t keyChar, bool shift, bool alt, bool ctrl)> onKeyDown;
    std::function<void(const std::wstring& filePath)> onFileDrop;

private:
    Display* m_display = nullptr;
    Window m_window = 0;
    GLXContext m_glContext = nullptr;
    int m_screen = 0;
    int m_width = 1920;
    int m_height = 1080;
    int m_savedX = 100;
    int m_savedY = 100;
    int m_savedWidth = 1280;
    int m_savedHeight = 720;
    bool m_isFullscreen = false;
    bool m_shouldClose = false;
    uint32_t m_lastTick = 0;
};

#endif
