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
    mImagePanel->SetDoubleBuffered(true);
    mImagePanel->Bind(wxEVT_PAINT, (wxObjectEventFunction)&GaugeElectricalElementControl::OnPaint, this);
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

void EngineControllerElectricalElementControl::OnKeyboardShortcutDown()
{
    if (mIsEnabled)
    {
        ++mCurrentValue;
        if (mCurrentValue > mMaxValue)
            mCurrentValue = 0;

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

        // Center->CurrentHand (positive y down)
        float constexpr Hand0CCWAngle = 3.68915488f;
        float constexpr HandMaxCCWAngle = -0.5475622359f;
        float const handCCWAngle =
            Hand0CCWAngle
            + (HandMaxCCWAngle - Hand0CCWAngle) * static_cast<float>(mCurrentValue) / static_cast<float>(mMaxValue);
        vec2f const handVector = vec2f(std::cos(handCCWAngle), -std::sin(handCCWAngle));

        // See if click is CW or CCW wrt Center->CurrentHand
        if (handVector.angleCw(clickVector) >= 0.0f)
        {
            // Click is CW wrt Center->CurrentHand, but positive y is down, hence
            // click is CCW: increase
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