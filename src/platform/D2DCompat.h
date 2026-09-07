#pragma once
#include "PlatformDefs.h"

#if defined(_WIN32)
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <d2d1_2.h>
#include <dwrite.h>
#include <dcomp.h>
#else

#include <vector>
#include <string>
#include <cmath>

struct D2D1_COLOR_F {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

namespace D2D1 {
    inline D2D1_COLOR_F ColorF(float r, float g, float b, float a = 1.0f) {
        return { r, g, b, a };
    }
    enum StandardColors {
        White = 0xFFFFFF,
        Black = 0x000000
    };
    inline D2D1_COLOR_F ColorF(StandardColors sc, float a = 1.0f) {
        float r = ((sc >> 16) & 0xFF) / 255.0f;
        float g = ((sc >> 8) & 0xFF) / 255.0f;
        float b = (sc & 0xFF) / 255.0f;
        return { r, g, b, a };
    }
}

struct D2D1_POINT_2F {
    float x = 0.0f;
    float y = 0.0f;
};

namespace D2D1 {
    inline D2D1_POINT_2F Point2F(float x = 0.0f, float y = 0.0f) {
        return { x, y };
    }
}

struct D2D1_RECT_F {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

namespace D2D1 {
    inline D2D1_RECT_F RectF(float l = 0.0f, float t = 0.0f, float r = 0.0f, float b = 0.0f) {
        return { l, t, r, b };
    }
}

struct D2D1_ROUNDED_RECT {
    D2D1_RECT_F rect;
    float radiusX = 0.0f;
    float radiusY = 0.0f;
};

namespace D2D1 {
    inline D2D1_ROUNDED_RECT RoundedRect(const D2D1_RECT_F& rc, float rx, float ry) {
        return { rc, rx, ry };
    }
}

struct D2D1_SIZE_U {
    UINT width = 0;
    UINT height = 0;
};

struct D2D1_SIZE_F {
    float width = 0.0f;
    float height = 0.0f;
};

namespace D2D1 {
    inline D2D1_SIZE_U SizeU(UINT w = 0, UINT h = 0) {
        return { w, h };
    }
    inline D2D1_SIZE_F SizeF(float w = 0.0f, float h = 0.0f) {
        return { w, h };
    }
}

struct D2D1_MATRIX_3X2_F {
    float m11 = 1.0f, m12 = 0.0f;
    float m21 = 0.0f, m22 = 1.0f;
    float dx = 0.0f, dy = 0.0f;

    static D2D1_MATRIX_3X2_F Identity() {
        return { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    }

    static D2D1_MATRIX_3X2_F Translation(float x, float y) {
        return { 1.0f, 0.0f, 0.0f, 1.0f, x, y };
    }

    static D2D1_MATRIX_3X2_F Scale(float sx, float sy) {
        return { sx, 0.0f, 0.0f, sy, 0.0f, 0.0f };
    }

    static D2D1_MATRIX_3X2_F Rotation(float angleDegrees, D2D1_POINT_2F center) {
        float rad = angleDegrees * 3.14159265358979323846f / 180.0f;
        float c = std::cos(rad);
        float s = std::sin(rad);
        return {
            c, s,
            -s, c,
            center.x * (1.0f - c) + center.y * s,
            center.y * (1.0f - c) - center.x * s
        };
    }

    D2D1_MATRIX_3X2_F operator*(const D2D1_MATRIX_3X2_F& o) const {
        return {
            m11 * o.m11 + m12 * o.m21,
            m11 * o.m12 + m12 * o.m22,
            m21 * o.m11 + m22 * o.m21,
            m21 * o.m12 + m22 * o.m22,
            dx * o.m11 + dy * o.m21 + o.dx,
            dx * o.m12 + dy * o.m22 + o.dy
        };
    }
};

namespace D2D1 {
    namespace Matrix3x2F {
        inline D2D1_MATRIX_3X2_F Identity() { return D2D1_MATRIX_3X2_F::Identity(); }
        inline D2D1_MATRIX_3X2_F Translation(float x, float y) { return D2D1_MATRIX_3X2_F::Translation(x, y); }
        inline D2D1_MATRIX_3X2_F Scale(float sx, float sy) { return D2D1_MATRIX_3X2_F::Scale(sx, sy); }
        inline D2D1_MATRIX_3X2_F Rotation(float deg, D2D1_POINT_2F c) { return D2D1_MATRIX_3X2_F::Rotation(deg, c); }
    }
}

enum D2D1_INTERPOLATION_MODE {
    D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR = 0,
    D2D1_INTERPOLATION_MODE_LINEAR = 1
};

enum DWRITE_TEXT_ALIGNMENT {
    DWRITE_TEXT_ALIGNMENT_LEADING = 0,
    DWRITE_TEXT_ALIGNMENT_TRAILING = 1,
    DWRITE_TEXT_ALIGNMENT_CENTER = 2,
    DWRITE_TEXT_ALIGNMENT_JUSTIFIED = 3
};

enum DWRITE_PARAGRAPH_ALIGNMENT {
    DWRITE_PARAGRAPH_ALIGNMENT_NEAR = 0,
    DWRITE_PARAGRAPH_ALIGNMENT_FAR = 1,
    DWRITE_PARAGRAPH_ALIGNMENT_CENTER = 2
};

enum DWRITE_FONT_WEIGHT {
    DWRITE_FONT_WEIGHT_NORMAL = 400,
    DWRITE_FONT_WEIGHT_MEDIUM = 500,
    DWRITE_FONT_WEIGHT_SEMI_BOLD = 600,
    DWRITE_FONT_WEIGHT_BOLD = 700
};

enum DWRITE_FONT_STYLE {
    DWRITE_FONT_STYLE_NORMAL = 0,
    DWRITE_FONT_STYLE_OBLIQUE = 1,
    DWRITE_FONT_STYLE_ITALIC = 2
};

enum DWRITE_FONT_STRETCH {
    DWRITE_FONT_STRETCH_NORMAL = 5
};

struct D2D1_PIXEL_FORMAT {
    UINT format = 0;
    UINT alphaMode = 0;
};

struct D2D1_BITMAP_PROPERTIES1 {
    D2D1_PIXEL_FORMAT pixelFormat;
    float dpiX = 96.0f;
    float dpiY = 96.0f;
    UINT bitmapOptions = 0;
    void* colorContext = nullptr;
};

namespace D2D1 {
    inline D2D1_BITMAP_PROPERTIES1 BitmapProperties1(UINT opt = 0, D2D1_PIXEL_FORMAT pf = {0,0}, float dx = 96.0f, float dy = 96.0f) {
        D2D1_BITMAP_PROPERTIES1 p = {};
        p.pixelFormat = pf;
        p.dpiX = dx;
        p.dpiY = dy;
        p.bitmapOptions = opt;
        return p;
    }
    inline D2D1_PIXEL_FORMAT PixelFormat(UINT f = 0, UINT a = 0) {
        return { f, a };
    }
}

typedef D2D1_BITMAP_PROPERTIES1 D2D1_BITMAP_PROPERTIES;

namespace D2D1 {
    inline D2D1_BITMAP_PROPERTIES BitmapProperties(D2D1_PIXEL_FORMAT pf = {0,0}, float dx = 96.0f, float dy = 96.0f) {
        return BitmapProperties1(0, pf, dx, dy);
    }
}

#define DXGI_FORMAT_B8G8R8A8_UNORM 87
#define D2D1_ALPHA_MODE_PREMULTIPLIED 1
#define D2D1_ALPHA_MODE_STRAIGHT 2
#define D2D1_BITMAP_OPTIONS_NONE 0
#define D2D1_BITMAP_OPTIONS_TARGET 1
#define D2D1_BITMAP_OPTIONS_CANNOT_DRAW 2
#define D2D1_BITMAP_OPTIONS_CPU_READ 4

enum D2D1_ANTIALIAS_MODE {
    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE = 0,
    D2D1_ANTIALIAS_MODE_ALIASED = 1
};

class ID2D1SolidColorBrush {
public:
    D2D1_COLOR_F color = { 0, 0, 0, 1 };
    float opacity = 1.0f;

    void SetColor(const D2D1_COLOR_F& c) { color = c; }
    void SetOpacity(float op) { opacity = op; }
    float GetOpacity() const { return opacity; }
    void Release() { delete this; }
};

class ID2D1Bitmap {
public:
    uint32_t textureId = 0;
    UINT width = 0;
    UINT height = 0;
    bool hasAlpha = true;

    virtual ~ID2D1Bitmap();
    D2D1_SIZE_F GetSize() const { return { (float)width, (float)height }; }
    D2D1_SIZE_U GetPixelSize() const { return { width, height }; }
    HRESULT CopyFromMemory(const D2D1_RECT_F* dstRect, const void* srcData, UINT32 pitch);
    virtual void Release() { delete this; }
};

class ID2D1Bitmap1 : public ID2D1Bitmap {
public:
    void Release() override { delete this; }
};

class IDWriteTextFormat {
public:
    std::wstring fontFamily;
    float fontSize = 14.0f;
    DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

    HRESULT SetTextAlignment(DWRITE_TEXT_ALIGNMENT a) { textAlignment = a; return S_OK; }
    HRESULT SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT a) { paragraphAlignment = a; return S_OK; }
    void Release() { delete this; }
};

class IDWriteFactory {
public:
    HRESULT CreateTextFormat(
        const wchar_t* fontFamilyName,
        void* fontCollection,
        DWRITE_FONT_WEIGHT fontWeight,
        DWRITE_FONT_STYLE fontStyle,
        DWRITE_FONT_STRETCH fontStretch,
        float fontSize,
        const wchar_t* localeName,
        IDWriteTextFormat** textFormat
    );
    void Release() { delete this; }
};

#define DWRITE_FACTORY_TYPE_SHARED 0
#define __uuidof(x) 0
#define IUnknown void

inline HRESULT DWriteCreateFactory(int, int, void** ppFactory) {
    if (!ppFactory) return E_FAIL;
    *ppFactory = new IDWriteFactory();
    return S_OK;
}

class ID2D1DeviceContext {
public:
    ID2D1DeviceContext();
    ~ID2D1DeviceContext();

    void SetViewport(int width, int height);

    void BeginDraw();
    HRESULT EndDraw();

    void Clear(const D2D1_COLOR_F& clearColor);
    void SetTransform(const D2D1_MATRIX_3X2_F& transform);

    HRESULT CreateSolidColorBrush(const D2D1_COLOR_F& color, ID2D1SolidColorBrush** brush);
    HRESULT CreateBitmap(
        D2D1_SIZE_U size,
        const void* srcData,
        UINT32 pitch,
        const D2D1_BITMAP_PROPERTIES1& bitmapProperties,
        ID2D1Bitmap1** bitmap
    );
    HRESULT CreateBitmap(
        D2D1_SIZE_U size,
        const void* srcData,
        UINT32 pitch,
        const D2D1_BITMAP_PROPERTIES1* bitmapProperties,
        ID2D1Bitmap1** bitmap
    ) {
        static D2D1_BITMAP_PROPERTIES1 defProps;
        return CreateBitmap(size, srcData, pitch, bitmapProperties ? *bitmapProperties : defProps, bitmap);
    }
    HRESULT CreateBitmap(
        D2D1_SIZE_U size,
        const void* srcData,
        UINT32 pitch,
        const D2D1_BITMAP_PROPERTIES& bitmapProperties,
        ID2D1Bitmap** bitmap
    ) {
        return CreateBitmap(size, srcData, pitch, bitmapProperties, (ID2D1Bitmap1**)bitmap);
    }
    HRESULT CreateBitmap(
        D2D1_SIZE_U size,
        const void* srcData,
        UINT32 pitch,
        const D2D1_BITMAP_PROPERTIES* bitmapProperties,
        ID2D1Bitmap** bitmap
    ) {
        static D2D1_BITMAP_PROPERTIES1 defProps;
        return CreateBitmap(size, srcData, pitch, bitmapProperties ? *bitmapProperties : defProps, (ID2D1Bitmap1**)bitmap);
    }

    void FillRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush);
    void DrawRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush, float strokeWidth = 1.0f);
    void FillRoundedRectangle(const D2D1_ROUNDED_RECT* roundedRect, ID2D1SolidColorBrush* brush);
    void DrawRoundedRectangle(const D2D1_ROUNDED_RECT* roundedRect, ID2D1SolidColorBrush* brush, float strokeWidth = 1.0f);
    void DrawLine(D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1SolidColorBrush* brush, float strokeWidth = 1.0f);

    void DrawBitmap(
        ID2D1Bitmap* bitmap,
        const D2D1_RECT_F& destinationRectangle,
        float opacity = 1.0f,
        D2D1_INTERPOLATION_MODE interpolationMode = D2D1_INTERPOLATION_MODE_LINEAR,
        const D2D1_RECT_F* sourceRectangle = nullptr
    );

    void DrawText(
        const wchar_t* string,
        UINT32 stringLength,
        IDWriteTextFormat* textFormat,
        const D2D1_RECT_F& layoutRect,
        ID2D1SolidColorBrush* defaultFillBrush,
        UINT options = 0,
        int measuringMode = 0
    );

    void PushAxisAlignedClip(const D2D1_RECT_F* clipRect, D2D1_ANTIALIAS_MODE antialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    void PopAxisAlignedClip();

    void Release() { delete this; }

private:
    int m_vpWidth = 800;
    int m_vpHeight = 600;
    D2D1_MATRIX_3X2_F m_currentTransform = D2D1_MATRIX_3X2_F::Identity();
};

#endif
