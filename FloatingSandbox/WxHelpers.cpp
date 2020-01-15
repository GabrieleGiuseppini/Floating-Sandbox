/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WxHelpers.h"

#include <GameCore/GameException.h>

#include <wx/rawbmp.h>

#include <stdexcept>

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

std::unique_ptr<wxCursor> WxHelpers::MakeCursor(
    std::filesystem::path cursorFilepath,
    int hotspotX,
    int hotspotY)
{
    wxBitmap* bmp = new wxBitmap(cursorFilepath.string(), wxBITMAP_TYPE_PNG);
    if (nullptr == bmp)
    {
        throw GameException("Cannot load cursor '" + cursorFilepath.string() + "'");
    }

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return std::make_unique<wxCursor>(img);
}