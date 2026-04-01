//
// Created by kenne on 01/04/2026.
//

#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <ranges>
#include <stb_image.h>
#include <olectl.h>   // OleCreatePictureIndirect, PICTDESC

// Decode a PNG from any contiguous byte range and return it as an IPictureDisp.
// Accepts cmrc::file, std::vector<uint8_t>, std::span<const std::byte>, etc.
// The caller owns the returned pointer (must Release it).
template<std::ranges::contiguous_range ByteRange>
    requires (sizeof(std::ranges::range_value_t<ByteRange>) == 1)
static IPictureDisp* ImageFromPNGBytes(const ByteRange& data)
{
    const auto* bytes = reinterpret_cast<const stbi_uc*>(std::ranges::data(data));
    const int   size  = static_cast<int>(std::ranges::size(data));

    // Decode PNG directly from the memory pointer — no HGLOBAL or IStream needed.
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(bytes, size,
                                                   &width, &height, &channels,
                                                   4 /*force RGBA output*/);
    if (!pixels) return nullptr;

    // GDI expects BGRA; stb_image delivers RGBA — swap R and B in-place.
    for (int i = 0; i < width * height; ++i)
        std::swap(pixels[i * 4 + 0], pixels[i * 4 + 2]);

    // Create a 32-bit top-down DIB and copy the pixel data into it.
    BITMAPV4HEADER bmi   = {};
    bmi.bV4Size          = sizeof(bmi);
    bmi.bV4Width         = width;
    bmi.bV4Height        = -height;       // negative = top-down row order
    bmi.bV4Planes        = 1;
    bmi.bV4BitCount      = 32;
    bmi.bV4V4Compression = BI_BITFIELDS;
    bmi.bV4RedMask       = 0x00FF0000;
    bmi.bV4GreenMask     = 0x0000FF00;
    bmi.bV4BlueMask      = 0x000000FF;
    bmi.bV4AlphaMask     = 0xFF000000;

    void*   pBits   = nullptr;
    HBITMAP hBitmap = CreateDIBSection(nullptr,
                                       reinterpret_cast<BITMAPINFO*>(&bmi),
                                       DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hBitmap) { stbi_image_free(pixels); return nullptr; }

    memcpy(pBits, pixels, static_cast<size_t>(width) * height * 4);
    stbi_image_free(pixels);

    // Wrap the HBITMAP in an OLE IPictureDisp that Office's ribbon can consume.
    PICTDESC pd       = {};
    pd.cbSizeofstruct = sizeof(pd);
    pd.picType        = PICTYPE_BITMAP;
    pd.bmp.hbitmap    = hBitmap;

    IPictureDisp* pPicture = nullptr;
    if (FAILED(OleCreatePictureIndirect(&pd, IID_IPictureDisp,
                                        TRUE /*picture owns hBitmap*/,
                                        reinterpret_cast<void**>(&pPicture))))
    {
        DeleteObject(hBitmap);
        return nullptr;
    }
    return pPicture;
}