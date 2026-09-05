#pragma once
#include <windows.h>
#include <wincodec.h>
#include <d2d1_2.h>
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
