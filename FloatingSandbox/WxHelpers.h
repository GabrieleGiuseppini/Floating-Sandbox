/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/ImageData.h>

#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/image.h>
#include <wx/wx.h>

#include <string>

namespace WxHelpers
{
    wxBitmap MakeBitmap(RgbaImageData const & imageData);

    wxBitmap MakeEmptyBitmap();

    wxCursor LoadCursor(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLocator & resourceLocator);

    wxImage LoadCursorImage(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLocator & resourceLocator);
};
