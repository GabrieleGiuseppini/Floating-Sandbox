/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-12-27
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ElectricalElementControl.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// GaugeElectricalElementControl
///////////////////////////////////////////////////////////////////////////////////////////////////////

GaugeElectricalElementControl::GaugeElectricalElementControl(
    wxWindow * parent,
    wxBitmap const & backgroundImage,
    wxPoint const & centerPoint,
    float handLength,
    float minAngle,
    float maxAngle,
    std::string const & label,
    wxCursor const & cursor,
    std::function<void()> onTick,
    float currentValue)
    : ElectricalElementControl(
        ControlType::Gauge,
        parent,
        backgroundImage.GetSize(),
        label)
    , mBackgroundImage(backgroundImage)
    , mCenterPoint(centerPoint)
    , mHandLength(handLength)
    , mMinAngle(minAngle)
    , mMaxAngle(maxAngle)
    //
    , mCurrentAngle(CalculateAngle(currentValue, minAngle, maxAngle))
    , mCurrentVelocity(0.0f)
    , mTargetAngle(CalculateAngle(currentValue, minAngle, maxAngle))
    //
    , mHandEndpoint(CalculateHandEndpoint(mCenterPoint, mHandLength, mCurrentAngle))
    , mHandPen1(wxColor(0xdb, 0x04, 0x04), 3, wxPENSTYLE_SOLID)
    , mHandPen2(wxColor(0xd8, 0xd8, 0xd8), 1, wxPENSTYLE_SOLID)
{
    mImagePanel->SetCursor(cursor);

#ifdef __WXMSW__
    mImagePanel->SetDoubleBuffered(true);
#endif

    mImagePanel->Bind(wxEVT_PAINT, (wxObjectEventFunction)&GaugeElectricalElementControl::OnPaint, this);

    mImagePanel->Bind(
        wxEVT_LEFT_DOWN,
        [onTick](wxMouseEvent &)
        {
            onTick();
        });
}

void GaugeElectricalElementControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(mImagePanel);
    Render(dc);
}

void GaugeElectricalElementControl::UpdateSimulation()
{
    //
    // Update physics
    //

    float constexpr Stiffness = 0.4f;
    float constexpr Dt = 0.11f;
    float constexpr InvDamping = 0.930f;

    float const acceleration = Stiffness * (mTargetAngle - mCurrentAngle);
    float const deltaAngle = mCurrentVelocity * Dt + acceleration * Dt * Dt;
    mCurrentAngle += deltaAngle;
    mCurrentVelocity = InvDamping * deltaAngle / Dt;

    //
    // Update hand endpoint
    //

    mHandEndpoint = CalculateHandEndpoint(mCenterPoint, mHandLength, mCurrentAngle);

    //
    // Redraw
    //

    mImagePanel->Refresh();
}

void GaugeElectricalElementControl::Render(wxDC & dc)
{
    //
    // Draw background image
    //

    dc.DrawBitmap(
        mBackgroundImage,
        0,
        0,
        true);

    //
    // Draw hand
    //

    dc.SetPen(mHandPen1);
    dc.DrawLine(mCenterPoint, mHandEndpoint);
    dc.SetPen(mHandPen2);
    dc.DrawLine(mCenterPoint, mHandEndpoint);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// EngineControllerTelegraphElectricalElementControl
///////////////////////////////////////////////////////////////////////////////////////////////////////

void EngineControllerTelegraphElectricalElementControl::OnKeyboardShortcutDown(bool isShift)
{
    if (mIsEnabled)
    {
        if (!isShift)
        {
            // Plus
            if (mCurrentValue < MaxTelegraphValue)
                ++mCurrentValue;
        }
        else
        {
            // Minus
            if (mCurrentValue > 0)
                --mCurrentValue;
        }

        mOnControllerUpdated(TelegraphValueToControllerValue(mCurrentValue));
    }
}

void EngineControllerTelegraphElectricalElementControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(mImagePanel);
    Render(dc);
}

void EngineControllerTelegraphElectricalElementControl::Render(wxDC & dc)
{
    //
    // Draw background image
    //

    dc.DrawBitmap(
        mIsEnabled ? mEnabledBackgroundImage : mDisabledBackgroundImage,
        0,
        0,
        true);

    //
    // Draw hand
    //

    assert(mCurrentValue >= 0 && mCurrentValue < mHandImages.size());

    dc.DrawBitmap(
        mHandImages[mCurrentValue],
        0,
        0,
        true);

    // TEST: to draw sectors
    ////wxPen handPen(wxColor(0xd8, 0xd8, 0xd8), 1, wxPENSTYLE_SOLID);
    ////dc.SetPen(handPen);
    ////for (ControllerValue i = 0; i <= MaxTelegraphValue + 1; ++i)
    ////{
    ////    float angle = mHandMaxCCWAngle + mSectorAngle * static_cast<float>(i);
    ////    vec2f p(37.0f, 0.0f);
    ////    p = p.rotate(angle);
    ////    dc.DrawLine(
    ////        wxPoint(mCenterPoint.x, mCenterPoint.y),
    ////        wxPoint(mCenterPoint.x, mCenterPoint.y) + wxPoint(p.x, -p.y));
    ////}
}

void EngineControllerTelegraphElectricalElementControl::OnLeftDown(wxMouseEvent & event)
{
    if (mIsEnabled)
    {
        // Capture mouse
        if (!mIsMouseCaptured)
        {
            mImagePanel->CaptureMouse();
            mIsMouseCaptured = true;
        }

        // Register for mouse move events
        mImagePanel->Unbind(wxEVT_MOTION, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnMouseMove, this);
        mImagePanel->Bind(wxEVT_MOTION, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnMouseMove, this);

        // Move to this point
        MoveToPoint(event.GetPosition());
    }

    // Remember state of left button
    mIsLeftMouseDown = true;
}

void EngineControllerTelegraphElectricalElementControl::OnLeftUp(wxMouseEvent & /*event*/)
{
    // De-register for mouse move events
    mImagePanel->Unbind(wxEVT_MOTION, (wxObjectEventFunction)&EngineControllerTelegraphElectricalElementControl::OnMouseMove, this);

    // Release mouse capture
    assert(mIsMouseCaptured || !mIsEnabled);
    if (mIsMouseCaptured)
    {
        mImagePanel->ReleaseMouse();
        mIsMouseCaptured = false;
    }

    // Remember state of left button
    mIsLeftMouseDown = false;
}

void EngineControllerTelegraphElectricalElementControl::OnMouseMove(wxMouseEvent & event)
{
    if (mIsEnabled && mIsLeftMouseDown)
    {
        MoveToPoint(event.GetPosition());
    }
}

std::optional<EngineControllerTelegraphElectricalElementControl::TelegraphValue> EngineControllerTelegraphElectricalElementControl::PointToValue(wxPoint const & point) const
{
    // Center->Click (positive y down)
    vec2f const clickVector =
        vec2f(static_cast<float>(point.x), static_cast<float>(point.y))
        - mCenterPoint;

    // Click CCW angle (CW angle becomes CCW due to inverted y)
    float clickCCWAngle = clickVector.angleCw();
    if (clickCCWAngle < -Pi<float> / 2.0f) // Wrap around on the left side
        clickCCWAngle += 2.0f * Pi<float>;

    // Value
    float const sector = (clickCCWAngle - mHandMaxCCWAngle) / mSectorAngle;
    auto value = MaxTelegraphValue - static_cast<TelegraphValue>(std::floor(sector));

    // Continue only if the click is in the telegraph range
    if (value >= 0 && value <= MaxTelegraphValue)
    {
        return value;
    }
    else
    {
        return std::nullopt;
    }
}

void EngineControllerTelegraphElectricalElementControl::MoveToPoint(wxPoint const & point)
{
    // Map to value
    std::optional<TelegraphValue> value = PointToValue(point);

    // Move to value, if valid and different
    if (value.has_value()
        && *value != mCurrentValue)
    {
        mCurrentValue = *value;

        // Notify
        mOnControllerUpdated(TelegraphValueToControllerValue(mCurrentValue));
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// EngineControllerJetEngineThrottleElectricalElementControl
///////////////////////////////////////////////////////////////////////////////////////////////////////

void EngineControllerJetEngineThrottleElectricalElementControl::OnKeyboardShortcutDown(bool isShift)
{
    float constexpr ControllerValueStep = 0.1f;

    if (mIsEnabled)
    {
        if (!isShift)
        {
            // Up
            if (mCurrentValue == 0.0f)
            {
                mCurrentValue = SimulationParameters::EngineControllerJetThrottleIdleFraction;
            }
            else
            {
                mCurrentValue = std::min(mCurrentValue + ControllerValueStep, 1.0f);
            }
        }
        else
        {
            // Down
            if (mCurrentValue > SimulationParameters::EngineControllerJetThrottleIdleFraction)
            {
                mCurrentValue = std::max(mCurrentValue - ControllerValueStep, SimulationParameters::EngineControllerJetThrottleIdleFraction);
            }
            else if (mCurrentValue == SimulationParameters::EngineControllerJetThrottleIdleFraction)
            {
                mCurrentValue = 0.0f;
            }
        }

        mOnControllerUpdated(mCurrentValue);
    }
}

void EngineControllerJetEngineThrottleElectricalElementControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(mImagePanel);
    Render(dc);
}

void EngineControllerJetEngineThrottleElectricalElementControl::Render(wxDC & dc)
{
    //
    // Draw background image
    //

    dc.DrawBitmap(
        mIsEnabled ? mEnabledBackgroundImage : mDisabledBackgroundImage,
        0,
        0,
        true);

    //
    // Draw handle
    //

    dc.DrawBitmap(
        mIsEnabled ? mEnabledHandleImage: mDisabledHandleImage,
        mCenterPoint.x - mEnabledHandleImage.GetWidth() / 2,
        mCenterPoint.y - static_cast<int>(mCurrentValue * mYExtent) - mEnabledHandleImage.GetHeight() / 2,
        true);
}

void EngineControllerJetEngineThrottleElectricalElementControl::OnLeftDown(wxMouseEvent & event)
{
    if (mIsEnabled)
    {
        // Capture mouse
        if (!mIsMouseCaptured)
        {
            mImagePanel->CaptureMouse();
            mIsMouseCaptured = true;
        }

        // Start engagement
        mCurrentEngagementY.emplace(event.GetY());
    }
}

void EngineControllerJetEngineThrottleElectricalElementControl::OnLeftUp(wxMouseEvent & /*event*/)
{
    // Release mouse capture
    if (mIsMouseCaptured)
    {
        mImagePanel->ReleaseMouse();
        mIsMouseCaptured = false;
    }

    // Reset engagement
    mCurrentEngagementY.reset();

    // Unblock up and down (in case we've just landed at idle)
    mIdleBlockHandleUp = false;
    mIdleBlockHandleDown = false;
}

void EngineControllerJetEngineThrottleElectricalElementControl::OnMouseMove(wxMouseEvent & event)
{
    int constexpr IdleThreshold = 18;

    if (mCurrentEngagementY.has_value())
    {
        // Calculate Y stride (positive up)
        int const yStride = -(event.GetY() - *mCurrentEngagementY);

        // Process stride depending on current state
        if (mCurrentValue == 0.0f)
        {
            // At zero

            // Go to Idle if more than magic stride
            if (yStride > IdleThreshold)
            {
                // Go to Idle
                mCurrentValue = SimulationParameters::EngineControllerJetThrottleIdleFraction;
                mOnControllerUpdated(mCurrentValue);

                // Transition state
                mIdleBlockHandleUp = true;
                mCurrentEngagementY = event.GetY();

                Refresh();
            }
        }
        else if (mCurrentValue == SimulationParameters::EngineControllerJetThrottleIdleFraction)
        {
            // At Idle

            // Go to Free if can, and more than magic stride
            if (!mIdleBlockHandleUp && yStride > IdleThreshold)
            {
                // Go to Free
                mCurrentValue = std::min(mCurrentValue + HandleStrideToControllerValueOffset(yStride - IdleThreshold), 1.0f);
                mOnControllerUpdated(mCurrentValue);

                // Transition state
                mCurrentEngagementY = event.GetY();

                Refresh();
            }
            // Go to Zero if can, and more than magic stide
            else if (!mIdleBlockHandleDown && yStride < -IdleThreshold)
            {
                // Go to Zero
                mCurrentValue = 0.0f;
                mOnControllerUpdated(mCurrentValue);

                // Transition state
                mCurrentEngagementY = event.GetY();

                Refresh();
            }
        }
        else
        {
            // Free
            assert(mCurrentValue > SimulationParameters::EngineControllerJetThrottleIdleFraction);

            if (yStride > 0)
            {
                // Move staying in Free
                mCurrentValue = std::min(mCurrentValue + HandleStrideToControllerValueOffset(yStride), 1.0f);
                mOnControllerUpdated(mCurrentValue);

                // Transition state
                mCurrentEngagementY = event.GetY();

                Refresh();
            }
            else if (yStride < 0)
            {
                // Move down towards Idle
                float const newValue = mCurrentValue + HandleStrideToControllerValueOffset(yStride);
                mCurrentValue = std::max(newValue, SimulationParameters::EngineControllerJetThrottleIdleFraction);
                if (mCurrentValue == SimulationParameters::EngineControllerJetThrottleIdleFraction)
                {
                    // Go to Idle...

                    // ...but not further down
                    mIdleBlockHandleDown = true;
                }

                mOnControllerUpdated(mCurrentValue);

                // Transition state
                mCurrentEngagementY = event.GetY();

                Refresh();
            }
        }
    }
}

float constexpr MagicResistance = 2.0f;

float EngineControllerJetEngineThrottleElectricalElementControl::HandleStrideToControllerValueOffset(int handleStride) const
{
    return static_cast<float>(handleStride) / (mYExtent * MagicResistance);
}

int EngineControllerJetEngineThrottleElectricalElementControl::ControllerValueOffsetToHandleStride(float controllerValueOffset) const
{
    return static_cast<int>(controllerValueOffset * (mYExtent * MagicResistance));
}
