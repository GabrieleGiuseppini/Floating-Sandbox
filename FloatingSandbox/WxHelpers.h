/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLoader.h>

#include <GameCore/ImageData.h>

#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/image.h>
#include <wx/wx.h>

#include <filesystem>
#include <memory>

namespace WxHelpers
{
    wxBitmap MakeBitmap(RgbaImageData const & imageData);

    wxBitmap MakeEmptyBitmap();

    wxCursor LoadCursor(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLoader & resourceLoader);

    wxImage LoadCursorImage(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLoader & resourceLoader);
};
