/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/Log.h>

#include <wx/bitmap.h>
#include <wx/stattext.h>
#include <wx/wx.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class ShipSwitchControl : public wxPanel
{
public:

    void SetState(SwitchState state)
    {
        mCurrentState = state;

        SetImageForCurrentState();
    }

    SwitchState ToggleState()
    {
        SetState(mCurrentState == SwitchState::On ? SwitchState::Off : SwitchState::On);

        return mCurrentState;
    }

    void SetEnabled(bool isEnabled)
    {
        mIsEnabled = isEnabled;

        SetImageForCurrentState();
    }

protected:

    ShipSwitchControl(
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
        , mSwitchId(switchId)
        , mCurrentState(currentState)
        , mIsEnabled(true)
        , mImageBitmap(nullptr)
        ///////////
        , mOnEnabledImage(onEnabledImage)
        , mOffEnabledImage(offEnabledImage)
        , mOnDisabledImage(onDisabledImage)
        , mOffDisabledImage(offDisabledImage)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImageBitmap = new wxStaticBitmap(this, wxID_ANY, mOnEnabledImage, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
        vSizer->Add(labelStaticText, 0, wxALIGN_CENTRE_HORIZONTAL);

        this->SetSizerAndFit(vSizer);
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

        Refresh();
    }

protected:

    SwitchId const mSwitchId;
    SwitchState mCurrentState;
    bool mIsEnabled;

    wxStaticBitmap * mImageBitmap;

private:

    wxBitmap const & mOnEnabledImage;
    wxBitmap const & mOffEnabledImage;
    wxBitmap const & mOnDisabledImage;
    wxBitmap const & mOffDisabledImage;
};

class ShipInteractiveSwitchControl : public ShipSwitchControl
{
public:

    ShipInteractiveSwitchControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        SwitchId switchId,
        std::string const & label,
        std::function<void(SwitchId, SwitchState)> onSwitchToggled,
        SwitchState currentState)
        : ShipSwitchControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            switchId,
            label,
            currentState)
        , mOnSwitchToggled(std::move(onSwitchToggled))
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&ShipInteractiveSwitchControl::OnLeftDown, this);
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        if (mIsEnabled)
        {
            //
            // Just invoke the callback, we'll end up being toggled when the event travels back
            //

            SwitchState const newState = (mCurrentState == SwitchState::On)
                ? SwitchState::Off
                : SwitchState::On;

            mOnSwitchToggled(mSwitchId, newState);
        }
    }

private:

    std::function<void(SwitchId, SwitchState)> const mOnSwitchToggled;
};

class ShipAutomaticSwitchControl : public ShipSwitchControl
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
        : ShipSwitchControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            switchId,
            label,
            currentState)
    {
    }
};
