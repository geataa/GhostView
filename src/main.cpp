#if defined(_WIN32)
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
#else
#include "ViewerApp.h"
#include <string>
#include <cstdio>

static std::string NormalizePath(const std::string& input) {
    std::string path = input;
    if (path.rfind("file://", 0) == 0) {
        path = path.substr(7);
    }
    std::string out;
    out.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '%' && i + 2 < path.size()) {
            int ch = 0;
            if (sscanf(path.substr(i + 1, 2).c_str(), "%x", &ch) == 1) {
                out += static_cast<char>(ch);
                i += 2;
                continue;
            }
        }
        out += path[i];
    }
    return out;
}

int main(int argc, char** argv) {
    std::wstring initialFile = L"";
    if (argc > 1) {
        initialFile = Utf8ToWide(NormalizePath(argv[1]));
    }

    ViewerApp app;
    if (!app.Initialize(initialFile)) {
        return 1;
    }

    return app.Run();
}
#endif
