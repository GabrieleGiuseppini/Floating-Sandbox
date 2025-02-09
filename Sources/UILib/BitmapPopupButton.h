/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/image.h>

#include <filesystem>
#include <functional>

class BitmapButton : public wxButton
{
public:

    BitmapButton(
        wxWindow * parent,
        std::filesystem::path const & bitmapFilePath,
        std::function<void()> onClickHandler,
        wxString const & toolTipLabel)
        : wxButton(
            parent,
            wxID_ANY,
            wxEmptyString,
            wxDefaultPosition,
            wxDefaultSize)
    {
        auto img = wxImage(bitmapFilePath.string(), wxBITMAP_TYPE_PNG);
        SetBitmap(wxBitmap(img));

        if (!toolTipLabel.empty())
            SetToolTip(toolTipLabel);

        Bind(
            wxEVT_BUTTON,
            [onClickHandler](wxCommandEvent & /*event*/)
            {
                onClickHandler();
            });
    }
};
