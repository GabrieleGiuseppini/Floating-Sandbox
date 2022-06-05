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

        mOnControllerUpdated(mCurrentValue);
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
// EngineControllerJetThrottle
///////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO

///////////////////////////////////////////////////////////////////////////////////////////////////////
// EngineControllerJetThrust
///////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO