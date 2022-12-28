/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <GameCore/GameMath.h>

#include <cassert>

float constexpr NdcFractionZoomTarget = 0.7f; // Fraction of the [0, 2] NDC space that needs to be occupied by AABB
float constexpr MaxZoom = 2.0f; // Arbitrary max zoom, to avoid getting to atomic scale with e.g. Thanos

ViewManager::ViewManager(
    Render::RenderContext & renderContext,
    NotificationLayer & notificationLayer)
    : mRenderContext(renderContext)
    , mNotificationLayer(notificationLayer)
    , mZoomParameterSmoother()
    , mCameraWorldPositionParameterSmoother()
    // Defaults
    , mCameraSpeedAdjustment(1.0f)
    , mDoAutoFocusOnShipLoad(true)
    , mAutoFocus() // Set later
{
    mZoomParameterSmoother = std::make_unique<ParameterSmoother<float>>(
        [this]() -> float const &
        {
            return mRenderContext.GetZoom();
        },
        [this](float const & value) -> float const &
        {
            return mRenderContext.SetZoom(value);
        },
        [this](float const & value) -> float
        {
            return mRenderContext.ClampZoom(value);
        },
        CalculateZoomParameterSmootherConvergenceFactor(mCameraSpeedAdjustment),
        0.0001f);

    mCameraWorldPositionParameterSmoother = std::make_unique<ParameterSmoother<vec2f>>(
        [this]() -> vec2f const &
        {
            return mRenderContext.GetCameraWorldPosition();
        },
        [this](vec2f const & value) -> vec2f const &
        {
            return mRenderContext.SetCameraWorldPosition(value);
        },
        [this](vec2f const & value) -> vec2f
        {
            return mRenderContext.ClampCameraWorldPosition(value);
        },
        CalculateCameraWorldPositionParameterSmootherConvergenceFactor(mCameraSpeedAdjustment),
        0.001f);

    // Default: continuous auto-focus is ON
    SetDoContinuousAutoFocus(true);
}

float ViewManager::GetCameraSpeedAdjustment() const
{
    return mCameraSpeedAdjustment;
}

void ViewManager::SetCameraSpeedAdjustment(float value)
{
    mCameraSpeedAdjustment = value;

    mZoomParameterSmoother->SetConvergenceFactor(
        CalculateZoomParameterSmootherConvergenceFactor(mCameraSpeedAdjustment));

    mCameraWorldPositionParameterSmoother->SetConvergenceFactor(
        CalculateCameraWorldPositionParameterSmootherConvergenceFactor(mCameraSpeedAdjustment));
}

bool ViewManager::GetDoAutoFocusOnShipLoad() const
{ 
    return mDoAutoFocusOnShipLoad;
}

void ViewManager::SetDoAutoFocusOnShipLoad(bool value)
{ 
    mDoAutoFocusOnShipLoad = value;
}

bool ViewManager::GetDoContinuousAutoFocus() const
{ 
    return mAutoFocus.has_value(); 
}

void ViewManager::SetDoContinuousAutoFocus(bool value)
{
    if (value)
    {
        // Start auto-focus
        mAutoFocus.emplace(
            mZoomParameterSmoother->GetValue(),
            mCameraWorldPositionParameterSmoother->GetValue());
    }
    else
    {
        // Stop auto-focus
        mAutoFocus.reset();
    }

    mNotificationLayer.SetAutoFocusIndicator(mAutoFocus.has_value());
}

void ViewManager::OnViewModelUpdated()
{
    // Pickup eventual changes to view model constraints
    mZoomParameterSmoother->ReClamp();
    mCameraWorldPositionParameterSmoother->ReClamp();
}

void ViewManager::OnNewShip(Geometry::AABBSet const & allAABBs)
{
    if (mDoAutoFocusOnShipLoad)
    {
        FocusOnShip(allAABBs);
    }
}

void ViewManager::Pan(vec2f const & worldOffset)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition =
            mCameraWorldPositionParameterSmoother->GetValue()
            + worldOffset;

        mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset += worldOffset;
    }
}

void ViewManager::PanToWorldX(float worldX)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition = vec2f(
            worldX,
            mCameraWorldPositionParameterSmoother->GetValue().y);

        mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset.x = worldX;
    }
}

void ViewManager::AdjustZoom(float amount)
{
    if (!mAutoFocus.has_value())
    {
        float const newTargetZoom = mZoomParameterSmoother->GetValue() * amount;

        mZoomParameterSmoother->SetValue(newTargetZoom);
    }
    else
    {
        mAutoFocus->UserZoomOffset *= amount;
    }
}

void ViewManager::ResetView(Geometry::AABBSet const & allAABBs)
{
    // When continuous auto-focus is off, "view reset" focuses on ship;
    // When continuous auto-focus is on, "view reset" zeroes-out user offsets
    if (!mAutoFocus.has_value())
    {
        InternalFocusOnShip(allAABBs);
    }
    else
    {
        mAutoFocus->Reset();
    }
}

void ViewManager::FocusOnShip(Geometry::AABBSet const & allAABBs)
{
    if (!mAutoFocus.has_value())
    {
        InternalFocusOnShip(allAABBs);
    }
    else
    {
        mAutoFocus->Reset();
    }
}

void ViewManager::Update(Geometry::AABBSet const & allAABBs)
{
    if (mAutoFocus.has_value())
    {
        auto const unionAABB = allAABBs.MakeUnion();
        if (unionAABB.has_value())
        {
            //
            // Auto-focus algorithm:
            // - Zoom:
            //      - Auto-focus zoom is the zoom required to ensure that the AABB's width and height
            //        fall in a specific sub-window of the physical display window
            //      - User zoom is the zoom offset exherted by the user
            //      - The final zoom is the sum of the two zooms
            // - Pan:
            //      - Auto-focus pan is the pan required to ensure that the center of the AABB is at
            //        the center of the physical display window, after auto-focus zoom is applied
            //      - User pan is the pan offset exherted by the user
            //      - The final pan is the sum of the two pans
            //

            //
            // Zoom
            //

            mAutoFocus->CurrentAutoFocusZoom = InternalCalculateZoom(*unionAABB);

            //
            // Pan
            //

            // Calculate NDC offset required to center view onto AABB's center (net of user offsets)
            vec2f const aabbCenterNdc = mRenderContext.WorldToNdc(unionAABB->CalculateCenter(), mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            vec2f const newAutoFocusCameraPositionNdcOffset = aabbCenterNdc / 2.0f;

            // Convert back into world offset
            vec2f const newAutoFocusCameraWorldPositionOffset = mRenderContext.NdcOffsetToWorldOffset(
                vec2f(
                    newAutoFocusCameraPositionNdcOffset.x * SmoothStep(0.04f, 0.1f, std::abs(newAutoFocusCameraPositionNdcOffset.x)),    // Compress X displacement to reduce small oscillations
                    newAutoFocusCameraPositionNdcOffset.y * SmoothStep(0.04f, 0.4f, std::abs(newAutoFocusCameraPositionNdcOffset.y))),   // Compress Y displacement to reduce effect of waves
                mAutoFocus->CurrentAutoFocusZoom);

            mAutoFocus->CurrentAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionOffset;
        }

        //
        // Set zoom
        //

        mZoomParameterSmoother->SetValue(mAutoFocus->CurrentAutoFocusZoom * mAutoFocus->UserZoomOffset);

        // If we've clamped the zoom, erode lost zoom from user offset
        {
            float const impliedUserOffset = mZoomParameterSmoother->GetValue() / mAutoFocus->CurrentAutoFocusZoom;

            mAutoFocus->UserZoomOffset = Clamp(
                impliedUserOffset,
                std::min(mAutoFocus->UserZoomOffset, 1.0f),
                std::max(mAutoFocus->UserZoomOffset, 1.0f));
        }

        //
        // Set pan
        //

        // Clamp auto-focus pan
        vec2f const clampedAutoFocusPan = mRenderContext.ClampCameraWorldPosition(mAutoFocus->CurrentAutoFocusCameraWorldPosition);

        // Add user offset to clamped
        mCameraWorldPositionParameterSmoother->SetValue(clampedAutoFocusPan + mAutoFocus->UserCameraWorldPositionOffset);

        // If we've clamped the pan, erode lost panning from user offset
        {
            vec2f const impliedUserOffset = mCameraWorldPositionParameterSmoother->GetValue() - clampedAutoFocusPan;

            mAutoFocus->UserCameraWorldPositionOffset = vec2f(
                Clamp(
                    impliedUserOffset.x,
                    std::min(0.0f, mAutoFocus->UserCameraWorldPositionOffset.x),
                    std::max(0.0f, mAutoFocus->UserCameraWorldPositionOffset.x)),
                Clamp(
                    impliedUserOffset.y,
                    std::min(0.0f, mAutoFocus->UserCameraWorldPositionOffset.y),
                    std::max(0.0f, mAutoFocus->UserCameraWorldPositionOffset.y)));
        }
    }

    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}

float ViewManager::CalculateZoomParameterSmootherConvergenceFactor(float cameraSpeedAdjustment)
{
    return CalculateParameterSmootherConvergenceFactor(
        cameraSpeedAdjustment,
        0.005f,
        0.05f,
        0.2f);
}

float ViewManager::CalculateCameraWorldPositionParameterSmootherConvergenceFactor(float cameraSpeedAdjustment)
{
    return CalculateParameterSmootherConvergenceFactor(
        cameraSpeedAdjustment,
        0.005f,
        0.1f,
        0.2f);
}

float ViewManager::CalculateParameterSmootherConvergenceFactor(
    float cameraSpeedAdjustment,
    float min,
    float mid,
    float max)
{
    // SpeedAdjMin  => Min
    // SpeedAdj 1.0 => Mid
    // SpeedAdjMax  => Max

    static_assert(GetMinCameraSpeedAdjustment() < 1.0 && 1.0 < GetMaxCameraSpeedAdjustment());

    if (cameraSpeedAdjustment < 1.0f)
    {
        return
            min
            + (mid - min) * (cameraSpeedAdjustment - GetMinCameraSpeedAdjustment()) / (1.0f - GetMinCameraSpeedAdjustment());
    }
    else
    {
        return mid
            + (max - mid) * (cameraSpeedAdjustment - 1.0f) / (GetMaxCameraSpeedAdjustment() - 1.0f);

    }
}

void ViewManager::InternalFocusOnShip(Geometry::AABBSet const & allAABBs)
{
    auto const unionAABB = allAABBs.MakeUnion();
    if (unionAABB.has_value())
    {
        // Zoom
        float const newAutoFocusZoom = InternalCalculateZoom(*unionAABB);
        mZoomParameterSmoother->SetValue(newAutoFocusZoom);

        // Pan
        vec2f const newWorldCenter = unionAABB->CalculateCenter();
        mCameraWorldPositionParameterSmoother->SetValue(newWorldCenter);
    }
}

float ViewManager::InternalCalculateZoom(Geometry::AABB const & aabb)
{
    return std::min(
        std::min(
            mRenderContext.CalculateZoomForWorldWidth(aabb.GetWidth() / NdcFractionZoomTarget),
            mRenderContext.CalculateZoomForWorldHeight(aabb.GetHeight() / NdcFractionZoomTarget)),
        MaxZoom);
}
