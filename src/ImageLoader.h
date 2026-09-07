#pragma once

#if defined(_WIN32)
#include <windows.h>
#include <wincodec.h>
#include <d2d1_2.h>
#else
#include "platform/PlatformDefs.h"
#include "platform/D2DCompat.h"
#include <cstring>

struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
    bool operator==(const GUID& o) const {
        return std::memcmp(this, &o, sizeof(GUID)) == 0;
    }
};
static const GUID GUID_ContainerFormatPng = { 0x1b7cfaf4, 0x713f, 0x473c, { 0xbb, 0xcd, 0x61, 0x37, 0x42, 0x5f, 0xae, 0xaf } };
static const GUID GUID_ContainerFormatJpeg = { 0x19e4a5aa, 0x5662, 0x4fc5, { 0xa0, 0xc0, 0x17, 0x58, 0x02, 0x8e, 0x10, 0x57 } };
typedef void IWICImagingFactory;
typedef void IWICBitmapFrameDecode;
enum WICBitmapTransformOptions { WICBitmapTransformRotate0 = 0 };
#endif
#include <string>
#include <vector>
#include <memory>

struct FrameData {
    ID2D1Bitmap1* bitmap = nullptr;
    float delaySeconds = 0.1f;
};

class ImageLoader {
public:
    ImageLoader();
    ~ImageLoader();

    bool Initialize();

    // Loads an image file and creates Direct2D bitmap(s).
    // If it's an animated GIF, outFrames will contain all frames and delay timings.
    bool LoadImageFromFile(
        ID2D1DeviceContext* d2dContext,
        const std::wstring& filePath,
        ID2D1Bitmap1** outBitmap,
        UINT* outWidth,
        UINT* outHeight,
        std::vector<FrameData>* outFrames = nullptr,
        std::vector<uint32_t>* outPixels = nullptr
    );

    // Encodes 32bpp PBGRA pixels to PNG or JPEG file using WIC
    bool SavePixelsToFile(
        const std::wstring& filePath,
        const uint32_t* pixels,
        UINT width,
        UINT height,
        GUID containerFormat = GUID_ContainerFormatPng
    );

    IWICImagingFactory* GetFactory() const { return m_wicFactory; }

private:
    IWICImagingFactory* m_wicFactory = nullptr;

    WICBitmapTransformOptions GetExifOrientationTransform(IWICBitmapFrameDecode* frame);
    float GetFrameDelay(IWICBitmapFrameDecode* frame);
};
