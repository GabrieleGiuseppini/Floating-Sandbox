/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/Colors.h>
#include <GameCore/ImageData.h>

#include <wx/bitmap.h>
#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/image.h>
#include <wx/sizer.h>
#include <wx/wx.h>

#include <filesystem>
#include <string>

namespace WxHelpers
{
    wxBitmap LoadBitmap(
        std::string const & bitmapName,
        ResourceLocator const & resourceLocator);

    wxBitmap LoadBitmap(std::filesystem::path const & bitmapFilePath);

    wxBitmap LoadBitmap(
        std::string const & bitmapName,
        ImageSize const & size,
        ResourceLocator const & resourceLocator);

    wxBitmap MakeBitmap(RgbaImageData const & imageData);

    wxBitmap MakeBaseButtonBitmap(std::filesystem::path const & bitmapFilePath);
    wxBitmap MakeSelectedButtonBitmap(std::filesystem::path const & bitmapFilePath);

    wxBitmap MakeMatteBitmap(
        rgbaColor const & color,
        ImageSize const & size);

    wxBitmap MakeMatteBitmap(
        rgbaColor const & color,
        rgbaColor const & borderColor,
        ImageSize const & size);

    wxBitmap MakeEmptyBitmap();

    wxCursor LoadCursor(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLocator const & resourceLocator);

    wxImage LoadCursorImage(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        ResourceLocator const & resourceLocator);

    wxImage RetintCursorImage(
        wxImage const & src,
        rgbColor newTint);

    wxImage MakeImage(RgbaImageData const & imageData);

    void MakeAllColumnsExpandable(wxFlexGridSizer * gridSizer);
};
