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
#include <wx/tooltip.h>
#include <wx/wx.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class ElectricalElementControl : public wxPanel
{
public:

    void SetKeyboardShortcutLabel(std::string const & label)
    {
        mImageBitmap->SetToolTip(label);
    }

    ElectricalState GetState() const
    {
        return mCurrentState;
    }

    void SetState(ElectricalState state)
    {
        mCurrentState = state;

        SetImageForCurrentState();
    }

    ElectricalState ToggleState()
    {
        SetState(mCurrentState == ElectricalState::On ? ElectricalState::Off : ElectricalState::On);

        return mCurrentState;
    }

    void SetEnabled(bool isEnabled)
    {
        mIsEnabled = isEnabled;

        SetImageForCurrentState();
    }

protected:

    ElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        ElectricalState currentState)
        : wxPanel(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE)
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

        mImageBitmap = new wxStaticBitmap(this, wxID_ANY, GetImageForCurrentState(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        wxStaticText * labelStaticText = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0);
        vSizer->Add(labelStaticText, 0, wxALIGN_CENTRE_HORIZONTAL);

        this->SetSizerAndFit(vSizer);
    }

private:

    wxBitmap const & GetImageForCurrentState() const
    {
        if (mIsEnabled)
        {
            if (mCurrentState == ElectricalState::On)
            {
                return mOnEnabledImage;
            }
            else
            {
                assert(mCurrentState == ElectricalState::Off);
                return mOffEnabledImage;
            }
        }
        else
        {
            if (mCurrentState == ElectricalState::On)
            {
                return mOnDisabledImage;
            }
            else
            {
                assert(mCurrentState == ElectricalState::Off);
                return mOffDisabledImage;
            }
        }
    }

    void SetImageForCurrentState()
    {
        mImageBitmap->SetBitmap(GetImageForCurrentState());

        Refresh();
    }

protected:

    ElectricalState mCurrentState;
    bool mIsEnabled;

    wxStaticBitmap * mImageBitmap;

private:

    wxBitmap const & mOnEnabledImage;
    wxBitmap const & mOffEnabledImage;
    wxBitmap const & mOnDisabledImage;
    wxBitmap const & mOffDisabledImage;
};

class InteractiveToggleSwitchElectricalElementControl : public ElectricalElementControl
{
public:

    InteractiveToggleSwitchElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        std::function<void(ElectricalState)> onSwitchToggled,
        ElectricalState currentState)
        : ElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            currentState)
        , mOnSwitchToggled(std::move(onSwitchToggled))
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractiveToggleSwitchElectricalElementControl::OnLeftDown, this);
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        if (mIsEnabled)
        {
            //
            // Just invoke the callback, we'll end up being toggled when the event travels back
            //

            ElectricalState const newState = (mCurrentState == ElectricalState::On)
                ? ElectricalState::Off
                : ElectricalState::On;

            mOnSwitchToggled(newState);
        }
    }

private:

    std::function<void(ElectricalState)> const mOnSwitchToggled;
};

class InteractivePushSwitchElectricalElementControl : public ElectricalElementControl
{
public:

    InteractivePushSwitchElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        std::function<void(ElectricalState)> onSwitchToggled,
        ElectricalState currentState)
        : ElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            currentState)
        , mOnSwitchToggled(std::move(onSwitchToggled))
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftDown, this);
        mImageBitmap->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftUp, this);
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        if (mIsEnabled)
        {
            //
            // Just invoke the callback, we'll end up being toggled when the event travels back
            //

            ElectricalState const newState = (mCurrentState == ElectricalState::On)
                ? ElectricalState::Off
                : ElectricalState::On;

            mOnSwitchToggled(newState);
        }
    }

    void OnLeftUp(wxMouseEvent & /*event*/)
    {
        //
        // Just invoke the callback, we'll end up being toggled when the event travels back
        //

        ElectricalState const newState = (mCurrentState == ElectricalState::On)
            ? ElectricalState::Off
            : ElectricalState::On;

        mOnSwitchToggled(newState);
    }

private:

    std::function<void(ElectricalState)> const mOnSwitchToggled;
};

class AutomaticSwitchElectricalElementControl : public ElectricalElementControl
{
public:

    AutomaticSwitchElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        ElectricalState currentState)
        : ElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            currentState)
    {
    }
};

class PowerMonitorElectricalElementControl : public ElectricalElementControl
{
public:

    PowerMonitorElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onImage,
        wxBitmap const & offImage,
        std::string const & label,
        ElectricalState currentState)
        : ElectricalElementControl(
            parent,
            onImage,
            offImage,
            onImage,
            offImage,
            label,
            currentState)
    {
    }
};
