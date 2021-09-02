/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WxHelpers.h"

#include <GameCore/GameException.h>

#include <wx/rawbmp.h>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

wxBitmap WxHelpers::LoadBitmap(
    std::string const & bitmapName,
    ResourceLocator const & resourceLocator)
{
    return wxBitmap(resourceLocator.GetBitmapFilePath(bitmapName).string(), wxBITMAP_TYPE_PNG);
}

wxBitmap WxHelpers::LoadBitmap(
    std::string const & bitmapName,
    ImageSize const & size,
    ResourceLocator const & resourceLocator)
{
    if (size.Width == 0 || size.Height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxImage image(resourceLocator.GetBitmapFilePath(bitmapName).string(), wxBITMAP_TYPE_PNG);
    image.Rescale(size.Width, size.Height, wxIMAGE_QUALITY_HIGH);
    return wxBitmap(image);
}

wxBitmap WxHelpers::MakeBitmap(RgbaImageData const & imageData)
{
    if (imageData.Size.Width == 0 || imageData.Size.Height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxBitmap bitmap;
    bitmap.Create(imageData.Size.Width, imageData.Size.Height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == imageData.Size.Width);
    assert(pixelData.GetHeight() == imageData.Size.Height);

    rgbaColor const * readIt = imageData.Data.get();
    auto writeIt = pixelData.GetPixels();
    writeIt.OffsetY(pixelData, imageData.Size.Height - 1);
    for (int y = 0; y < imageData.Size.Height; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        for (int x = 0; x < imageData.Size.Width; ++x, ++readIt, ++writeIt)
        {
            // We have to pre-multiply r, g, and b by alpha,
            // see https://forums.wxwidgets.org/viewtopic.php?f=1&t=46322,
            // because Windows and Mac require the first term to be pre-computed,
            // as it only takes care of the second
            writeIt.Red() = readIt->r * readIt->a / 256;
            writeIt.Green() = readIt->g * readIt->a / 256;
            writeIt.Blue() = readIt->b * readIt->a / 256;
            writeIt.Alpha() = readIt->a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    return bitmap;
}

wxBitmap WxHelpers::MakeMatteBitmap(
    rgbaColor const & color,
    ImageSize const & size)
{
    if (size.Width == 0 || size.Height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxBitmap bitmap;
    bitmap.Create(size.Width, size.Height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == size.Width);
    assert(pixelData.GetHeight() == size.Height);

    auto writeIt = pixelData.GetPixels();
    writeIt.OffsetY(pixelData, size.Height - 1);
    for (int y = 0; y < size.Height; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        for (int x = 0; x < size.Width; ++x, ++writeIt)
        {
            writeIt.Red() = color.r;
            writeIt.Green() = color.g;
            writeIt.Blue() = color.b;
            writeIt.Alpha() = color.a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    return bitmap;
}

wxBitmap WxHelpers::MakeEmptyBitmap()
{
    wxBitmap bitmap;

    bitmap.Create(1, 1, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    auto writeIt = pixelData.GetPixels();
    writeIt.Red() = 0xff;
    writeIt.Green() = 0xff;
    writeIt.Blue() = 0xff;
    writeIt.Alpha() = 0xff;

    return bitmap;
}

wxCursor WxHelpers::LoadCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLocator const & resourceLocator)
{
    wxImage img = LoadCursorImage(
        cursorName,
        hotspotX,
        hotspotY,
        resourceLocator);

    return wxCursor(img);
}

wxImage WxHelpers::LoadCursorImage(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY,
    ResourceLocator const & resourceLocator)
{
    auto filepath = resourceLocator.GetCursorFilePath(cursorName);
    auto bmp = std::make_unique<wxBitmap>(filepath.string(), wxBITMAP_TYPE_PNG);

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return img;
}

wxImage WxHelpers::RetintCursorImage(
    wxImage const & src,
    rgbColor newTint)
{
    size_t const size = src.GetHeight() * src.GetWidth();
    vec3f const tint = newTint.toVec3f();

    wxImage dst = src.Copy();

    // Data
    {
        rgbColor const * srcData = reinterpret_cast<rgbColor const *>(src.GetData());
        unsigned char * const dstDataPtr = reinterpret_cast<unsigned char *>(std::malloc(size * 3));
        assert(nullptr != dstDataPtr);
        rgbColor * dstData = reinterpret_cast<rgbColor *>(dstDataPtr);

        for (int y = 0; y < src.GetHeight(); ++y)
        {
            for (int x = 0; x < src.GetWidth(); ++x, ++srcData, ++dstData)
            {
                vec3f const v = srcData->toVec3f();
                float const lum = (v.x + v.y + v.z) / 3.0f;
                *dstData = rgbColor(tint * (1.0f - lum));
            }
        }

        dst.SetData(dstDataPtr);
    }

    // Alpha
    if (src.HasAlpha())
    {
        unsigned char * const dstAlphaPtr = reinterpret_cast<unsigned char *>(std::malloc(size));
        assert(nullptr != dstAlphaPtr);
        std::memcpy(dstAlphaPtr, src.GetAlpha(), size);
        dst.SetAlpha(dstAlphaPtr);
    }

    return dst;
}