#include "D2DCompat.h"

#if !defined(_WIN32)
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../third_party/stb/stb_truetype.h"

// OpenGL C Declarations
extern "C" {
    void glViewport(int x, int y, int width, int height);
    void glMatrixMode(unsigned int mode);
    void glLoadIdentity();
    void glOrtho(double left, double right, double bottom, double top, double nearVal, double farVal);
    void glClearColor(float red, float green, float blue, float alpha);
    void glClear(unsigned int mask);
    void glEnable(unsigned int cap);
    void glDisable(unsigned int cap);
    void glBlendFunc(unsigned int sfactor, unsigned int dfactor);
    void glColor4f(float red, float green, float blue, float alpha);
    void glBegin(unsigned int mode);
    void glEnd();
    void glVertex2f(float x, float y);
    void glTexCoord2f(float s, float t);
    void glGenTextures(int n, unsigned int *textures);
    void glDeleteTextures(int n, const unsigned int *textures);
    void glBindTexture(unsigned int target, unsigned int texture);
    void glTexParameteri(unsigned int target, unsigned int pname, int param);
    void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void *pixels);
    void glTexSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height, unsigned int format, unsigned int type, const void *pixels);
    void glLineWidth(float width);
    void glLoadMatrixf(const float *m);
    void glPushMatrix();
    void glPopMatrix();
    void glPixelStorei(unsigned int pname, int param);
    void glScissor(int x, int y, int width, int height);
}

#define GL_SCISSOR_TEST     0x0C11
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_MODELVIEW        0x1700
#define GL_PROJECTION       0x1701
#define GL_TEXTURE_2D       0x0DE1
#define GL_BLEND            0x0BE2
#define GL_SRC_ALPHA        0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_LINES            0x0001
#define GL_LINE_LOOP        0x0002
#define GL_TRIANGLES        0x0004
#define GL_TRIANGLE_FAN     0x0006
#define GL_QUADS            0x0007
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S   0x2802
#define GL_TEXTURE_WRAP_T   0x2803
#define GL_CLAMP_TO_EDGE    0x812F
#define GL_LINEAR           0x2601
#define GL_NEAREST          0x2600
#define GL_RGBA             0x1908
#define GL_BGRA             0x80E1
#define GL_UNSIGNED_BYTE    0x1401
#define GL_UNPACK_ALIGNMENT 0x0CF5

// ID2D1Bitmap Implementation
ID2D1Bitmap::~ID2D1Bitmap() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}

HRESULT ID2D1Bitmap::CopyFromMemory(const D2D1_RECT_F* dstRect, const void* srcData, UINT32 pitch) {
    if (textureId == 0 || !srcData) return E_FAIL;
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    int x = 0, y = 0, w = width, h = height;
    if (dstRect) {
        x = (int)dstRect->left;
        y = (int)dstRect->top;
        w = (int)(dstRect->right - dstRect->left);
        h = (int)(dstRect->bottom - dstRect->top);
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_BGRA, GL_UNSIGNED_BYTE, srcData);
    return S_OK;
}

// IDWriteFactory Implementation
HRESULT IDWriteFactory::CreateTextFormat(
    const wchar_t* fontFamilyName,
    void* /*fontCollection*/,
    DWRITE_FONT_WEIGHT fontWeight,
    DWRITE_FONT_STYLE /*fontStyle*/,
    DWRITE_FONT_STRETCH /*fontStretch*/,
    float fontSize,
    const wchar_t* /*localeName*/,
    IDWriteTextFormat** textFormat
) {
    if (!textFormat) return E_FAIL;
    auto fmt = new IDWriteTextFormat();
    if (fontFamilyName) fmt->fontFamily = fontFamilyName;
    fmt->fontSize = fontSize;
    fmt->weight = fontWeight;
    *textFormat = fmt;
    return S_OK;
}

// Glyph cache for STB TrueType
struct CachedGlyph {
    uint32_t texId = 0;
    int width = 0;
    int height = 0;
    int xOffset = 0;
    int yOffset = 0;
    float advance = 0.0f;
};

static std::vector<unsigned char> s_fontData;
static stbtt_fontinfo s_fontInfo;
static bool s_fontInitialized = false;
static std::unordered_map<uint64_t, CachedGlyph> s_glyphMap;

static void EnsureFontLoaded() {
    if (s_fontInitialized) return;

    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
    };

    for (const char* path : fontPaths) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            s_fontData.resize(size);
            file.read((char*)s_fontData.data(), size);
            if (stbtt_InitFont(&s_fontInfo, s_fontData.data(), stbtt_GetFontOffsetForIndex(s_fontData.data(), 0))) {
                s_fontInitialized = true;
                break;
            }
        }
    }
}

static CachedGlyph GetGlyph(wchar_t codepoint, float pixelSize) {
    EnsureFontLoaded();
    if (!s_fontInitialized) return {};

    int intSize = (int)(pixelSize + 0.5f);
    if (intSize < 8) intSize = 8;
    uint64_t key = ((uint64_t)(uint32_t)intSize << 32) | (uint32_t)codepoint;

    auto it = s_glyphMap.find(key);
    if (it != s_glyphMap.end()) {
        return it->second;
    }

    float scale = stbtt_ScaleForPixelHeight(&s_fontInfo, (float)intSize);
    int advance = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&s_fontInfo, codepoint, &advance, &lsb);

    int gw = 0, gh = 0, xoff = 0, yoff = 0;
    unsigned char* bmp = stbtt_GetCodepointBitmap(&s_fontInfo, scale, scale, codepoint, &gw, &gh, &xoff, &yoff);

    CachedGlyph g;
    g.width = gw;
    g.height = gh;
    g.xOffset = xoff;
    g.yOffset = yoff;
    g.advance = advance * scale;

    if (bmp && gw > 0 && gh > 0) {
        std::vector<uint32_t> rgba(gw * gh);
        for (int i = 0; i < gw * gh; ++i) {
            uint32_t a = bmp[i];
            rgba[i] = (a << 24) | 0x00FFFFFF;
        }

        glGenTextures(1, &g.texId);
        glBindTexture(GL_TEXTURE_2D, g.texId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gw, gh, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        stbtt_FreeBitmap(bmp, nullptr);
    } else if (bmp) {
        stbtt_FreeBitmap(bmp, nullptr);
    }

    s_glyphMap[key] = g;
    return g;
}

// ID2D1DeviceContext Implementation
ID2D1DeviceContext::ID2D1DeviceContext() = default;
ID2D1DeviceContext::~ID2D1DeviceContext() = default;

void ID2D1DeviceContext::SetViewport(int width, int height) {
    m_vpWidth = width;
    m_vpHeight = height;
}

void ID2D1DeviceContext::BeginDraw() {
    glViewport(0, 0, m_vpWidth, m_vpHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)m_vpWidth, (double)m_vpHeight, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
}

HRESULT ID2D1DeviceContext::EndDraw() {
    return S_OK;
}

void ID2D1DeviceContext::Clear(const D2D1_COLOR_F& c) {
    glClearColor(c.r, c.g, c.b, c.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ID2D1DeviceContext::SetTransform(const D2D1_MATRIX_3X2_F& m) {
    m_currentTransform = m;
    float glM[16] = {
        m.m11, m.m12, 0.0f, 0.0f,
        m.m21, m.m22, 0.0f, 0.0f,
        0.0f,  0.0f,  1.0f, 0.0f,
        m.dx,  m.dy,  0.0f, 1.0f
    };
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glM);
}

HRESULT ID2D1DeviceContext::CreateSolidColorBrush(const D2D1_COLOR_F& color, ID2D1SolidColorBrush** brush) {
    if (!brush) return E_FAIL;
    auto b = new ID2D1SolidColorBrush();
    b->color = color;
    b->opacity = color.a;
    *brush = b;
    return S_OK;
}

HRESULT ID2D1DeviceContext::CreateBitmap(
    D2D1_SIZE_U size,
    const void* srcData,
    UINT32 /*pitch*/,
    const D2D1_BITMAP_PROPERTIES1& /*properties*/,
    ID2D1Bitmap1** bitmap
) {
    if (!bitmap) return E_FAIL;
    auto bmp = new ID2D1Bitmap1();
    bmp->width = size.width;
    bmp->height = size.height;

    glGenTextures(1, &bmp->textureId);
    glBindTexture(GL_TEXTURE_2D, bmp->textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (srcData) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, srcData);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    }

    *bitmap = bmp;
    return S_OK;
}

void ID2D1DeviceContext::FillRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush) {
    if (!rect || !brush) return;
    glDisable(GL_TEXTURE_2D);
    float a = brush->color.a * brush->opacity;
    glColor4f(brush->color.r, brush->color.g, brush->color.b, a);

    glBegin(GL_QUADS);
    glVertex2f(rect->left, rect->top);
    glVertex2f(rect->right, rect->top);
    glVertex2f(rect->right, rect->bottom);
    glVertex2f(rect->left, rect->bottom);
    glEnd();
}

void ID2D1DeviceContext::DrawRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush, float strokeWidth) {
    if (!rect || !brush) return;
    glDisable(GL_TEXTURE_2D);
    float a = brush->color.a * brush->opacity;
    glColor4f(brush->color.r, brush->color.g, brush->color.b, a);
    glLineWidth(strokeWidth);

    glBegin(GL_LINE_LOOP);
    glVertex2f(rect->left, rect->top);
    glVertex2f(rect->right, rect->top);
    glVertex2f(rect->right, rect->bottom);
    glVertex2f(rect->left, rect->bottom);
    glEnd();
}

static void BuildRoundedCorners(const D2D1_ROUNDED_RECT* rr, std::vector<D2D1_POINT_2F>& pts, int stepsPerCorner = 8) {
    float l = rr->rect.left;
    float t = rr->rect.top;
    float r = rr->rect.right;
    float b = rr->rect.bottom;
    float rx = (std::min)(rr->radiusX, (r - l) * 0.5f);
    float ry = (std::min)(rr->radiusY, (b - t) * 0.5f);

    const float PI = 3.14159265358979323846f;
    // Top-Right corner: -PI/2 to 0
    for (int i = 0; i <= stepsPerCorner; ++i) {
        float angle = -PI * 0.5f + (PI * 0.5f * i / stepsPerCorner);
        pts.push_back({ (r - rx) + rx * std::cos(angle), (t + ry) + ry * std::sin(angle) });
    }
    // Bottom-Right corner: 0 to PI/2
    for (int i = 0; i <= stepsPerCorner; ++i) {
        float angle = 0.0f + (PI * 0.5f * i / stepsPerCorner);
        pts.push_back({ (r - rx) + rx * std::cos(angle), (b - ry) + ry * std::sin(angle) });
    }
    // Bottom-Left corner: PI/2 to PI
    for (int i = 0; i <= stepsPerCorner; ++i) {
        float angle = PI * 0.5f + (PI * 0.5f * i / stepsPerCorner);
        pts.push_back({ (l + rx) + rx * std::cos(angle), (b - ry) + ry * std::sin(angle) });
    }
    // Top-Left corner: PI to 3*PI/2
    for (int i = 0; i <= stepsPerCorner; ++i) {
        float angle = PI + (PI * 0.5f * i / stepsPerCorner);
        pts.push_back({ (l + rx) + rx * std::cos(angle), (t + ry) + ry * std::sin(angle) });
    }
}

void ID2D1DeviceContext::FillRoundedRectangle(const D2D1_ROUNDED_RECT* roundedRect, ID2D1SolidColorBrush* brush) {
    if (!roundedRect || !brush) return;
    glDisable(GL_TEXTURE_2D);
    float a = brush->color.a * brush->opacity;
    glColor4f(brush->color.r, brush->color.g, brush->color.b, a);

    std::vector<D2D1_POINT_2F> pts;
    pts.reserve(36);
    BuildRoundedCorners(roundedRect, pts);

    float cx = (roundedRect->rect.left + roundedRect->rect.right) * 0.5f;
    float cy = (roundedRect->rect.top + roundedRect->rect.bottom) * 0.5f;

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (const auto& p : pts) {
        glVertex2f(p.x, p.y);
    }
    if (!pts.empty()) {
        glVertex2f(pts.front().x, pts.front().y);
    }
    glEnd();
}

void ID2D1DeviceContext::DrawRoundedRectangle(const D2D1_ROUNDED_RECT* roundedRect, ID2D1SolidColorBrush* brush, float strokeWidth) {
    if (!roundedRect || !brush) return;
    glDisable(GL_TEXTURE_2D);
    float a = brush->color.a * brush->opacity;
    glColor4f(brush->color.r, brush->color.g, brush->color.b, a);
    glLineWidth(strokeWidth);

    std::vector<D2D1_POINT_2F> pts;
    pts.reserve(36);
    BuildRoundedCorners(roundedRect, pts);

    glBegin(GL_LINE_LOOP);
    for (const auto& p : pts) {
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

void ID2D1DeviceContext::DrawLine(D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1SolidColorBrush* brush, float strokeWidth) {
    if (!brush) return;
    glDisable(GL_TEXTURE_2D);
    float a = brush->color.a * brush->opacity;
    glColor4f(brush->color.r, brush->color.g, brush->color.b, a);
    glLineWidth(strokeWidth);

    glBegin(GL_LINES);
    glVertex2f(p0.x, p0.y);
    glVertex2f(p1.x, p1.y);
    glEnd();
}

void ID2D1DeviceContext::DrawBitmap(
    ID2D1Bitmap* bitmap,
    const D2D1_RECT_F& dst,
    float opacity,
    D2D1_INTERPOLATION_MODE interpolationMode,
    const D2D1_RECT_F* src
) {
    if (!bitmap || bitmap->textureId == 0) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bitmap->textureId);

    int filter = (interpolationMode == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glColor4f(1.0f, 1.0f, 1.0f, opacity);

    float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    if (src && bitmap->width > 0 && bitmap->height > 0) {
        u0 = src->left / (float)bitmap->width;
        v0 = src->top / (float)bitmap->height;
        u1 = src->right / (float)bitmap->width;
        v1 = src->bottom / (float)bitmap->height;
    }

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(dst.left, dst.top);
    glTexCoord2f(u1, v0); glVertex2f(dst.right, dst.top);
    glTexCoord2f(u1, v1); glVertex2f(dst.right, dst.bottom);
    glTexCoord2f(u0, v1); glVertex2f(dst.left, dst.bottom);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void ID2D1DeviceContext::DrawText(
    const wchar_t* string,
    UINT32 stringLength,
    IDWriteTextFormat* textFormat,
    const D2D1_RECT_F& layoutRect,
    ID2D1SolidColorBrush* defaultFillBrush,
    UINT /*options*/,
    int /*measuringMode*/
) {
    if (!string || stringLength == 0 || !textFormat || !defaultFillBrush) return;

    float fontSize = textFormat->fontSize;
    float totalW = 0.0f;

    std::vector<CachedGlyph> glyphs;
    glyphs.reserve(stringLength);

    for (UINT32 i = 0; i < stringLength; ++i) {
        CachedGlyph g = GetGlyph(string[i], fontSize);
        glyphs.push_back(g);
        totalW += g.advance;
    }

    // Alignment calculation
    float startX = layoutRect.left;
    float boxW = layoutRect.right - layoutRect.left;
    if (textFormat->textAlignment == DWRITE_TEXT_ALIGNMENT_CENTER) {
        startX += (boxW - totalW) * 0.5f;
    } else if (textFormat->textAlignment == DWRITE_TEXT_ALIGNMENT_TRAILING) {
        startX += (boxW - totalW);
    }

    float boxH = layoutRect.bottom - layoutRect.top;
    float baselineY = layoutRect.top + fontSize * 0.8f;
    if (textFormat->paragraphAlignment == DWRITE_PARAGRAPH_ALIGNMENT_CENTER) {
        baselineY = layoutRect.top + (boxH - fontSize) * 0.5f + fontSize * 0.8f;
    }

    glEnable(GL_TEXTURE_2D);
    float a = defaultFillBrush->color.a * defaultFillBrush->opacity;
    glColor4f(defaultFillBrush->color.r, defaultFillBrush->color.g, defaultFillBrush->color.b, a);

    float curX = startX;
    for (const auto& g : glyphs) {
        if (g.texId != 0 && g.width > 0 && g.height > 0) {
            glBindTexture(GL_TEXTURE_2D, g.texId);
            float gx0 = curX + g.xOffset;
            float gy0 = baselineY + g.yOffset;
            float gx1 = gx0 + g.width;
            float gy1 = gy0 + g.height;

            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(gx0, gy0);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(gx1, gy0);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(gx1, gy1);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(gx0, gy1);
            glEnd();
        }
        curX += g.advance;
    }

    glDisable(GL_TEXTURE_2D);
}

void ID2D1DeviceContext::PushAxisAlignedClip(const D2D1_RECT_F* clipRect, D2D1_ANTIALIAS_MODE) {
    if (!clipRect) return;
    glEnable(GL_SCISSOR_TEST);
    int x = static_cast<int>(clipRect->left);
    int y = static_cast<int>(m_vpHeight - clipRect->bottom);
    int w = static_cast<int>(clipRect->right - clipRect->left);
    int h = static_cast<int>(clipRect->bottom - clipRect->top);
    if (w < 0) { x += w; w = -w; }
    if (h < 0) { y += h; h = -h; }
    glScissor(x, y, w, h);
}

void ID2D1DeviceContext::PopAxisAlignedClip() {
    glDisable(GL_SCISSOR_TEST);
}

#endif
