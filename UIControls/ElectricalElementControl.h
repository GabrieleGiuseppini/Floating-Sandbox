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
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class ElectricalElementControl : public wxPanel
{
public:

    enum class ControlType
    {
        Switch,
        PowerMonitor,
        Gauge
    };

public:

    virtual ~ElectricalElementControl()
    {}

    ControlType GetControlType() const
    {
        return mControlType;
    }

protected:

    ElectricalElementControl(
        ControlType controlType,
        wxWindow * parent,
        wxSize imageSize,
        std::string const & label)
        : wxPanel(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxBORDER_NONE)
        , mControlType(controlType)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImagePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, imageSize, wxBORDER_NONE);
        mImagePanel->SetMinSize(imageSize);
        vSizer->Add(mImagePanel, 0, wxALIGN_CENTRE_HORIZONTAL);

        vSizer->AddSpacer(4);

        wxPanel * labelPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
        {
            labelPanel->SetBackgroundColour(wxColour(165, 167, 156));

            wxStaticText * labelStaticText = new wxStaticText(
                labelPanel, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
            labelStaticText->SetForegroundColour(wxColour(0x20, 0x20, 0x20));
            wxFont font = labelStaticText->GetFont();
            font.SetPointSize(7);
            labelStaticText->SetFont(font);

            wxBoxSizer* labelSizer = new wxBoxSizer(wxVERTICAL);
            labelSizer->Add(labelStaticText, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 6);
            labelPanel->SetSizerAndFit(labelSizer);
        }
        vSizer->Add(labelPanel, 0, wxEXPAND);

        this->SetSizerAndFit(vSizer);
    }

protected:

    wxPanel * mImagePanel;

private:

    ControlType const mControlType;
};

class IDisablableElectricalElementControl
{
public:

    virtual ~IDisablableElectricalElementControl()
    {}

    virtual bool IsEnabled() const = 0;

    virtual void SetEnabled(bool isEnabled) = 0;
};

class IInteractiveElectricalElementControl
{
public:

    virtual ~IInteractiveElectricalElementControl()
    {}

    virtual void SetKeyboardShortcutLabel(std::string const & label) = 0;

    virtual void OnKeyboardShortcutDown() = 0;

    virtual void OnKeyboardShortcutUp() = 0;
};

class IUpdateableElectricalElementControl
{
public:

    virtual ~IUpdateableElectricalElementControl()
    {}

    virtual void Update() = 0;
};

class SwitchElectricalElementControl
    : public ElectricalElementControl
    , public IDisablableElectricalElementControl
{
public:

    virtual ~SwitchElectricalElementControl()
    {}

    ElectricalState GetState() const
    {
        return mCurrentState;
    }

    void SetState(ElectricalState state)
    {
        mCurrentState = state;

        SetImageForCurrentState();
    }

    virtual bool IsEnabled() const override
    {
        return mIsEnabled;
    }

    virtual void SetEnabled(bool isEnabled) override
    {
        mIsEnabled = isEnabled;

        SetImageForCurrentState();
    }

protected:

    SwitchElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        ElectricalState currentState)
        : ElectricalElementControl(
            ControlType::Switch,
            parent,
            onEnabledImage.GetSize(), // Arbitrarily the first one
            label)
        , mCurrentState(currentState)
        , mIsEnabled(true)
        , mImageBitmap(nullptr)
        //
        , mOnEnabledImage(onEnabledImage)
        , mOffEnabledImage(offEnabledImage)
        , mOnDisabledImage(onDisabledImage)
        , mOffDisabledImage(offDisabledImage)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImageBitmap = new wxStaticBitmap(mImagePanel, wxID_ANY, GetImageForCurrentState(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        mImagePanel->SetSizerAndFit(vSizer);
    }

    void SetImageForCurrentState()
    {
        mImageBitmap->SetBitmap(GetImageForCurrentState());

        Refresh();
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

protected:

    ElectricalState mCurrentState;
    bool mIsEnabled;

    wxStaticBitmap * mImageBitmap;

private:

    wxBitmap const mOnEnabledImage;
    wxBitmap const mOffEnabledImage;
    wxBitmap const mOnDisabledImage;
    wxBitmap const mOffDisabledImage;
};

class InteractiveSwitchElectricalElementControl
    : public SwitchElectricalElementControl
    , public IInteractiveElectricalElementControl
{
public:

    virtual ~InteractiveSwitchElectricalElementControl()
    {}

    virtual void SetKeyboardShortcutLabel(std::string const & label) override
    {
        mImageBitmap->SetToolTip(label);
    }

protected:

    InteractiveSwitchElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        std::function<void(ElectricalState)> onSwitchToggled,
        ElectricalState currentState)
        : SwitchElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            currentState)
        , mOnSwitchToggled(std::move(onSwitchToggled))
    {
    }

protected:

    std::function<void(ElectricalState)> const mOnSwitchToggled;
};

class InteractiveToggleSwitchElectricalElementControl : public InteractiveSwitchElectricalElementControl
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
        : InteractiveSwitchElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            std::move(onSwitchToggled),
            currentState)
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractiveToggleSwitchElectricalElementControl::OnLeftDown, this);
    }

    void OnKeyboardShortcutDown() override
    {
        OnDown();
    }

    void OnKeyboardShortcutUp() override
    {
        // Ignore
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        OnDown();
    }

    void OnDown()
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
};

class InteractivePushSwitchElectricalElementControl : public InteractiveSwitchElectricalElementControl
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
        : InteractiveSwitchElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            std::move(onSwitchToggled),
            currentState)
        , mIsPushed(false)
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftDown, this);
        mImageBitmap->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftUp, this);
        mImageBitmap->Bind(wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftUp, this);
    }

    void OnKeyboardShortcutDown() override
    {
        OnDown();
    }

    void OnKeyboardShortcutUp() override
    {
        OnUp();
    }

private:

    void OnLeftDown(wxMouseEvent & /*event*/)
    {
        OnDown();
    }

    void OnLeftUp(wxMouseEvent & /*event*/)
    {
        OnUp();
    }

    void OnLeaveWindow(wxMouseEvent & /*event*/)
    {
        OnUp();
    }

    void OnDown()
    {
        if (mIsEnabled && !mIsPushed)
        {
            //
            // Just invoke the callback, we'll end up being toggled when the event travels back
            //

            ElectricalState const newState = (mCurrentState == ElectricalState::On)
                ? ElectricalState::Off
                : ElectricalState::On;

            mOnSwitchToggled(newState);

            mIsPushed = true;
        }
    }

    void OnUp()
    {
        if (mIsPushed)
        {
            //
            // Just invoke the callback, we'll end up being toggled when the event travels back
            //

            ElectricalState const newState = (mCurrentState == ElectricalState::On)
                ? ElectricalState::Off
                : ElectricalState::On;

            mOnSwitchToggled(newState);

            mIsPushed = false;
        }
    }

private:

    bool mIsPushed;
};

class AutomaticSwitchElectricalElementControl : public SwitchElectricalElementControl
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
        : SwitchElectricalElementControl(
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
            ControlType::PowerMonitor,
            parent,
            onImage.GetSize(), // Arbitrarily the first one
            label)
        , mCurrentState(currentState)
        , mImageBitmap(nullptr)
        , mOnImage(onImage)
        , mOffImage(offImage)
    {
        wxBoxSizer* vSizer = new wxBoxSizer(wxVERTICAL);

        mImageBitmap = new wxStaticBitmap(mImagePanel, wxID_ANY, GetImageForCurrentState(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

        mImagePanel->SetSizerAndFit(vSizer);
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

private:

    wxBitmap const & GetImageForCurrentState() const
    {
        if (mCurrentState == ElectricalState::On)
        {
            return mOnImage;
        }
        else
        {
            assert(mCurrentState == ElectricalState::Off);
            return mOffImage;
        }
    }

    void SetImageForCurrentState()
    {
        mImageBitmap->SetBitmap(GetImageForCurrentState());

        Refresh();
    }

private:

    ElectricalState mCurrentState;

    wxStaticBitmap * mImageBitmap;

    wxBitmap const mOnImage;
    wxBitmap const mOffImage;
};

class GaugeElectricalElementControl
    : public ElectricalElementControl
    , public IUpdateableElectricalElementControl
{
public:

    GaugeElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & backgroundImage,
        wxPoint const & centerPoint,
        float handLength,
        float minAngle,
        float maxAngle,
        std::string const & label,
        float currentValue);

    void SetValue(float value)
    {
        mTargetAngle = CalculateAngle(value, mMinAngle, mMaxAngle);
    }

    virtual void Update() override;

private:

    static inline float CalculateAngle(
        float currentValue,
        float minAngle,
        float maxAngle)
    {
        return minAngle + (maxAngle - minAngle) * currentValue;
    }

    static inline wxPoint CalculateHandEndpoint(
        wxPoint const & centerPoint,
        float handLength,
        float angle)
    {
        return centerPoint
            + wxPoint(
                handLength * std::cos(angle),
                -handLength * std::sin(angle));
    }

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

private:

    wxBitmap const mBackgroundImage;
    wxPoint const mCenterPoint;
    float const mHandLength;
    float const mMinAngle;
    float const mMaxAngle;

    // Current state
    float mCurrentAngle; // In radiants, 0 at (1,0)
    float mCurrentVelocity;
    float mTargetAngle;

    wxPoint mHandEndpoint;
    wxStaticBitmap * mImageBitmap;
    wxPen const mHandPen1;
    wxPen const mHandPen2;
};
