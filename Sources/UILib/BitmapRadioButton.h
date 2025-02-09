/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-02-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <UILib/WxHelpers.h>

#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/tglbtn.h>

#include <filesystem>
#include <functional>

/*
 *
Once pressed, may only be set to false via SetValue().
 *
 */
class BitmapRadioButton : public wxToggleButton
{
public:

    BitmapRadioButton(
        wxWindow * parent,
        std::filesystem::path const & bitmapFilePath,
        std::function<void()> onClickHandler,
        wxString const & toolTipLabel = "")
        : wxToggleButton(
            parent,
            wxID_ANY,
            wxEmptyString,
            wxDefaultPosition,
            wxDefaultSize,
            wxBU_EXACTFIT)
    {
        SetBitmapLabel(WxHelpers::MakeBaseButtonBitmap(bitmapFilePath));
        SetBitmapPressed(WxHelpers::MakeSelectedButtonBitmap(bitmapFilePath));

        if (!toolTipLabel.empty())
        {
            SetToolTip(toolTipLabel);
        }

        Bind(
            wxEVT_TOGGLEBUTTON,
            [onClickHandler, this](wxCommandEvent & /*event*/)
            {
                if (!this->GetValue())
                    SetValue(true);

                onClickHandler();
            });
    }
};
