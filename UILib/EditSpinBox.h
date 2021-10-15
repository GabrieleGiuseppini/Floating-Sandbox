/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-10-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextValidators.h"

#include <GameCore/Utils.h>

#include <wx/spinbutt.h>
#include <wx/textctrl.h>

#include <sstream>
#include <string>

template <typename TValue>
class EditSpinBox : public wxPanel
{
public:

    EditSpinBox(
        wxWindow * parent,
        int width,
        TValue minValue,
        TValue maxValue,
        TValue currentValue,
        wxString const & toolTipLabel,
        std::function<void(TValue)> onValueChanged)
        : wxPanel(parent, wxID_ANY)
        , mMinValue(minValue)
        , mMaxValue(maxValue)
        , mOnValueChanged(std::move(onValueChanged))
    {
        wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);

        // Text control
        {
            mTextCtrlValidator = TextValidatorFactory::CreateInstance<TValue>(
                minValue,
                maxValue);

            mTextCtrl = new wxTextCtrl(
                this,
                wxID_ANY,
                wxEmptyString,
                wxDefaultPosition,
                wxSize(width, -1),
                wxTE_CENTRE | wxTE_PROCESS_ENTER,
                *mTextCtrlValidator);

            mTextCtrl->SetValue(ValueToString(currentValue));

            // TODOTEST
            //mTextCtrl->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

            mTextCtrl->Bind(wxEVT_KILL_FOCUS, &EditSpinBox::OnKillFocus, this);
            mTextCtrl->Bind(wxEVT_TEXT_ENTER, &EditSpinBox::OnTextEnter, this);

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
                wxSize(-1, 24),
                wxSP_VERTICAL | wxSP_ARROW_KEYS);

            mSpinButton->SetRange(minValue, maxValue);
            mSpinButton->SetValue(currentValue);

            mSpinButton->Bind(wxEVT_SPIN, &EditSpinBox::OnSpinButton, this);

            hSizer->Add(mSpinButton, 0, wxALIGN_CENTRE_VERTICAL);
        }

        this->SetSizerAndFit(hSizer);
    }

private:

    std::string ValueToString(TValue value) const
    {
        return std::to_string(value);
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
            value = std::max(value, mMinValue);
            value = std::min(value, mMaxValue);

            // Set text ctrl back to value
            mTextCtrl->SetValue(ValueToString(value));

            // Set SpinButton to value
            mSpinButton->SetValue(value);

            // Notify value
            mOnValueChanged(value);
        }
    }

    void OnSpinButton(wxSpinEvent & event)
    {
        auto const value = event.GetValue();

        mTextCtrl->SetValue(ValueToString(value));

        // Notify value
        mOnValueChanged(value);
    }

private:

    TValue const mMinValue;
    TValue const mMaxValue;

    wxTextCtrl * mTextCtrl;
    std::unique_ptr<wxValidator> mTextCtrlValidator;
    wxSpinButton * mSpinButton;

    std::function<void(TValue)> const mOnValueChanged;
};
