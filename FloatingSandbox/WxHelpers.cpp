/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WxHelpers.h"

#include <wx/rawbmp.h>

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
            writeIt.Red() = readIt->r;
            writeIt.Green() = readIt->g;
            writeIt.Blue() = readIt->b;
            writeIt.Alpha() = readIt->a;
        }

        // Move write iterator to next row
        writeIt = rowStart;
        writeIt.OffsetY(pixelData, -1);
    }

    return bitmap;
}