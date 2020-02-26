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
    , mImageBitmap(nullptr)
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
    float constexpr Dt = 0.1f;
    float constexpr InvDamping = 0.925f;

    float const acceleration = Stiffness * (mTargetAngle - mCurrentAngle);
    float const deltaAngle = mCurrentVelocity * Dt + acceleration * Dt * Dt;
    mCurrentAngle += deltaAngle;
    mCurrentVelocity = InvDamping * deltaAngle / Dt;

    //
    // Update hand endpoint
    //

    mHandEndpoint =
        mCenterPoint
        + wxPoint(
            mHandLength * std::cos(mCurrentAngle),
            -mHandLength * std::sin(mCurrentAngle));

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