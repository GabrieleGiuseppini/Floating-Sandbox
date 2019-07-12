/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ISliderCore.h>

#include <wx/bitmap.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tooltip.h>
#include <wx/wx.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>

/*
 * This control incorporates a slider and a textbox that shows the
 * current mapped float value of the slider.
 *
 * The control takes a core that provides the logic that maps
 * slider positions to float vaues.
 */
template <typename TValue>
class SliderControl : public wxPanel
{
public:

    SliderControl(
        wxWindow * parent,
        int width,
        int height,
        std::string const & label,
        std::string const & toolTipLabel,
        TValue currentValue,
        std::function<void(TValue)> onValueChanged,
        std::unique_ptr<ISliderCore<TValue>> sliderCore)
        : SliderControl(
            parent,
            width,
            height,
            label,
            toolTipLabel,
            currentValue,
            onValueChanged,
            std::move(sliderCore),
            nullptr)
    {
    }

    SliderControl(
        wxWindow * parent,
        int width,
        int height,
        std::string const & label,
        std::string const & toolTipLabel,
        TValue currentValue,
        std::function<void(TValue)> onValueChanged,
        std::unique_ptr<ISliderCore<TValue>> sliderCore,
        wxBitmap const * warningIcon)
        : wxPanel(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxSize(width, height),
            wxBORDER_NONE)
        , mOnValueChanged(std::move(onValueChanged))
        , mSliderCore(std::move(sliderCore))
    {
        if (!toolTipLabel.empty())
            SetToolTip(toolTipLabel);

        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        //
        // Slider
        //

        mSlider = std::make_unique<wxSlider>(
            this,
            wxNewId(),
            mSliderCore->ValueToTick(currentValue),
            0,
            mSliderCore->GetNumberOfTicks(),
            wxDefaultPosition,
            wxSize(-1, height),
            wxSL_VERTICAL | wxSL_LEFT | wxSL_INVERSE | wxSL_AUTOTICKS, wxDefaultValidator);

        mSlider->SetTickFreq(4);

        // Removed as it was getting in the way when moving the slider
        //
        //if (!toolTipLabel.empty())
        //    mSlider->SetToolTip(toolTipLabel);
        //

        Connect(mSlider->GetId(), wxEVT_SLIDER, (wxObjectEventFunction)&SliderControl::OnSliderScroll);

        sizer->Add(mSlider.get(), 1, wxALIGN_CENTER_HORIZONTAL);


        //
        // Label
        //

        wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
        if (!toolTipLabel.empty())
            labelStaticText->SetToolTip(toolTipLabel);

        if (nullptr == warningIcon)
        {
            // Just add label
            sizer->Add(labelStaticText, 0, wxALIGN_CENTER_HORIZONTAL);
        }
        else
        {
            // Create icon
            wxStaticBitmap * icon = new wxStaticBitmap(this, wxID_ANY, *warningIcon, wxDefaultPosition, wxSize(-1, -1), wxBORDER_NONE);
            //icon->SetScaleMode(wxStaticBitmap::Scale_AspectFill);
            if (!toolTipLabel.empty())
                icon->SetToolTip(toolTipLabel);

            // Add label and icon
            wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);
            hSizer->Add(labelStaticText, 0, wxALIGN_CENTRE_VERTICAL);
            hSizer->AddSpacer(2);
            hSizer->Add(icon, 0, wxALIGN_CENTRE_VERTICAL);

            sizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL);
        }


        //
        // Text control
        //

        mTextCtrl = std::make_unique<wxTextCtrl>(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_CENTRE);
        mTextCtrl->SetValue(std::to_string(currentValue));
        if (!toolTipLabel.empty())
            mTextCtrl->SetToolTip(toolTipLabel);

        sizer->Add(mTextCtrl.get(), 0, wxALIGN_CENTER_HORIZONTAL);

        this->SetSizerAndFit(sizer);
    }

    TValue GetValue() const
    {
        return mSliderCore->TickToValue(mSlider->GetValue());
    }

    void SetValue(TValue value)
    {
        mSlider->SetValue(mSliderCore->ValueToTick(value));
        mTextCtrl->SetValue(std::to_string(value));
    }

private:

    void OnSliderScroll(wxScrollEvent & /*event*/)
    {
        TValue value = mSliderCore->TickToValue(mSlider->GetValue());

        mTextCtrl->SetValue(std::to_string(value));

        // Notify value
        mOnValueChanged(value);
    }

private:

    std::unique_ptr<wxSlider> mSlider;
    std::unique_ptr<wxTextCtrl> mTextCtrl;

    std::function<void(TValue)> const mOnValueChanged;
    std::unique_ptr<ISliderCore<TValue>> const mSliderCore;
};
