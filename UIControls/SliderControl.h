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
#include <string>
#include <type_traits>

struct TextValidatorFactory
{
	template<typename TValue,
		typename std::enable_if_t<
		!std::is_integral<TValue>::value
		&& !std::is_floating_point<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & maxValue);

	template<typename TValue, typename std::enable_if_t<std::is_floating_point<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & /*maxValue*/)
	{
		auto validator = std::make_unique<wxFloatingPointValidator<TValue>>();

		float minRange;
		if (minValue >= 0.0f)
			minRange = 0.0f;
		else
			minRange = std::numeric_limits<TValue>::lowest();

		validator->SetRange(minRange, std::numeric_limits<TValue>::max());

		return validator;
	}

	template<typename TValue, typename std::enable_if_t<std::is_integral<TValue>::value> * = nullptr>
	static std::unique_ptr<wxValidator> CreateInstance(TValue const & minValue, TValue const & /*maxValue*/)
	{
		auto validator = std::make_unique<wxIntegerValidator<TValue>>();

		TValue minRange;
		if (minValue >= 0)
			minRange = 0;
		else
			minRange = std::numeric_limits<TValue>::lowest();

		validator->SetRange(minRange, std::numeric_limits<TValue>::max());

		return validator;
	}
};

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
        wxString const & label,
        wxString const & toolTipLabel,
        std::function<void(TValue)> onValueChanged,
        std::unique_ptr<ISliderCore<TValue>> sliderCore)
        : SliderControl(
            parent,
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

        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        //
        // Slider
        //

        {
            mSlider = new wxSlider(
                this,
                wxNewId(),
                0, // Start value
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

            // Make the slider expand
            vSizer->Add(mSlider, 1, wxALIGN_CENTER_HORIZONTAL);
        }


        //
        // Label
        //

        {
            wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);

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

                mSpinButton->SetRange(0, mSliderCore->GetNumberOfTicks());
                mSpinButton->SetValue(mSlider->GetValue());

                mSpinButton->Bind(wxEVT_SPIN, &SliderControl::OnSpinButton, this);

                hSizer->Add(mSpinButton, 0, wxALIGN_CENTRE_VERTICAL);
            }

            vSizer->Add(hSizer, 0, wxALIGN_CENTER_HORIZONTAL);
        }

        this->SetSizerAndFit(vSizer);
    }

    TValue GetValue() const
    {
        return mSliderCore->TickToValue(mSlider->GetValue());
    }

    void SetValue(TValue value)
    {
        auto const tickValue = mSliderCore->ValueToTick(value);

        mSlider->SetValue(tickValue);
        mTextCtrl->SetValue(std::to_string(value));
        mSpinButton->SetValue(tickValue);
    }

private:

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
            mTextCtrl->SetValue(std::to_string(value));

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

private:

    void SetTickValue(int tick)
    {
        TValue const value = mSliderCore->TickToValue(tick);

        mTextCtrl->SetValue(std::to_string(value));

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
