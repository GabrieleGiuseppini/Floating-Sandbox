/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WxHelpers.h"

#include "Style.h"

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
    if (size.width == 0 || size.height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxImage image(resourceLocator.GetBitmapFilePath(bitmapName).string(), wxBITMAP_TYPE_PNG);
    image.Rescale(size.width, size.height, wxIMAGE_QUALITY_HIGH);
    return wxBitmap(image);
}

wxBitmap WxHelpers::MakeBitmap(RgbaImageData const & imageData)
{
    if (imageData.Size.width == 0 || imageData.Size.height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxBitmap bitmap;
    bitmap.Create(imageData.Size.width, imageData.Size.height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == imageData.Size.width);
    assert(pixelData.GetHeight() == imageData.Size.height);

    rgbaColor const * readIt = imageData.Data.get();
    auto writeIt = pixelData.GetPixels();
    writeIt.OffsetY(pixelData, imageData.Size.height - 1);
    for (int y = 0; y < imageData.Size.height; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        for (int x = 0; x < imageData.Size.width; ++x, ++readIt, ++writeIt)
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

wxBitmap WxHelpers::MakeSelectedButtonBitmap(wxBitmap const & baseBitmap)
{
    rgbaColor const borderColor = rgbaColor(ButtonSelectedBorderColor, 255);
    rgbaColor const bgColor = rgbaColor(ButtonSelectedBgColor, 255);

    auto const width = baseBitmap.GetWidth();
    auto const height = baseBitmap.GetHeight();

    wxBitmap bitmap = baseBitmap.GetSubBitmap(wxRect(0, 0, width, height));

    wxPixelData<wxBitmap, wxAlphaPixelFormat> const pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == width);
    assert(pixelData.GetHeight() == height);

    //
    // Add border, and make interior light blue-ish
    //

    auto readIt = pixelData.GetPixels();
    auto writeIt = pixelData.GetPixels();

    readIt.OffsetY(pixelData, height - 1);
    writeIt.OffsetY(pixelData, height - 1);

    // Border bottom
    {
        auto wRowStart = writeIt;

        for (int x = 0; x < width; ++x, ++writeIt)
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;
        }

        // Move read iterator to next row
        readIt.OffsetY(pixelData, -1);

        // Move write iterator to next row
        writeIt = wRowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    for (int y = 1; y < height - 1; ++y)
    {
        // Save current iterators
        auto rRowStart = readIt;
        auto wRowStart = writeIt;        

        // Border left
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++readIt;
            ++writeIt;
        }

        // Interior
        for (int x = 1; x < width - 1; ++x)
        {
            auto const color = bgColor.blend(
                rgbaColor(
                    readIt.Red(),
                    readIt.Green(),
                    readIt.Blue(),
                    readIt.Alpha()));

            writeIt.Red() = color.r;
            writeIt.Green() = color.g;
            writeIt.Blue() = color.b;
            writeIt.Alpha() = color.a;

            ++readIt;
            ++writeIt;
        }

        // Border right
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++readIt;
            ++writeIt;
        }

        // Move read iterator to next row
        readIt = rRowStart;
        readIt.OffsetY(pixelData, -1);

        // Move write iterator to next row
        writeIt = wRowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    // Border top
    {
        for (int x = 0; x < width; ++x)
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++writeIt;
        }
    }

    return bitmap;
}

wxBitmap WxHelpers::MakeMatteBitmap(
    rgbaColor const & color,
    ImageSize const & size)
{
    return WxHelpers::MakeMatteBitmap(color, color, size);
}

wxBitmap WxHelpers::MakeMatteBitmap(
    rgbaColor const & color,
    rgbaColor const & borderColor,
    ImageSize const & size)
{
    if (size.width == 0 || size.height == 0)
    {
        throw std::runtime_error("Cannot create bitmap with one zero dimension");
    }

    wxBitmap bitmap;
    bitmap.Create(size.width, size.height, 32);

    wxPixelData<wxBitmap, wxAlphaPixelFormat> pixelData(bitmap);
    if (!pixelData)
    {
        throw std::runtime_error("Cannot get bitmap pixel data");
    }

    assert(pixelData.GetWidth() == size.width);
    assert(pixelData.GetHeight() == size.height);

    auto writeIt = pixelData.GetPixels();

    writeIt.OffsetY(pixelData, size.height - 1);

    // Border bottom
    {
        auto rowStart = writeIt;

        for (int x = 0; x < size.width; ++x, ++writeIt)
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    for (int y = 1; y < size.height - 1; ++y)
    {
        // Save current iterator
        auto rowStart = writeIt;

        // Border left
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++writeIt;
        }

        // Interior
        for (int x = 1; x < size.width - 1; ++x)
        {
            writeIt.Red() = color.r;
            writeIt.Green() = color.g;
            writeIt.Blue() = color.b;
            writeIt.Alpha() = color.a;

            ++writeIt;
        }

        // Border right
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++writeIt;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    // Border top
    {
        for (int x = 0; x < size.width; ++x)
        {
            writeIt.Red() = borderColor.r;
            writeIt.Green() = borderColor.g;
            writeIt.Blue() = borderColor.b;
            writeIt.Alpha() = borderColor.a;

            ++writeIt;
        }
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

wxImage WxHelpers::MakeImage(RgbaImageData const & imageData)
{
    size_t const linearSize = imageData.Size.GetLinearSize();

    unsigned char * const dstDataPtr = reinterpret_cast<unsigned char *>(std::malloc(linearSize * 3));
    assert(nullptr != dstDataPtr);
    rgbColor * dstData = reinterpret_cast<rgbColor *>(dstDataPtr);

    unsigned char * const dstAlphaPtr = reinterpret_cast<unsigned char *>(std::malloc(linearSize * 1));
    assert(nullptr != dstAlphaPtr);
    rgbColor::data_type * dstAlpha = reinterpret_cast<rgbColor::data_type *>(dstAlphaPtr);
    
    for (int y = imageData.Size.height - 1; y >= 0; --y)
    {
        for (int x = 0; x < imageData.Size.width; ++x)
        {
            rgbaColor const srcSample = imageData[{x, y}];

            *(dstData++) = srcSample.toRgbColor();
            *(dstAlpha++) = srcSample.a;
        }
    }

    wxImage dst = wxImage(imageData.Size.width, imageData.Size.height, dstDataPtr, false);
    dst.SetAlpha(dstAlphaPtr, false);

    return dst;
}