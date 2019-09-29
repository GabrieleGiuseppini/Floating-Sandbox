/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ISliderCore.h>
#include <GameCore/Log.h>
#include <GameCore/Utils.h>

#include <wx/bitmap.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tooltip.h>
#include <wx/valnum.h>
#include <wx/wx.h>

#include <cassert>
#include <functional>
#include <limits>
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

        mSlider->Bind(wxEVT_SLIDER, (wxObjectEventFunction)&SliderControl::OnSliderScroll, this);

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

        CreateTextCtrlValidator(
            mSliderCore->GetMinValue(), 
            mSliderCore->GetMaxValue());

        mTextCtrl = std::make_unique<wxTextCtrl>(
            this, 
            wxID_ANY, 
            _(""), 
            wxDefaultPosition, 
            wxDefaultSize, 
            wxTE_CENTRE | wxTE_PROCESS_ENTER,
            *mTextCtrlValidator);

        mTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        mTextCtrl->SetValue(std::to_string(currentValue));

        mTextCtrl->Bind(wxEVT_TEXT_ENTER, &SliderControl::OnTextEnter, this);

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

    void OnTextEnter(wxCommandEvent & /*event*/)
    {
        std::string const strValue = mTextCtrl->GetValue().ToStdString();

        TValue value;
        if (Utils::LexicalCast<TValue>(strValue, &value))
        {
            // Clamp to range
            value = std::max(value, mSliderCore->GetMinValue());
            value = std::min(value, mSliderCore->GetMaxValue());

            // Set slider to value
            mSlider->SetValue(mSliderCore->ValueToTick(value));

            // Set text ctrl back to value
            mTextCtrl->SetValue(std::to_string(value));

            // Notify value
            mOnValueChanged(value);
        }
    }

private:

    void CreateTextCtrlValidator(TValue const & minValue, TValue const & maxValue);

private:

    std::unique_ptr<wxSlider> mSlider;
    std::unique_ptr<wxTextCtrl> mTextCtrl;
    std::unique_ptr<wxValidator> mTextCtrlValidator;

    std::function<void(TValue)> const mOnValueChanged;
    std::unique_ptr<ISliderCore<TValue>> const mSliderCore;
};

template<>
inline void SliderControl<float>::CreateTextCtrlValidator(float const & minValue, float const & /*maxValue*/)
{
    auto validator = std::make_unique<wxFloatingPointValidator<float>>();

    float minRange;
    if (minValue >= 0.0f)
        minRange = 0.0f;
    else
        minRange = std::numeric_limits<float>::lowest();

    validator->SetRange(minRange, std::numeric_limits<float>::max());

    mTextCtrlValidator = std::move(validator);
}

template<>
inline void SliderControl<unsigned int>::CreateTextCtrlValidator(unsigned int const & minValue, unsigned int const & /*maxValue*/)
{
    auto validator = std::make_unique<wxIntegerValidator<unsigned int>>();
    
    unsigned int minRange;
    if (minValue >= 0)
        minRange = 0;
    else
        minRange = std::numeric_limits<unsigned int>::lowest();

    validator->SetRange(minRange, std::numeric_limits<unsigned int>::max());

    mTextCtrlValidator = std::move(validator);
}