/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Simulation/SimulationParameters.h>

#include <Core/GameTypes.h>
#include <Core/Log.h>
#include <Core/Vectors.h>

#include <wx/bitmap.h>
#include <wx/cursor.h>
#include <wx/stattext.h>
#include <wx/tooltip.h>
#include <wx/wx.h>

#include <cassert>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ElectricalElementControl : public wxPanel
{
public:

    enum class ControlType
    {
        Switch,
        PowerMonitor,
        Gauge,
        EngineController
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
            labelSizer->Add(labelStaticText, 1, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT, 6);
            labelPanel->SetSizer(labelSizer);
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

    virtual void OnKeyboardShortcutDown(bool isShift) = 0;

    virtual void OnKeyboardShortcutUp() = 0;
};

class IUpdateableElectricalElementControl
{
public:

    virtual ~IUpdateableElectricalElementControl()
    {}

    virtual void UpdateSimulation() = 0;
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
        wxCursor const & cursor,
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
        mImageBitmap->SetCursor(cursor);
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
        wxCursor const & cursor,
        std::function<void(ElectricalState)> onSwitchToggled,
        ElectricalState currentState)
        : InteractiveSwitchElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            cursor,
            std::move(onSwitchToggled),
            currentState)
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractiveToggleSwitchElectricalElementControl::OnLeftDown, this);
    }

    void OnKeyboardShortcutDown(bool /*isShift*/) override
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
        wxCursor const & cursor,
        std::function<void(ElectricalState)> onSwitchToggled,
        ElectricalState currentState)
        : InteractiveSwitchElectricalElementControl(
            parent,
            onEnabledImage,
            offEnabledImage,
            onDisabledImage,
            offDisabledImage,
            label,
            cursor,
            std::move(onSwitchToggled),
            currentState)
        , mIsPushed(false)
    {
        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftDown, this);
        mImageBitmap->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftUp, this);
        mImageBitmap->Bind(wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&InteractivePushSwitchElectricalElementControl::OnLeftUp, this);
    }

    void OnKeyboardShortcutDown(bool /*isShift*/) override
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
        wxCursor const & cursor,
        std::function<void()> onTick,
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
        mImageBitmap->SetCursor(cursor);

        mImageBitmap->Bind(
            wxEVT_LEFT_DOWN,
            [onTick](wxMouseEvent &)
            {
                onTick();
            });
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
        wxCursor const & cursor,
        std::function<void()> onTick,
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

        mImageBitmap->SetCursor(cursor);

        mImageBitmap->Bind(
            wxEVT_LEFT_DOWN,
            [onTick](wxMouseEvent &)
            {
                onTick();
            });

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
        float minAngle, // radians, CCW
        float maxAngle, // radians, CCW
        std::string const & label,
        wxCursor const & cursor,
        std::function<void()> onTick,
        float currentValue);

    void SetValue(float value)
    {
        mTargetAngle = CalculateAngle(value, mMinAngle, mMaxAngle);
    }

    virtual void UpdateSimulation() override;

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
                handLength * std::cosf(angle),
                -handLength * std::sinf(angle));
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
    float mCurrentAngle; // In radians, 0 at (1,0)
    float mCurrentVelocity;
    float mTargetAngle;

    wxPoint mHandEndpoint;
    wxPen const mHandPen1;
    wxPen const mHandPen2;
};

class EngineControllerElectricalElementControl
    : public ElectricalElementControl
    , public IDisablableElectricalElementControl
    , public IInteractiveElectricalElementControl
{
public:

    EngineControllerElectricalElementControl(
        wxWindow * parent,
        wxSize imageSize,
        std::string const & label)
        : ElectricalElementControl(
            ControlType::EngineController,
            parent,
        imageSize,
            label)
    {}

    virtual void SetValue(float controllerValue) = 0;
};


class EngineControllerTelegraphElectricalElementControl
    : public EngineControllerElectricalElementControl
{
private:

    using TelegraphValue = unsigned int; // Between 0 and EngineControllerTelegraphDegreesOfFreedom

    static TelegraphValue constexpr MaxTelegraphValue = static_cast<TelegraphValue>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom - 1);

public:

    EngineControllerTelegraphElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & enabledBackgroundImage,
        wxBitmap const & disabledBackgroundImage,
        std::vector<wxBitmap> const & handImages,
        wxPoint const & centerPoint,
        float hand0CCWAngle,
        float handMaxCCWAngle,
        std::string const & label,
        wxCursor const & cursor,
        std::function<void(float)> onControllerUpdated,
        float currentValue)
        : EngineControllerElectricalElementControl(
            parent,
            enabledBackgroundImage.GetSize(),
            label)
        , mEnabledBackgroundImage(enabledBackgroundImage)
        , mDisabledBackgroundImage(disabledBackgroundImage)
        , mHandImages(handImages)
        , mCenterPoint(static_cast<float>(centerPoint.x), static_cast<float>(centerPoint.y))
        , mHand0CCWAngle(hand0CCWAngle)
        , mHandMaxCCWAngle(handMaxCCWAngle)
        , mSectorAngle(std::abs(mHandMaxCCWAngle - mHand0CCWAngle) / static_cast<float>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom))
        , mOnControllerUpdated(std::move(onControllerUpdated))
        //
        , mCurrentValue(ControllerValueToTelegraphValue(currentValue))
        , mIsEnabled(true)
        , mIsLeftMouseDown(false)
        , mIsMouseCaptured(false)
    {
        assert(mHandImages.size() == SimulationParameters::EngineControllerTelegraphDegreesOfFreedom);

        mImagePanel->SetCursor(cursor);

#ifdef __WXMSW__
        mImagePanel->SetDoubleBuffered(true);
#endif

        mImagePanel->Bind(wxEVT_PAINT, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnPaint, this);

        mImagePanel->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnLeftDown, this);
        mImagePanel->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnLeftUp, this);
    }

    void SetValue(float controllerValue) override
    {
        mCurrentValue = ControllerValueToTelegraphValue(controllerValue);

        Refresh();
    }

    virtual bool IsEnabled() const override
    {
        return mIsEnabled;
    }

    virtual void SetEnabled(bool isEnabled) override
    {
        mIsEnabled = isEnabled;

        Refresh();
    }

    virtual void SetKeyboardShortcutLabel(std::string const & label) override
    {
        mImagePanel->SetToolTip(label);
    }

    void OnKeyboardShortcutDown(bool isShift) override;

    void OnKeyboardShortcutUp() override
    {
        // Ignore
    }

private:

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

    void OnLeftDown(wxMouseEvent & event);

    void OnLeftUp(wxMouseEvent & event);

    void OnMouseMove(wxMouseEvent & event);

private:

    std::optional<TelegraphValue> PointToValue(wxPoint const & point) const;

    void MoveToPoint(wxPoint const & point);

    float TelegraphValueToControllerValue(TelegraphValue telegraphValue) const
    {
        // 0 -> -1.0
        // EngineControllerTelegraphDegreesOfFreedom / 2 -> 0.0
        // MaxValue (EngineControllerTelegraphDegreesOfFreedom - 1) -> 1.0

        return (static_cast<float>(telegraphValue) - static_cast<float>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom / 2))
            / static_cast<float>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom / 2);
    }

    TelegraphValue ControllerValueToTelegraphValue(float controllerValue) const
    {
        return static_cast<TelegraphValue>(controllerValue * static_cast<float>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom / 2))
            + static_cast<TelegraphValue>(SimulationParameters::EngineControllerTelegraphDegreesOfFreedom / 2);
    }

private:

    wxBitmap const mEnabledBackgroundImage;
    wxBitmap const mDisabledBackgroundImage;
    std::vector<wxBitmap> const mHandImages;
    vec2f const mCenterPoint;
    float const mHand0CCWAngle;
    float const mHandMaxCCWAngle;
    float const mSectorAngle;
    std::function<void(float)> mOnControllerUpdated;

    // Current state
    TelegraphValue mCurrentValue; // Between 0 and EngineTelegraphDegreesOfFreedom - 1
    bool mIsEnabled;
    bool mIsLeftMouseDown;
    bool mIsMouseCaptured;
};

class EngineControllerJetEngineThrottleElectricalElementControl
    : public EngineControllerElectricalElementControl
{
public:

    EngineControllerJetEngineThrottleElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & enabledBackgroundImage,
        wxBitmap const & disabledBackgroundImage,
        wxBitmap const & enabledHandleImage,
        wxBitmap const & disabledHandleImage,
        wxPoint const & centerPoint,
        int topY,
        std::string const & label,
        wxCursor const & cursor,
        std::function<void(float)> onControllerUpdated,
        float currentValue)
        : EngineControllerElectricalElementControl(
            parent,
            enabledBackgroundImage.GetSize(),
            label)
        , mEnabledBackgroundImage(enabledBackgroundImage)
        , mDisabledBackgroundImage(disabledBackgroundImage)
        , mEnabledHandleImage(enabledHandleImage)
        , mDisabledHandleImage(disabledHandleImage)
        , mCenterPoint(centerPoint)
        , mYExtent(static_cast<float>(centerPoint.y - topY + 1))
        , mOnControllerUpdated(std::move(onControllerUpdated))
        //
        , mCurrentValue(currentValue)
        , mCurrentEngagementY()
        , mIdleBlockHandleUp(false) // Arbitrary, will be set at state transitions
        , mIdleBlockHandleDown(false) // Arbitrary, will be set at state transitions
        , mIsMouseCaptured(false)
        , mIsEnabled(true)
    {
        assert(mEnabledHandleImage.GetSize() == mDisabledHandleImage.GetSize());

        mImagePanel->SetCursor(cursor);

#ifdef __WXMSW__
        mImagePanel->SetDoubleBuffered(true);
#endif

        mImagePanel->Bind(wxEVT_PAINT, (wxObjectEventFunction)&EngineControllerJetEngineThrottleElectricalElementControl::OnPaint, this);

        mImagePanel->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&EngineControllerJetEngineThrottleElectricalElementControl::OnLeftDown, this);
        mImagePanel->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&EngineControllerJetEngineThrottleElectricalElementControl::OnLeftUp, this);
        mImagePanel->Bind(wxEVT_MOTION, (wxObjectEventFunction)&EngineControllerJetEngineThrottleElectricalElementControl::OnMouseMove, this);
    }

    void SetValue(float controllerValue) override
    {
        mCurrentValue = controllerValue;

        Refresh();
    }

    virtual bool IsEnabled() const override
    {
        return mIsEnabled;
    }

    virtual void SetEnabled(bool isEnabled) override
    {
        mIsEnabled = isEnabled;

        Refresh();
    }

    virtual void SetKeyboardShortcutLabel(std::string const & label) override
    {
        mImagePanel->SetToolTip(label);
    }

    void OnKeyboardShortcutDown(bool isShift) override;

    void OnKeyboardShortcutUp() override
    {
        // Ignore
    }

private:

    void OnPaint(wxPaintEvent & event);

    void Render(wxDC & dc);

    void OnLeftDown(wxMouseEvent & event);

    void OnLeftUp(wxMouseEvent & event);

    void OnMouseMove(wxMouseEvent & event);

private:

    inline float HandleStrideToControllerValueOffset(int handleStride) const;
    inline int ControllerValueOffsetToHandleStride(float controllerValueOffset) const;

private:

    wxBitmap const mEnabledBackgroundImage;
    wxBitmap const mDisabledBackgroundImage;
    wxBitmap const mEnabledHandleImage;
    wxBitmap const mDisabledHandleImage;
    wxPoint const mCenterPoint;
    float const mYExtent;

    std::function<void(float)> mOnControllerUpdated;

    // Current state
    float mCurrentValue; // The engine controller range (-1.0,...,1.0), but in practice only (0.0,...,1.0)
    std::optional<int> mCurrentEngagementY;
    bool mIdleBlockHandleUp;
    bool mIdleBlockHandleDown;
    bool mIsMouseCaptured;
    bool mIsEnabled;
};

class EngineControllerJetEngineThrustElectricalElementControl
    : public EngineControllerElectricalElementControl
{
public:

    EngineControllerJetEngineThrustElectricalElementControl(
        wxWindow * parent,
        wxBitmap const & onEnabledImage,
        wxBitmap const & offEnabledImage,
        wxBitmap const & onDisabledImage,
        wxBitmap const & offDisabledImage,
        std::string const & label,
        wxCursor const & cursor,
        std::function<void(float)> onControllerUpdated,
        float currentValue)
        : EngineControllerElectricalElementControl(
            parent,
            onEnabledImage.GetSize(),
            label)
        , mOnEnabledImage(onEnabledImage)
        , mOffEnabledImage(offEnabledImage)
        , mOnDisabledImage(onDisabledImage)
        , mOffDisabledImage(offDisabledImage)
        , mOnControllerUpdated(std::move(onControllerUpdated))
        //
        , mCurrentValue(currentValue)
        , mIsEnabled(true)
    {
        mImagePanel->SetCursor(cursor);

        {
            wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

            mImageBitmap = new wxStaticBitmap(mImagePanel, wxID_ANY, GetImageForCurrentState(), wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
            vSizer->Add(mImageBitmap, 0, wxALIGN_CENTRE_HORIZONTAL);

            mImagePanel->SetSizerAndFit(vSizer);
        }

        mImageBitmap->Bind(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&EngineControllerJetEngineThrustElectricalElementControl::OnLeftDown, this);
        mImageBitmap->Bind(wxEVT_LEFT_UP, (wxObjectEventFunction)&EngineControllerJetEngineThrustElectricalElementControl::OnLeftUp, this);
        mImageBitmap->Bind(wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&EngineControllerJetEngineThrustElectricalElementControl::OnLeftUp, this);
    }

    void SetValue(float controllerValue) override
    {
        mCurrentValue = controllerValue;

        mImageBitmap->SetBitmap(GetImageForCurrentState());

        Refresh();
    }

    virtual bool IsEnabled() const override
    {
        return mIsEnabled;
    }

    virtual void SetEnabled(bool isEnabled) override
    {
        mIsEnabled = isEnabled;

        mImageBitmap->SetBitmap(GetImageForCurrentState());

        Refresh();
    }

    virtual void SetKeyboardShortcutLabel(std::string const & label) override
    {
        mImageBitmap->SetToolTip(label);
    }

    void OnKeyboardShortcutDown(bool /*isShift*/) override
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

    void OnDown()
    {
        if (mIsEnabled && mCurrentValue != 1.0f)
        {
            mCurrentValue = 1.0f;

            // Just invoke the callback, we'll end up being toggled when the event travels back
            mOnControllerUpdated(mCurrentValue);
        }
    }

    void OnUp()
    {
        if (mCurrentValue == 1.0f)
        {
            mCurrentValue = 0.0f;

            // Just invoke the callback, we'll end up being toggled when the event travels back
            mOnControllerUpdated(mCurrentValue);
        }
    }

    wxBitmap const & GetImageForCurrentState() const
    {
        if (mIsEnabled)
        {
            if (mCurrentValue == 1.0f)
            {
                return mOnEnabledImage;
            }
            else
            {
                assert(mCurrentValue == 0.0f);
                return mOffEnabledImage;
            }
        }
        else
        {
            if (mCurrentValue == 1.0f)
            {
                return mOnDisabledImage;
            }
            else
            {
                assert(mCurrentValue == 0.0f);
                return mOffDisabledImage;
            }
        }
    }

private:

    wxBitmap const mOnEnabledImage;
    wxBitmap const mOffEnabledImage;
    wxBitmap const mOnDisabledImage;
    wxBitmap const mOffDisabledImage;

    std::function<void(float)> mOnControllerUpdated;

    // Current state
    float mCurrentValue;
    bool mIsEnabled;

    // UI
    wxStaticBitmap * mImageBitmap;
};
