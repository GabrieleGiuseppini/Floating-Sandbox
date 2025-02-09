/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-06-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <Core/Colors.h>
#include <Core/ImageData.h>

#include <wx/bitmap.h>
#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/image.h>
#include <wx/numformatter.h>
#include <wx/sizer.h>
#include <wx/wx.h>

#include <filesystem>
#include <string>
#include <type_traits>

namespace WxHelpers
{
    wxBitmap LoadBitmap(
        std::string const & bitmapName,
        GameAssetManager const & gameAssetManager);

    wxBitmap LoadBitmap(std::filesystem::path const & bitmapFilePath);

    wxBitmap LoadBitmap(
        std::string const & bitmapName,
        ImageSize const & size,
        GameAssetManager const & gameAssetManager);

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
        GameAssetManager const & gameAssetManager);

    wxImage LoadCursorImage(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY,
        GameAssetManager const & gameAssetManager);

    wxImage RetintCursorImage(
        wxImage const & src,
        rgbColor newTint);

    wxImage MakeImage(RgbaImageData const & imageData);

    void MakeAllColumnsExpandable(wxFlexGridSizer * gridSizer);

    //////////////////////////////////////////////////////////////////////////
    // String <-> Numbers with UI- and Locale-specific formats
    //////////////////////////////////////////////////////////////////////////

    template<typename TValue,
        typename std::enable_if_t<
        !std::is_integral<TValue>::value
        && !std::is_floating_point<TValue>::value> * = nullptr>
    static bool StringToValue(wxString const & strValue, TValue * value);

    template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
    static bool StringToValue(wxString const & strValue, TValue * value)
    {
        double _value;
        if (wxNumberFormatter::FromString(strValue, &_value))
        {
            *value = static_cast<TValue>(_value);
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename TValue, typename std::enable_if_t<std::is_integral<TValue>::value> * = nullptr>
    static bool StringToValue(wxString const & strValue, TValue * value)
    {
        long long _value;
        if (wxNumberFormatter::FromString(strValue, &_value))
        {
            *value = static_cast<TValue>(_value);
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename TValue,
        typename std::enable_if_t<
        !std::is_integral<TValue>::value
        && !std::is_floating_point<TValue>::value> * = nullptr>
    static wxString ValueToString(TValue const & value);

    template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
    static wxString ValueToString(TValue const & value)
    {
        return wxNumberFormatter::ToString(static_cast<double>(value), 3);
    }

    template<typename TValue, typename std::enable_if_t<std::is_integral<TValue>::value> * = nullptr>
    static wxString ValueToString(TValue const & value)
    {
        return wxNumberFormatter::ToString(static_cast<long long>(value));
    }
};
