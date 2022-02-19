/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-02-10
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <wx/panel.h>
#include <wx/ribbon/art.h>
#include <wx/ribbon/panel.h>
#include <wx/stattext.h>

#include <filesystem>
#include <functional>

namespace ShipBuilder {

template<typename TButton>
class RibbonToolbarButton : public wxPanel
{
public:

    RibbonToolbarButton(
        wxRibbonPanel * parent,
        int direction,
        std::filesystem::path const & bitmapFilePath,
        wxString const & label,
        std::function<void()> onClickHandler,
        wxString const & toolTipLabel = "")
        : wxPanel(parent)
        , mLabelEnabledColor(parent->GetArtProvider()->GetColor(wxRIBBON_ART_BUTTON_BAR_LABEL_COLOUR))
        , mLabelDisabledColor(parent->GetArtProvider()->GetColor(wxRIBBON_ART_BUTTON_BAR_LABEL_DISABLED_COLOUR))
    {
        wxSizer * sizer = new wxBoxSizer(direction);

        // Button
        {
            mButton = new TButton(
                this,
                bitmapFilePath,
                onClickHandler,
                toolTipLabel);

            sizer->Add(
                mButton,
                0,
                direction == wxVERTICAL ? wxALIGN_CENTER_HORIZONTAL : wxALIGN_CENTER_VERTICAL,
                0);
        }

        // Label
        {
            mLabel = new wxStaticText(
                this, 
                wxID_ANY, 
                label);

            // We begin enabled
            mLabel->SetForegroundColour(mLabelEnabledColor);

            sizer->Add(
                mLabel,
                0,
                direction == wxVERTICAL ? (wxALIGN_CENTER_HORIZONTAL | wxTOP) : (wxALIGN_CENTER_VERTICAL | wxLEFT),
                2);
        }

        this->SetSizerAndFit(sizer);
    }

    bool GetValue()
    {
        return mButton->GetValue();
    }

    void SetValue(bool value)
    {
        mButton->SetValue(value);
    }

private:

    TButton * mButton;
    wxStaticText * mLabel;

    wxColor const mLabelEnabledColor;
    wxColor const mLabelDisabledColor;
};

}