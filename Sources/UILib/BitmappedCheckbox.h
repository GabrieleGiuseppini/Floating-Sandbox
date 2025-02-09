/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-01-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/wx.h>

#include <cassert>
#include <string>

class BitmappedCheckbox : public wxPanel
{
public:

    BitmappedCheckbox(
        wxWindow * parent,
        wxWindowID id,
        wxBitmap const & uncheckedBitmap,
        wxBitmap const & checkedBitmap,
        std::string const & toolTipLabel)
        : wxPanel(
            parent,
            id,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE)
        , mUncheckedBitmap(uncheckedBitmap)
        , mCheckedBitmap(checkedBitmap)
        , mIsChecked(false)
    {
        mStaticBitmap = new wxStaticBitmap(this, wxID_ANY, mUncheckedBitmap, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        if (!toolTipLabel.empty())
            mStaticBitmap->SetToolTip(toolTipLabel);

        mStaticBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&BitmappedCheckbox::OnLeftDown, this);
    }

    bool IsChecked() const
    {
        return mIsChecked;
    }

    void SetChecked(bool isChecked)
    {
        mIsChecked = isChecked;
        SelectBitmapForCurrentState();
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        // Toggle state
        mIsChecked = !mIsChecked;

        // Refresh UI
        SelectBitmapForCurrentState();

        // Fire event
        auto event = wxCommandEvent(wxEVT_CHECKBOX, GetId());
        event.SetInt(mIsChecked ? 1 : 0);
        ProcessEvent(event);
    }

    void SelectBitmapForCurrentState()
    {
        if (mIsChecked)
            mStaticBitmap->SetBitmap(mCheckedBitmap);
        else
            mStaticBitmap->SetBitmap(mUncheckedBitmap);

        Refresh();
    }

private:

    wxStaticBitmap * mStaticBitmap;

    wxBitmap const mUncheckedBitmap;
    wxBitmap const mCheckedBitmap;

    bool mIsChecked;
};
