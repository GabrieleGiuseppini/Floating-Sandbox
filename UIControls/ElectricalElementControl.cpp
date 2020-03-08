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

void GaugeElectricalElementControl::Update()
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
// EngineControllerElectricalElementControl
///////////////////////////////////////////////////////////////////////////////////////////////////////

void EngineControllerElectricalElementControl::OnKeyboardShortcutDown(bool isShift)
{
    if (mIsEnabled)
    {
        if (!isShift)
        {
            // Plus
            if (mCurrentValue < mMaxValue)
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

void EngineControllerElectricalElementControl::OnPaint(wxPaintEvent & /*event*/)
{
    wxPaintDC dc(mImagePanel);
    Render(dc);
}

void EngineControllerElectricalElementControl::Render(wxDC & dc)
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
}

void EngineControllerElectricalElementControl::OnLeftDown(wxMouseEvent & event)
{
    if (mIsEnabled)
    {
        //
        // Calculate direction of hand movement
        //

        // Center->Click (positive y down)
        vec2f const clickVector =
            vec2f(static_cast<float>(event.GetPosition().x), static_cast<float>(event.GetPosition().y))
            - mCenterPoint;

        // Click CCW angle (CW angle becomes CCW due to inverted y)
        float clickCCWAngle = clickVector.angleCw();
        if (clickCCWAngle < -Pi<float> / 2.0f) // Wrap around on the left side
            clickCCWAngle += 2.0f * Pi<float>;

        // Continue only if the click is in the telegraph range
        if (clickCCWAngle >= mHandMaxCCWAngle && clickCCWAngle <= mHand0CCWAngle)
        {
            float const halfSectorAngle =
                std::abs(mHandMaxCCWAngle - mHand0CCWAngle)
                / static_cast<float>(mMaxValue)
                / 2.0f;

            // Current hand CCW angle (CW angle becomes CCW due to inverted y)
            float const handCCWAngle =
                (mHand0CCWAngle - halfSectorAngle)
                + (mHandMaxCCWAngle - mHand0CCWAngle + 2.0f * halfSectorAngle)
                    * static_cast<float>(mCurrentValue)
                    / static_cast<float>(mMaxValue);

            if (abs(clickCCWAngle - handCCWAngle) > halfSectorAngle)
            {
                if (clickCCWAngle <= handCCWAngle)
                {
                    // Increase
                    if (mCurrentValue < mMaxValue)
                    {
                        ++mCurrentValue;
                        mOnControllerUpdated(mCurrentValue);
                    }
                }
                else
                {
                    // Decrease
                    if (mCurrentValue > 0)
                    {
                        --mCurrentValue;
                        mOnControllerUpdated(mCurrentValue);
                    }
                }
            }
        }
    }
}