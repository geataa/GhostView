#include <windows.h>
#include <shellapi.h>
#include <string>
#include "ViewerApp.h"

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    LPWSTR /*lpCmdLine*/,
    int nCmdShow
) {
    // Enable Per-Monitor V2 DPI awareness for crystal-clear text & buttons and zero DWM scaling latency
    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextFn)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pfn = (SetProcessDpiAwarenessContextFn)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pfn) {
            pfn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    // Parse command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::wstring initialFile = L"";

    if (argv && argc > 1) {
        initialFile = argv[1];
    }
    if (argv) {
        LocalFree(argv);
    }

    ViewerApp app;
    if (!app.Initialize(hInstance, nCmdShow, initialFile)) {
        return 1;
    }

    return app.Run();
}
