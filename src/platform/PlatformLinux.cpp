#include "PlatformLinux.h"

#if !defined(_WIN32)
#include <iostream>
#include <chrono>
#include <cstring>
#include <array>
#include <memory>
#include <unistd.h>
#include <sys/time.h>

// X11 Types & Structs
typedef unsigned long XID;
typedef XID Colormap;
typedef XID Cursor;
typedef XID Atom;
typedef XID VisualID;
typedef unsigned long KeySym;

struct Visual {
    void* ext_data;
    VisualID visualid;
    int c_class;
    unsigned long red_mask, green_mask, blue_mask;
    int bits_per_rgb;
    int map_entries;
};

struct XVisualInfo {
    Visual* visual;
    VisualID visualid;
    int screen;
    int depth;
    int c_class;
    unsigned long red_mask, green_mask, blue_mask;
    int colormap_size;
    int bits_per_rgb;
};

struct XSetWindowAttributes {
    void* background_pixmap;
    unsigned long background_pixel;
    void* border_pixmap;
    unsigned long border_pixel;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    int save_under;
    long event_mask;
    long do_not_propagate_mask;
    int override_redirect;
    Colormap colormap;
    Cursor cursor;
};

struct XAnyEvent { int type; unsigned long serial; int send_event; Display *display; Window window; };
struct XKeyEvent { int type; unsigned long serial; int send_event; Display *display; Window window; Window root; Window subwindow; unsigned long time; int x, y; int x_root, y_root; unsigned int state; unsigned int keycode; int same_screen; };
struct XButtonEvent { int type; unsigned long serial; int send_event; Display *display; Window window; Window root; Window subwindow; unsigned long time; int x, y; int x_root, y_root; unsigned int state; unsigned int button; int same_screen; };
struct XMotionEvent { int type; unsigned long serial; int send_event; Display *display; Window window; Window root; Window subwindow; unsigned long time; int x, y; int x_root, y_root; unsigned int state; char is_hint; int same_screen; };
struct XExposeEvent { int type; unsigned long serial; int send_event; Display *display; Window window; int x, y; int width, height; int count; };
struct XConfigureEvent { int type; unsigned long serial; int send_event; Display *display; Window event; Window window; int x, y; int width, height; int border_width; Window above; int override_redirect; };
struct XClientMessageEvent {
    int type; unsigned long serial; int send_event; Display *display; Window window;
    Atom message_type; int format;
    union { char b[20]; short s[10]; long l[5]; } data;
};

union XEvent {
    int type;
    XAnyEvent xany;
    XKeyEvent xkey;
    XButtonEvent xbutton;
    XMotionEvent xmotion;
    XExposeEvent xexpose;
    XConfigureEvent xconfigure;
    XClientMessageEvent xclient;
    long pad[24];
};

typedef void* GLXFBConfig;

extern "C" {
    Display* XOpenDisplay(const char*);
    int XCloseDisplay(Display*);
    int XDefaultScreen(Display*);
    Window XRootWindow(Display*, int);
    int XDisplayWidth(Display*, int);
    int XDisplayHeight(Display*, int);
    XVisualInfo* glXChooseVisual(Display*, int, int*);
    Colormap XCreateColormap(Display*, Window, Visual*, int);
    Window XCreateWindow(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    int XMapWindow(Display*, Window);
    int XUnmapWindow(Display*, Window);
    int XDestroyWindow(Display*, Window);
    int XStoreName(Display*, Window, const char*);
    int XSelectInput(Display*, Window, long);
    int XPending(Display*);
    int XNextEvent(Display*, XEvent*);
    Atom XInternAtom(Display*, const char*, int);
    int XSetWMProtocols(Display*, Window, Atom*, int);
    int XChangeProperty(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
    int XSendEvent(Display*, Window, int, long, XEvent*);
    int XMoveResizeWindow(Display*, Window, int, int, unsigned int, unsigned int);
    int XFlush(Display*);
    int XSync(Display*, int);
    KeySym XLookupKeysym(XKeyEvent*, int);
    Cursor XCreateFontCursor(Display*, unsigned int);
    int XDefineCursor(Display*, Window, Cursor);
    int XFreeCursor(Display*, Cursor);

    GLXContext glXCreateContext(Display*, XVisualInfo*, GLXContext, int);
    GLXFBConfig* glXChooseFBConfig(Display*, int, const int*, int*);
    XVisualInfo* glXGetVisualFromFBConfig(Display*, GLXFBConfig);
    GLXContext glXCreateNewContext(Display*, GLXFBConfig, int, GLXContext, int);
    int glXMakeCurrent(Display*, Window, GLXContext);
    void glXSwapBuffers(Display*, Window);
    void glXDestroyContext(Display*, GLXContext);
}

#define GLX_RGBA 4
#define GLX_DOUBLEBUFFER 5
#define GLX_RED_SIZE 8
#define GLX_GREEN_SIZE 8
#define GLX_BLUE_SIZE 8
#define GLX_ALPHA_SIZE 11
#define GLX_DEPTH_SIZE 12
#define GLX_RENDER_TYPE 0x8011
#define GLX_RGBA_BIT 0x00000001
#define GLX_RGBA_TYPE 0x8014

#define AllocNone 0
#define CWBackPixel (1L<<1)
#define CWBorderPixel (1L<<3)
#define CWColormap (1L<<13)
#define CWEventMask (1L<<11)

#define KeyPressMask (1L<<0)
#define KeyReleaseMask (1L<<1)
#define ButtonPressMask (1L<<2)
#define ButtonReleaseMask (1L<<3)
#define PointerMotionMask (1L<<6)
#define ExposureMask (1L<<15)
#define StructureNotifyMask (1L<<17)

#define KeyPress 2
#define KeyRelease 3
#define ButtonPress 4
#define ButtonRelease 5
#define MotionNotify 6
#define Expose 12
#define ConfigureNotify 22
#define ClientMessage 33

#define ShiftMask (1<<0)
#define ControlMask (1<<2)
#define Mod1Mask (1<<3)

#define XC_left_ptr 68
#define XC_crosshair 34
#define XC_fleur 52
#define XC_top_left_corner 134
#define XC_top_right_corner 136
#define XC_bottom_right_corner 14
#define XC_bottom_left_corner 12
#define XC_top_side 138
#define XC_right_side 96
#define XC_bottom_side 16
#define XC_left_side 70
#define XC_sizing 120

#define XK_Escape 0xff1b
#define XK_Return 0xff0d
#define XK_Left 0xff51
#define XK_Up 0xff52
#define XK_Right 0xff53
#define XK_Down 0xff54
#define XK_F11 0xffc8
#define XK_space 0x0020
#define XK_bracketleft 0x005b
#define XK_bracketright 0x005d

struct MotifWmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};

PlatformLinux::PlatformLinux() = default;

PlatformLinux::~PlatformLinux() {
    Shutdown();
}

static uint32_t GetCurrentMilliseconds() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<uint32_t>(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

bool PlatformLinux::Initialize(int width, int height, bool fullscreen) {
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        std::cerr << "Error: Cannot open X11 display!\n";
        return false;
    }

    m_screen = XDefaultScreen(m_display);
    Window root = XRootWindow(m_display, m_screen);

    int dispW = XDisplayWidth(m_display, m_screen);
    int dispH = XDisplayHeight(m_display, m_screen);

    if (fullscreen) {
        m_width = dispW;
        m_height = dispH;
        m_isFullscreen = true;
    } else {
        m_width = width;
        m_height = height;
        m_isFullscreen = false;
    }

    int fbAttribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER, 1,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        0
    };

    int nConfigs = 0;
    GLXFBConfig* cfgs = glXChooseFBConfig(m_display, m_screen, fbAttribs, &nConfigs);
    GLXFBConfig bestCfg = nullptr;
    XVisualInfo* vi = nullptr;

    if (cfgs && nConfigs > 0) {
        for (int i = 0; i < nConfigs; ++i) {
            XVisualInfo* v = glXGetVisualFromFBConfig(m_display, cfgs[i]);
            if (v && v->depth == 32) {
                bestCfg = cfgs[i];
                vi = v;
                break;
            }
        }
    }

    if (!vi) {
        int glxAttribs[] = {
            GLX_RGBA,
            GLX_DOUBLEBUFFER,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            0
        };
        vi = glXChooseVisual(m_display, m_screen, glxAttribs);
    }

    if (!vi) {
        std::cerr << "Error: No suitable X11 visual found!\n";
        return false;
    }

    Colormap cmap = XCreateColormap(m_display, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    std::memset(&swa, 0, sizeof(swa));
    swa.colormap = cmap;
    swa.background_pixel = 0;
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    int posX = (dispW - m_width) / 2;
    int posY = (dispH - m_height) / 2;
    if (fullscreen) {
        posX = 0;
        posY = 0;
    }

    m_window = XCreateWindow(
        m_display, root,
        posX, posY, m_width, m_height, 0,
        vi->depth, 1 /* InputOutput */,
        vi->visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
        &swa
    );

    if (!m_window) {
        std::cerr << "Error: XCreateWindow failed!\n";
        return false;
    }

    // Set frameless / borderless hints
    MotifWmHints hints;
    hints.flags = 2; // MWM_HINTS_DECORATIONS
    hints.functions = 0;
    hints.decorations = 0;
    hints.input_mode = 0;
    hints.status = 0;
    Atom mwmAtom = XInternAtom(m_display, "_MOTIF_WM_HINTS", 0);
    XChangeProperty(m_display, m_window, mwmAtom, mwmAtom, 32, 0, (unsigned char*)&hints, 5);

    // Keep compositing active (no bypass) so transparency works in fullscreen
    Atom bypassAtom = XInternAtom(m_display, "_NET_WM_BYPASS_COMPOSITOR", 0);
    unsigned long bypassValue = 2; // 2 = Do NOT bypass compositor (keeps transparency active!)
    XChangeProperty(m_display, m_window, bypassAtom, 6 /* XA_CARDINAL */, 32, 0, (unsigned char*)&bypassValue, 1);

    // Set WM_DELETE_WINDOW protocol
    Atom wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(m_display, m_window, &wmDelete, 1);

    // Set window type to NORMAL
    Atom typeAtom = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", 0);
    Atom typeNormal = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_NORMAL", 0);
    XChangeProperty(m_display, m_window, typeAtom, 4 /* XA_ATOM */, 32, 0, (unsigned char*)&typeNormal, 1);

    // Fullscreen and Topmost properties if requested
    Atom wmState = XInternAtom(m_display, "_NET_WM_STATE", 0);
    if (fullscreen) {
        Atom atoms[2];
        atoms[0] = XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", 0);
        atoms[1] = XInternAtom(m_display, "_NET_WM_STATE_ABOVE", 0);
        XChangeProperty(m_display, m_window, wmState, 4 /* XA_ATOM */, 32, 0, (unsigned char*)atoms, 2);
    } else {
        Atom wmAbove = XInternAtom(m_display, "_NET_WM_STATE_ABOVE", 0);
        XChangeProperty(m_display, m_window, wmState, 4 /* XA_ATOM */, 32, 0, (unsigned char*)&wmAbove, 1);
    }

    XStoreName(m_display, m_window, "GhostView");
    XMapWindow(m_display, m_window);

    if (bestCfg) {
        m_glContext = glXCreateNewContext(m_display, bestCfg, GLX_RGBA_TYPE, nullptr, 1);
    } else {
        m_glContext = glXCreateContext(m_display, vi, nullptr, 1);
    }
    if (!m_glContext) {
        std::cerr << "Error: glXCreateContext failed!\n";
        return false;
    }

    glXMakeCurrent(m_display, m_window, m_glContext);
    XSync(m_display, 0);

    m_lastTick = GetCurrentMilliseconds();
    return true;
}

void PlatformLinux::Shutdown() {
    if (m_display) {
        if (m_glContext) {
            glXMakeCurrent(m_display, 0, nullptr);
            glXDestroyContext(m_display, m_glContext);
            m_glContext = nullptr;
        }
        if (m_window) {
            XDestroyWindow(m_display, m_window);
            m_window = 0;
        }
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

void PlatformLinux::SetWindowTitle(const std::string& title) {
    if (m_display && m_window) {
        XStoreName(m_display, m_window, title.c_str());
    }
}

void PlatformLinux::SetFullscreen(bool fullscreen) {
    if (!m_display || !m_window) return;
    m_isFullscreen = fullscreen;

    Atom wmState = XInternAtom(m_display, "_NET_WM_STATE", 0);
    Atom wmFullscreen = XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", 0);

    XClientMessageEvent xclient;
    std::memset(&xclient, 0, sizeof(xclient));
    xclient.type = ClientMessage;
    xclient.window = m_window;
    xclient.message_type = wmState;
    xclient.format = 32;
    xclient.data.l[0] = fullscreen ? 1 : 0; // 1 = _NET_WM_STATE_ADD, 0 = _NET_WM_STATE_REMOVE
    xclient.data.l[1] = wmFullscreen;
    xclient.data.l[2] = 0;
    xclient.data.l[3] = 1;

    Window root = XRootWindow(m_display, m_screen);
    XSendEvent(m_display, root, 0, StructureNotifyMask, (XEvent*)&xclient);

    // Keep compositor active in fullscreen so background remains transparent
    Atom bypassAtom = XInternAtom(m_display, "_NET_WM_BYPASS_COMPOSITOR", 0);
    unsigned long bypassVal = 2; // 2 = Don't bypass compositor
    XChangeProperty(m_display, m_window, bypassAtom, 6 /* XA_CARDINAL */, 32, 0, (unsigned char*)&bypassVal, 1);

    if (!fullscreen) {
        int dispW = XDisplayWidth(m_display, m_screen);
        int dispH = XDisplayHeight(m_display, m_screen);
        int winW = (std::min)(1280, static_cast<int>(dispW * 0.8f));
        int winH = (std::min)(720, static_cast<int>(dispH * 0.8f));
        int winX = (dispW - winW) / 2;
        int winY = (dispH - winH) / 2;
        XMoveResizeWindow(m_display, m_window, winX, winY, winW, winH);
    }

    XFlush(m_display);
}

void PlatformLinux::GetWindowSize(int& outWidth, int& outHeight) const {
    outWidth = m_width;
    outHeight = m_height;
}

void PlatformLinux::SwapBuffers() {
    if (m_display && m_window) {
        glXSwapBuffers(m_display, m_window);
    }
}

void PlatformLinux::Invalidate() {
    if (onPaint) {
        onPaint();
    }
}

void PlatformLinux::SetCursor(LinuxCursor cursorType) {
    if (!m_display || !m_window) return;

    unsigned int shape = XC_left_ptr;
    switch (cursorType) {
    case LinuxCursor::Arrow: shape = XC_left_ptr; break;
    case LinuxCursor::Move4Way: shape = XC_fleur; break;
    case LinuxCursor::Crosshair: shape = XC_crosshair; break;
    case LinuxCursor::SizeNWSE: shape = XC_top_left_corner; break;
    case LinuxCursor::SizeNESW: shape = XC_top_right_corner; break;
    case LinuxCursor::SizeNS: shape = XC_sizing; break;
    case LinuxCursor::SizeWE: shape = XC_sizing; break;
    }

    Cursor c = XCreateFontCursor(m_display, shape);
    if (c) {
        XDefineCursor(m_display, m_window, c);
        XFreeCursor(m_display, c);
    }
}

std::wstring PlatformLinux::OpenFileDialog(const std::wstring& title) {
    std::string titleUtf8 = WideToUtf8(title);
    std::string cmd = "zenity --file-selection --title=\"" + titleUtf8 + "\" --file-filter=\"Images | *.jpg *.jpeg *.png *.bmp *.gif *.webp *.ico *.tif *.tiff\" 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return L"";

    char buf[4096] = { 0 };
    if (fgets(buf, sizeof(buf) - 1, fp)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[len - 1] = '\0';
            len--;
        }
        pclose(fp);
        return Utf8ToWide(buf);
    }
    pclose(fp);
    return L"";
}

std::wstring PlatformLinux::SaveFileDialog(const std::wstring& title, const std::wstring& defaultExt) {
    std::string titleUtf8 = WideToUtf8(title);
    std::string extUtf8 = WideToUtf8(defaultExt);
    std::string cmd = "zenity --file-selection --save --confirm-overwrite --title=\"" + titleUtf8 + "\" --filename=\"output." + extUtf8 + "\" 2>/dev/null";
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return L"";

    char buf[4096] = { 0 };
    if (fgets(buf, sizeof(buf) - 1, fp)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[len - 1] = '\0';
            len--;
        }
        pclose(fp);
        return Utf8ToWide(buf);
    }
    pclose(fp);
    return L"";
}

bool PlatformLinux::PollEvents() {
    if (!m_display || m_shouldClose) return false;

    uint32_t now = GetCurrentMilliseconds();
    float dt = (now - m_lastTick) / 1000.0f;
    m_lastTick = now;
    if (dt > 0.1f) dt = 0.1f;

    if (onUpdate) {
        onUpdate(dt);
    }

    bool hasMotion = false;
    float lastMotionX = 0.0f;
    float lastMotionY = 0.0f;
    int eventsProcessed = 0;

    while (XPending(m_display) > 0) {
        XEvent ev;
        XNextEvent(m_display, &ev);
        eventsProcessed++;

        switch (ev.type) {
        case Expose:
            if (ev.xexpose.count == 0 && onPaint) {
                onPaint();
            }
            break;

        case ConfigureNotify:
            if (ev.xconfigure.width != m_width || ev.xconfigure.height != m_height) {
                m_width = ev.xconfigure.width;
                m_height = ev.xconfigure.height;
                if (onResize) {
                    onResize(m_width, m_height);
                }
            }
            break;

        case MotionNotify:
            hasMotion = true;
            lastMotionX = static_cast<float>(ev.xmotion.x);
            lastMotionY = static_cast<float>(ev.xmotion.y);
            break;

        case ButtonPress: {
            if (hasMotion) {
                if (onMouseMove) onMouseMove(lastMotionX, lastMotionY);
                hasMotion = false;
            }

            float mx = static_cast<float>(ev.xbutton.x);
            float my = static_cast<float>(ev.xbutton.y);
            bool shift = (ev.xbutton.state & ShiftMask) != 0;
            bool ctrl = (ev.xbutton.state & ControlMask) != 0;
            bool alt = (ev.xbutton.state & Mod1Mask) != 0;

            if (ev.xbutton.button == 1) { // Left
                if (onMouseDown) onMouseDown(0, mx, my, shift, alt, ctrl);
            } else if (ev.xbutton.button == 3) { // Right
                if (onMouseDown) onMouseDown(1, mx, my, shift, alt, ctrl);
            } else if (ev.xbutton.button == 4) { // Wheel Up
                if (onMouseWheel) onMouseWheel(120, mx, my, shift, alt, ctrl);
            } else if (ev.xbutton.button == 5) { // Wheel Down
                if (onMouseWheel) onMouseWheel(-120, mx, my, shift, alt, ctrl);
            }
            break;
        }

        case ButtonRelease: {
            if (hasMotion) {
                if (onMouseMove) onMouseMove(lastMotionX, lastMotionY);
                hasMotion = false;
            }

            float mx = static_cast<float>(ev.xbutton.x);
            float my = static_cast<float>(ev.xbutton.y);
            if (ev.xbutton.button == 1) {
                if (onMouseUp) onMouseUp(0, mx, my);
            } else if (ev.xbutton.button == 3) {
                if (onMouseUp) onMouseUp(1, mx, my);
            }
            break;
        }

        case KeyPress: {
            if (hasMotion) {
                if (onMouseMove) onMouseMove(lastMotionX, lastMotionY);
                hasMotion = false;
            }

            KeySym sym = XLookupKeysym(&ev.xkey, 0);
            bool shift = (ev.xkey.state & ShiftMask) != 0;
            bool ctrl = (ev.xkey.state & ControlMask) != 0;
            bool alt = (ev.xkey.state & Mod1Mask) != 0;

            wchar_t ch = 0;
            int code = 0;

            if (sym == XK_Escape) code = 27; // ESC
            else if (sym == XK_Return) code = 13; // Enter
            else if (sym == XK_Left) code = 37;
            else if (sym == XK_Up) code = 38;
            else if (sym == XK_Right) code = 39;
            else if (sym == XK_Down) code = 40;
            else if (sym == XK_F11) code = 122;
            else if (sym == XK_bracketleft) ch = L'[';
            else if (sym == XK_bracketright) ch = L']';
            else if (sym >= 0x20 && sym <= 0x7E) {
                ch = static_cast<wchar_t>(sym);
                code = static_cast<int>(sym);
                if (ch >= L'a' && ch <= L'z') {
                    code = static_cast<int>(ch - L'a' + L'A'); // Match Windows VK_A..Z
                }
            }

            if (onKeyDown) {
                onKeyDown(code, ch, shift, alt, ctrl);
            }
            break;
        }

        case ClientMessage: {
            Atom wmDelete = XInternAtom(m_display, "WM_DELETE_WINDOW", 0);
            if ((Atom)ev.xclient.data.l[0] == wmDelete) {
                m_shouldClose = true;
                return false;
            }
            break;
        }
        }
    }

    // Deliver the coalesced motion event (once per event loop tick)
    if (hasMotion) {
        if (onMouseMove) {
            onMouseMove(lastMotionX, lastMotionY);
        }
    }

    // Only sleep when completely idle to yield CPU (~0% CPU when stationary, 60+ FPS when interacting)
    if (eventsProcessed == 0) {
        usleep(2000);
    }

    return true;
}

#endif
