#include "ImageLoader.h"
#include <propvarutil.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "propsys.lib")

ImageLoader::ImageLoader() = default;

ImageLoader::~ImageLoader() {
    if (m_wicFactory) {
        m_wicFactory->Release();
        m_wicFactory = nullptr;
    }
}

bool ImageLoader::Initialize() {
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory)
    );
    return SUCCEEDED(hr) && (m_wicFactory != nullptr);
}

WICBitmapTransformOptions ImageLoader::GetExifOrientationTransform(IWICBitmapFrameDecode* frame) {
    if (!frame) return WICBitmapTransformRotate0;

    IWICMetadataQueryReader* queryReader = nullptr;
    if (FAILED(frame->GetMetadataQueryReader(&queryReader)) || !queryReader) {
        return WICBitmapTransformRotate0;
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    WICBitmapTransformOptions transform = WICBitmapTransformRotate0;

    if (SUCCEEDED(queryReader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &value))) {
        if (value.vt == VT_UI2) {
            switch (value.uiVal) {
            case 1: transform = WICBitmapTransformRotate0; break;
            case 2: transform = WICBitmapTransformFlipHorizontal; break;
            case 3: transform = WICBitmapTransformRotate180; break;
            case 4: transform = WICBitmapTransformFlipVertical; break;
            case 5: transform = (WICBitmapTransformOptions)(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal); break;
            case 6: transform = WICBitmapTransformRotate90; break;
            case 7: transform = (WICBitmapTransformOptions)(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal); break;
            case 8: transform = WICBitmapTransformRotate270; break;
            default: transform = WICBitmapTransformRotate0; break;
            }
        }
        PropVariantClear(&value);
    }

    queryReader->Release();
    return transform;
}

float ImageLoader::GetFrameDelay(IWICBitmapFrameDecode* frame) {
    if (!frame) return 0.1f;

    IWICMetadataQueryReader* queryReader = nullptr;
    if (FAILED(frame->GetMetadataQueryReader(&queryReader)) || !queryReader) {
        return 0.1f;
    }

    PROPVARIANT value;
    PropVariantInit(&value);
    float delaySeconds = 0.1f;

    // Standard GIF Graphic Control Extension delay tag (/grctlext/Delay)
    if (SUCCEEDED(queryReader->GetMetadataByName(L"/grctlext/Delay", &value))) {
        if (value.vt == VT_UI2) {
            // Units are 1/100th of a second
            UINT hundredths = value.uiVal;
            // Web browsers use a minimum of 100ms if delay is <= 1
            if (hundredths <= 1) hundredths = 10;
            delaySeconds = hundredths / 100.0f;
        }
        PropVariantClear(&value);
    }

    queryReader->Release();
    return delaySeconds;
}

bool ImageLoader::LoadImageFromFile(
    ID2D1DeviceContext* d2dContext,
    const std::wstring& filePath,
    ID2D1Bitmap1** outBitmap,
    UINT* outWidth,
    UINT* outHeight,
    std::vector<FrameData>* outFrames,
    std::vector<uint32_t>* outPixels
) {
    if (!m_wicFactory || !d2dContext || !outBitmap) return false;
    *outBitmap = nullptr;
    if (outWidth) *outWidth = 0;
    if (outHeight) *outHeight = 0;
    if (outFrames) outFrames->clear();
    if (outPixels) outPixels->clear();

    IWICBitmapDecoder* decoder = nullptr;
    HRESULT hr = m_wicFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &decoder
    );

    if (FAILED(hr) || !decoder) return false;

    UINT frameCount = 1;
    decoder->GetFrameCount(&frameCount);

    // Multi-frame animated GIF decoding with full GIF89a canvas composition and disposal handling
    if (frameCount > 1 && outFrames != nullptr) {
        UINT canvasWidth = 0;
        UINT canvasHeight = 0;

        IWICMetadataQueryReader* decoderReader = nullptr;
        if (SUCCEEDED(decoder->GetMetadataQueryReader(&decoderReader)) && decoderReader) {
            PROPVARIANT val;
            PropVariantInit(&val);
            if (SUCCEEDED(decoderReader->GetMetadataByName(L"/logscrdesc/Width", &val)) && val.vt == VT_UI2) {
                canvasWidth = val.uiVal;
            }
            PropVariantClear(&val);
            if (SUCCEEDED(decoderReader->GetMetadataByName(L"/logscrdesc/Height", &val)) && val.vt == VT_UI2) {
                canvasHeight = val.uiVal;
            }
            PropVariantClear(&val);
            decoderReader->Release();
        }

        if (canvasWidth == 0 || canvasHeight == 0) {
            IWICBitmapFrameDecode* frame0 = nullptr;
            if (SUCCEEDED(decoder->GetFrame(0, &frame0)) && frame0) {
                frame0->GetSize(&canvasWidth, &canvasHeight);
                frame0->Release();
            }
        }
        if (canvasWidth == 0 || canvasHeight == 0) {
            canvasWidth = 1;
            canvasHeight = 1;
        }

        std::vector<uint32_t> canvas(canvasWidth * canvasHeight, 0);
        std::vector<uint32_t> previousCanvasSnapshot;

        UINT prevDisposal = 0;
        UINT prevLeft = 0, prevTop = 0, prevWidth = 0, prevHeight = 0;

        for (UINT i = 0; i < frameCount; ++i) {
            IWICBitmapFrameDecode* frame = nullptr;
            hr = decoder->GetFrame(i, &frame);
            if (FAILED(hr) || !frame) continue;

            float delay = GetFrameDelay(frame);
            UINT fw = 0, fh = 0;
            frame->GetSize(&fw, &fh);

            UINT left = 0, top = 0;
            BYTE disposal = 0;

            IWICMetadataQueryReader* reader = nullptr;
            if (SUCCEEDED(frame->GetMetadataQueryReader(&reader)) && reader) {
                PROPVARIANT val;
                PropVariantInit(&val);
                if (SUCCEEDED(reader->GetMetadataByName(L"/imgdesc/Left", &val)) && val.vt == VT_UI2) {
                    left = val.uiVal;
                }
                PropVariantClear(&val);
                if (SUCCEEDED(reader->GetMetadataByName(L"/imgdesc/Top", &val)) && val.vt == VT_UI2) {
                    top = val.uiVal;
                }
                PropVariantClear(&val);
                if (SUCCEEDED(reader->GetMetadataByName(L"/grctlext/Disposal", &val)) && val.vt == VT_UI1) {
                    disposal = val.bVal;
                }
                PropVariantClear(&val);
                reader->Release();
            }

            // Apply disposal method of the PREVIOUS frame
            if (i > 0) {
                if (prevDisposal == 2) {
                    // Restore to background (transparent)
                    for (UINT y = 0; y < prevHeight; ++y) {
                        UINT cy = prevTop + y;
                        if (cy >= canvasHeight) break;
                        for (UINT x = 0; x < prevWidth; ++x) {
                            UINT cx = prevLeft + x;
                            if (cx >= canvasWidth) break;
                            canvas[cy * canvasWidth + cx] = 0;
                        }
                    }
                } else if (prevDisposal == 3 && !previousCanvasSnapshot.empty()) {
                    // Restore to previous snapshot
                    canvas = previousCanvasSnapshot;
                }
            }

            // If current frame requests disposal 3, save snapshot before rendering this frame
            if (disposal == 3) {
                previousCanvasSnapshot = canvas;
            }

            // Convert frame to 32bpp PBGRA
            IWICFormatConverter* converter = nullptr;
            hr = m_wicFactory->CreateFormatConverter(&converter);
            if (SUCCEEDED(hr)) {
                hr = converter->Initialize(
                    frame,
                    GUID_WICPixelFormat32bppPBGRA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0f,
                    WICBitmapPaletteTypeCustom
                );
            }

            if (SUCCEEDED(hr) && converter) {
                std::vector<uint32_t> framePixels(fw * fh);
                hr = converter->CopyPixels(
                    nullptr,
                    fw * sizeof(uint32_t),
                    static_cast<UINT>(framePixels.size() * sizeof(uint32_t)),
                    reinterpret_cast<BYTE*>(framePixels.data())
                );

                if (SUCCEEDED(hr)) {
                    // Composite current frame on canvas at (left, top)
                    for (UINT y = 0; y < fh; ++y) {
                        UINT cy = top + y;
                        if (cy >= canvasHeight) break;
                        for (UINT x = 0; x < fw; ++x) {
                            UINT cx = left + x;
                            if (cx >= canvasWidth) break;
                            uint32_t px = framePixels[y * fw + x];
                            uint8_t a = static_cast<uint8_t>((px >> 24) & 0xFF);
                            if (a > 0) {
                                canvas[cy * canvasWidth + cx] = px;
                            }
                        }
                    }

                    // Create Direct2D bitmap of full canvas
                    D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
                        D2D1_BITMAP_OPTIONS_NONE,
                        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                    );

                    ID2D1Bitmap1* bmp1 = nullptr;
                    hr = d2dContext->CreateBitmap(
                        D2D1::SizeU(canvasWidth, canvasHeight),
                        canvas.data(),
                        canvasWidth * sizeof(uint32_t),
                        &props,
                        &bmp1
                    );

                    if (SUCCEEDED(hr) && bmp1) {
                        FrameData fd;
                        fd.bitmap = bmp1;
                        fd.delaySeconds = delay;
                        outFrames->push_back(fd);
                    }
                }
            }

            if (converter) converter->Release();
            frame->Release();

            prevDisposal = disposal;
            prevLeft = left;
            prevTop = top;
            prevWidth = fw;
            prevHeight = fh;
        }

        if (!outFrames->empty()) {
            if (outWidth) *outWidth = canvasWidth;
            if (outHeight) *outHeight = canvasHeight;
            if (outPixels) {
                *outPixels = canvas;
            }
            *outBitmap = (*outFrames)[0].bitmap;
            (*outBitmap)->AddRef();
            decoder->Release();
            return true;
        }
    }

    // Single frame fallback (JPEG, PNG, WebP, single-frame GIF, etc.)
    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) {
        decoder->Release();
        return false;
    }

    WICBitmapTransformOptions transform = GetExifOrientationTransform(frame);

    IWICBitmapSource* source = frame;
    frame->AddRef();

    // Apply EXIF rotation if needed
    if (transform != WICBitmapTransformRotate0) {
        IWICBitmapFlipRotator* rotator = nullptr;
        if (SUCCEEDED(m_wicFactory->CreateBitmapFlipRotator(&rotator))) {
            if (SUCCEEDED(rotator->Initialize(source, transform))) {
                source->Release();
                source = rotator;
                source->AddRef();
            }
            rotator->Release();
        }
    }

    // Convert pixel format to 32bpp PBGRA (Premultiplied BGRA)
    IWICFormatConverter* converter = nullptr;
    hr = m_wicFactory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(
            source,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
        );
    }

    if (SUCCEEDED(hr) && converter) {
        UINT w = 0, h = 0;
        converter->GetSize(&w, &h);

        if (w > 0 && h > 0) {
            UINT stride = w * sizeof(uint32_t);
            UINT bufferSize = stride * h;
            std::vector<uint32_t> pixels(w * h);
            hr = converter->CopyPixels(nullptr, stride, bufferSize, reinterpret_cast<BYTE*>(pixels.data()));
            if (SUCCEEDED(hr)) {
                D2D1_BITMAP_PROPERTIES1 bp = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_NONE,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
                );
                hr = d2dContext->CreateBitmap(
                    D2D1::SizeU(w, h),
                    pixels.data(),
                    stride,
                    &bp,
                    outBitmap
                );
                if (SUCCEEDED(hr) && *outBitmap) {
                    if (outWidth) *outWidth = w;
                    if (outHeight) *outHeight = h;
                    if (outPixels) {
                        *outPixels = std::move(pixels);
                    }
                }
            }
        }
    }

    if (converter) converter->Release();
    if (source) source->Release();
    frame->Release();
    decoder->Release();

    return (*outBitmap != nullptr);
}

bool ImageLoader::SavePixelsToFile(
    const std::wstring& filePath,
    const uint32_t* pixels,
    UINT width,
    UINT height,
    GUID containerFormat
) {
    if (!m_wicFactory || !pixels || width == 0 || height == 0) return false;

    IWICStream* stream = nullptr;
    HRESULT hr = m_wicFactory->CreateStream(&stream);
    if (FAILED(hr)) return false;

    hr = stream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) {
        stream->Release();
        return false;
    }

    IWICBitmapEncoder* encoder = nullptr;
    hr = m_wicFactory->CreateEncoder(containerFormat, nullptr, &encoder);
    if (FAILED(hr)) {
        stream->Release();
        return false;
    }

    hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) {
        encoder->Release();
        stream->Release();
        return false;
    }

    IWICBitmapFrameEncode* frameEncode = nullptr;
    hr = encoder->CreateNewFrame(&frameEncode, nullptr);
    if (FAILED(hr)) {
        encoder->Release();
        stream->Release();
        return false;
    }

    hr = frameEncode->Initialize(nullptr);
    if (SUCCEEDED(hr)) {
        hr = frameEncode->SetSize(width, height);
    }

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
    if (containerFormat == GUID_ContainerFormatJpeg) {
        format = GUID_WICPixelFormat24bppBGR;
    }
    if (SUCCEEDED(hr)) {
        hr = frameEncode->SetPixelFormat(&format);
    }

    if (containerFormat == GUID_ContainerFormatJpeg) {
        // Convert 32bpp PBGRA to 24bpp BGR composited over white background for JPEG
        std::vector<BYTE> bgr24(width * height * 3);
        for (UINT i = 0; i < width * height; ++i) {
            uint32_t px = pixels[i];
            BYTE b = static_cast<BYTE>(px & 0xFF);
            BYTE g = static_cast<BYTE>((px >> 8) & 0xFF);
            BYTE r = static_cast<BYTE>((px >> 16) & 0xFF);
            BYTE a = static_cast<BYTE>((px >> 24) & 0xFF);
            if (a == 0) {
                bgr24[i * 3 + 0] = 255;
                bgr24[i * 3 + 1] = 255;
                bgr24[i * 3 + 2] = 255;
            } else if (a < 255) {
                float af = a / 255.0f;
                bgr24[i * 3 + 0] = static_cast<BYTE>((std::min)(255.0f, b + (1.0f - af) * 255.0f));
                bgr24[i * 3 + 1] = static_cast<BYTE>((std::min)(255.0f, g + (1.0f - af) * 255.0f));
                bgr24[i * 3 + 2] = static_cast<BYTE>((std::min)(255.0f, r + (1.0f - af) * 255.0f));
            } else {
                bgr24[i * 3 + 0] = b;
                bgr24[i * 3 + 1] = g;
                bgr24[i * 3 + 2] = r;
            }
        }
        UINT bgrStride = width * 3;
        hr = frameEncode->WritePixels(height, bgrStride, static_cast<UINT>(bgr24.size()), bgr24.data());
    } else {
        UINT stride = width * sizeof(uint32_t);
        UINT bufferSize = stride * height;
        hr = frameEncode->WritePixels(height, stride, bufferSize, reinterpret_cast<BYTE*>(const_cast<uint32_t*>(pixels)));
    }

    if (SUCCEEDED(hr)) {
        hr = frameEncode->Commit();
    }
    if (SUCCEEDED(hr)) {
        hr = encoder->Commit();
    }

    frameEncode->Release();
    encoder->Release();
    stream->Release();

    return SUCCEEDED(hr);
}
