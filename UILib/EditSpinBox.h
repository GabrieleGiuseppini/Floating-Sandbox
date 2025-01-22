/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-10-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextValidators.h"

#include <UILib/WxHelpers.h>

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinbutt.h>
#include <wx/textctrl.h>

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
        , mValue(currentValue)
        , mIsModified(false)
        , mOnValueChanged(std::move(onValueChanged))
    {
        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

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

            mTextCtrl->SetValue(WxHelpers::ValueToString(currentValue));

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

    bool IsModified() const
    {
        return mIsModified;
    }

    TValue GetValue() const
    {
        return mValue;
    }

    // Clears modified flag
    void SetValue(float value)
    {
        mValue = value;
        mIsModified = false;

        mSpinButton->SetValue(mValue);
        mTextCtrl->SetValue(WxHelpers::ValueToString(mValue));
    }

    // Marks as modified
    void ChangeValue(float value)
    {
        SetValue(value);

        mIsModified = true;
    }

private:

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
        TValue value;
        if (WxHelpers::StringToValue(mTextCtrl->GetValue(), &value))
        {
            // Clamp to range
            value = std::max(value, mMinValue);
            value = std::min(value, mMaxValue);

            // Store value
            mValue = value;
            mIsModified = true;

            // Set text ctrl back to value
            mTextCtrl->SetValue(WxHelpers::ValueToString(value));

            // Set SpinButton to value
            mSpinButton->SetValue(value);

            // Notify value
            mOnValueChanged(value);
        }
    }

    void OnSpinButton(wxSpinEvent & event)
    {
        mValue = static_cast<TValue>(event.GetValue());
        mIsModified = true;

        mTextCtrl->SetValue(WxHelpers::ValueToString(mValue));

        // Notify value
        mOnValueChanged(mValue);
    }

private:

    TValue const mMinValue;
    TValue const mMaxValue;

    TValue mValue;
    bool mIsModified;

    wxTextCtrl * mTextCtrl;
    std::unique_ptr<wxValidator> mTextCtrlValidator;
    wxSpinButton * mSpinButton;

    std::function<void(TValue)> const mOnValueChanged;
};
