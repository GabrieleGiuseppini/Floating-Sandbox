/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageData.h>

#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/wx.h>

#include <filesystem>
#include <memory>

class WxHelpers
{
public:

    static wxBitmap MakeBitmap(RgbaImageData const & imageData);

    static wxBitmap MakeEmptyBitmap();

    static std::unique_ptr<wxCursor> MakeCursor(
        std::filesystem::path cursorFilepath,
        int hotspotX,
        int hotspotY);
};
