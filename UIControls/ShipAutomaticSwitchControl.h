/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>

#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/wx.h>

#include <cassert>
#include <memory>
#include <optional>
#include <string>

class ShipAutomaticSwitchControl : public wxPanel
{
public:

    ShipAutomaticSwitchControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        SwitchId switchId,
        std::string const & label,
        SwitchState currentState)
        : wxPanel(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE)
        , mOnEnabledImage(onEnabledImage)
        , mOffEnabledImage(offEnabledImage)
        , mOnDisabledImage(onDisabledImage)
        , mOffDisabledImage(offDisabledImage)
        , mImageBitmap(nullptr)
        , mSwitchId(switchId)
        , mCurrentState(currentState)
        , mIsEnabled(true)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImageBitmap = new wxStaticBitmap(this, wxID_ANY, mOnEnabledImage, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
        vSizer->Add(labelStaticText, 1, wxALIGN_CENTRE_HORIZONTAL);

        this->SetSizerAndFit(vSizer);
    }

    void SetState(SwitchState state)
    {
        mCurrentState = state;

        SetImageForCurrentState();
    }

    void SetEnabled(bool isEnabled)
    {
        mIsEnabled = isEnabled;

        SetImageForCurrentState();
    }

private:

    void SetImageForCurrentState()
    {
        if (mIsEnabled)
        {
            if (mCurrentState == SwitchState::On)
            {
                mImageBitmap->SetBitmap(mOnEnabledImage);
            }
            else
            {
                assert(mCurrentState == SwitchState::Off);
                mImageBitmap->SetBitmap(mOffEnabledImage);
            }
        }
        else
        {
            if (mCurrentState == SwitchState::On)
            {
                mImageBitmap->SetBitmap(mOnDisabledImage);
            }
            else
            {
                assert(mCurrentState == SwitchState::Off);
                mImageBitmap->SetBitmap(mOffDisabledImage);
            }
        }
    }

private:

    wxBitmap const & mOnEnabledImage;
    wxBitmap const & mOffEnabledImage;
    wxBitmap const & mOnDisabledImage;
    wxBitmap const & mOffDisabledImage;

    wxStaticBitmap * mImageBitmap;

private:

    SwitchId const mSwitchId;
    SwitchState mCurrentState;
    bool mIsEnabled;
};
