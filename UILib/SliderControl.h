/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextValidators.h"

#include <GameCore/ISliderCore.h>
#include <GameCore/Utils.h>

#include <wx/bitmap.h>
#include <wx/slider.h>
#include <wx/spinbutt.h>
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
#include <sstream>
#include <string>
#include <type_traits>

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

    enum class DirectionType
    {
        Horizontal,
        Vertical
    };

    SliderControl(
        wxWindow * parent,
        DirectionType direction,
        int width,
        int height,
        wxString const & label,
        wxString const & toolTipLabel,
        std::function<void(TValue)> onValueChanged,
        std::unique_ptr<ISliderCore<TValue>> sliderCore)
        : SliderControl(
            parent,
            direction,
            width,
            height,
            label,
            toolTipLabel,
            onValueChanged,
            std::move(sliderCore),
            nullptr)
    {
    }

    SliderControl(
        wxWindow * parent,
        DirectionType direction,
        int width,
        int height,
        wxString const & label,
        wxString const & toolTipLabel,
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
        // Set font
        SetFont(parent->GetFont());

        // Set tooltop
        if (!toolTipLabel.IsEmpty())
            SetToolTip(toolTipLabel);

        // Calculate parameters
        int const n = mSliderCore->GetNumberOfTicks();
        int const wxMaxValue = std::max(n - 1, 1); // So we always give max > min to wxWidgets; but then we disable ourselves if n <= 1

        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        //
        // Slider
        //

        {
            mSlider = new wxSlider(
                this,
                wxNewId(),
                0, // Start value
                0, // Min value
                wxMaxValue, // Max value, included
                wxDefaultPosition,
                wxSize(-1, height),
                (direction == DirectionType::Vertical ? (wxSL_VERTICAL | wxSL_LEFT | wxSL_INVERSE) : (wxSL_HORIZONTAL)) | wxSL_AUTOTICKS,
                wxDefaultValidator);

            mSlider->SetTickFreq(height >= n * 4 ? 1 : std::max(4, static_cast<int>(std::ceil(static_cast<float>(n) / static_cast<float>(height)))));

            mSlider->Bind(wxEVT_SLIDER, (wxObjectEventFunction)&SliderControl::OnSliderScroll, this);

            if (direction == DirectionType::Vertical)
            {
                // Make the slider expand vertically
                vSizer->Add(mSlider, 1, wxALIGN_CENTER_HORIZONTAL);
            }
            else
            {
                // Use required vertical height, expand horizontally
                vSizer->Add(mSlider, 0, wxEXPAND);
            }
        }

        //
        // Label
        //

        {
            wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

            if (!toolTipLabel.IsEmpty())
                labelStaticText->SetToolTip(toolTipLabel);

            if (nullptr == warningIcon)
            {
                // Just add label
                vSizer->Add(labelStaticText, 0, wxALIGN_CENTER_HORIZONTAL);
            }
            else
            {
                // Add label and icon

                wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);

                // Label
                {
                    hSizer->Add(labelStaticText, 0, wxALIGN_CENTRE_VERTICAL);
                }

                hSizer->AddSpacer(2);

                // Icon
                {
                    wxStaticBitmap * icon = new wxStaticBitmap(this, wxID_ANY, *warningIcon, wxDefaultPosition, wxSize(-1, -1), wxBORDER_NONE);

                    if (!toolTipLabel.IsEmpty())
                        icon->SetToolTip(toolTipLabel);

                    hSizer->Add(icon, 0, wxALIGN_CENTRE_VERTICAL);
                }

                vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL);
            }
        }

        //
        // Text control and spin button
        //

        {
            wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);

            // Text control
            {
                mTextCtrlValidator = TextValidatorFactory::CreateInstance<TValue>(
                    mSliderCore->GetMinValue(),
                    mSliderCore->GetMaxValue());

                mTextCtrl = new wxTextCtrl(
                    this,
                    wxID_ANY,
                    wxEmptyString,
                    wxDefaultPosition,
                    wxSize(width, -1),
                    wxTE_CENTRE | wxTE_PROCESS_ENTER,
                    *mTextCtrlValidator);

                mTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

                mTextCtrl->Bind(wxEVT_KILL_FOCUS, &SliderControl::OnKillFocus, this);
                mTextCtrl->Bind(wxEVT_TEXT_ENTER, &SliderControl::OnTextEnter, this);

                if (!toolTipLabel.IsEmpty())
                    mTextCtrl->SetToolTip(toolTipLabel);

                hSizer->Add(mTextCtrl, 0, wxALIGN_CENTER_VERTICAL);
            }

            // Spin button
            {
                mSpinButton = new wxSpinButton(
                    this,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxSize(-1, 22),
                    wxSP_VERTICAL | wxSP_ARROW_KEYS);

                mSpinButton->SetRange(0, wxMaxValue);
                mSpinButton->SetValue(mSlider->GetValue());

                mSpinButton->Bind(wxEVT_SPIN, &SliderControl::OnSpinButton, this);

                hSizer->Add(mSpinButton, 0, wxALIGN_CENTRE_VERTICAL);
            }

            vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL);
        }

        this->SetSizerAndFit(vSizer);

        //
        // Disable self if no degrees of freedom
        //

        if (n <= 1)
        {
            this->Enable(false);
        }
    }

    TValue GetValue() const
    {
        return mSliderCore->TickToValue(mSlider->GetValue());
    }

    void SetValue(TValue value)
    {
        auto const tickValue = mSliderCore->ValueToTick(value);

        mSlider->SetValue(tickValue);
        mTextCtrl->SetValue(ValueToString(value));
        mSpinButton->SetValue(tickValue);
    }

private:

    template<typename _TValue>
    std::string ValueToString(_TValue value)
    {
        return std::to_string(value);
    }

    std::string ValueToString(float value)
    {
        std::stringstream ss;
        ss << std::fixed;
        ss.precision(3);
        ss << value;
        return ss.str();
    }

    void OnSliderScroll(wxScrollEvent & /*event*/)
    {
        auto const tickValue = mSlider->GetValue();

        SetTickValue(tickValue);
        mSpinButton->SetValue(tickValue);
    }

    void OnKillFocus(wxFocusEvent & event)
    {
        OnTextUpdated();

        event.Skip();
    }

    void OnTextEnter(wxCommandEvent & /*event*/)
    {
        OnTextUpdated();
    }

    void OnTextUpdated()
    {
        std::string const strValue = mTextCtrl->GetValue().ToStdString();

        TValue value;
        if (Utils::LexicalCast<TValue>(strValue, &value))
        {
            // Clamp to range
            value = std::max(value, mSliderCore->GetMinValue());
            value = std::min(value, mSliderCore->GetMaxValue());

            auto const tickValue = mSliderCore->ValueToTick(value);

            // Set slider to value
            mSlider->SetValue(tickValue);

            // Set text ctrl back to value
            mTextCtrl->SetValue(ValueToString(value));

            // Set SpinButton to value
            mSpinButton->SetValue(tickValue);

            // Notify value
            mOnValueChanged(value);
        }
    }

    void OnSpinButton(wxSpinEvent & event)
    {
        auto const tickValue = event.GetValue();

        SetTickValue(tickValue);
        mSlider->SetValue(tickValue);
    }

    void SetTickValue(int tick)
    {
        TValue const value = mSliderCore->TickToValue(tick);

        mTextCtrl->SetValue(ValueToString(value));

        // Notify value
        mOnValueChanged(value);
    }

private:

    wxSlider * mSlider;
    wxTextCtrl * mTextCtrl;
    std::unique_ptr<wxValidator> mTextCtrlValidator;
    wxSpinButton * mSpinButton;

    std::function<void(TValue)> const mOnValueChanged;
    std::unique_ptr<ISliderCore<TValue>> const mSliderCore;
};
