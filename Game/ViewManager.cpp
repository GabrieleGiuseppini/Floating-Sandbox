/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <GameCore/GameMath.h>

#include <cassert>

float constexpr NdcFractionZoomTarget = 0.7f; // Fraction of the [0, 2] NDC space that needs to be occupied by AABB
float constexpr AutoFocusMaxZoom = 2.0f; // Arbitrary max zoom, to avoid getting to atomic scale with e.g. Thanos

ViewManager::ViewManager(
    Render::RenderContext & renderContext,
    NotificationLayer & notificationLayer,
    GameEventDispatcher & gameEventDispatcher)
    : mRenderContext(renderContext)
    , mNotificationLayer(notificationLayer)
    , mGameEventHandler(gameEventDispatcher)
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

void ViewManager::OnNewShip(std::optional<Geometry::AABB> const & aabb)
{
    if (mDoAutoFocusOnShipLoad)
    {
        if (!mAutoFocus.has_value())
        {
            if (aabb.has_value())
            {
                InternalFocusOn(*aabb, 1.0f, 1.0f, 1.0f, 1.0f, false);
            }
        }
        else
        {
            mAutoFocus->ResetUserOffsets();
        }
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

void ViewManager::ResetView(std::optional<Geometry::AABB> const & aabb)
{
    // When continuous auto-focus is off, "view reset" focuses on ship;
    // When continuous auto-focus is on, "view reset" zeroes-out user offsets
    if (!mAutoFocus.has_value())
    {
        if (aabb)
        {
            InternalFocusOn(*aabb, 1.0f, 1.0f, 1.0f, 1.0f, false);
        }
    }
    else
    {
        mAutoFocus->ResetUserOffsets();
    }
}

void ViewManager::FocusOn(
    Geometry::AABB const & aabb,
    float widthMultiplier,
    float heightMultiplier,
    float zoomToleranceMultiplierMin,
    float zoomToleranceMultiplierMax,
    bool anchorAabbCenterAtCurrentScreenPosition)
{
    // Turn off auto-focus if it's on
    if (mAutoFocus.has_value())
    {
        mAutoFocus.reset();
        mGameEventHandler.OnContinuousAutoFocusToggled(false);
    }

    InternalFocusOn(aabb, widthMultiplier, heightMultiplier, zoomToleranceMultiplierMin, zoomToleranceMultiplierMax, anchorAabbCenterAtCurrentScreenPosition);
}

void ViewManager::Update(std::optional<Geometry::AABB> const & aabb)
{
    if (mAutoFocus.has_value())
    {
        if (aabb.has_value())
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

            mAutoFocus->CurrentAutoFocusZoom = InternalCalculateZoom(*aabb, 1.0f, 1.0f, AutoFocusMaxZoom);

            //
            // Pan
            //

            // Calculate NDC offset required to center view onto AABB's center (net of user offsets)
            vec2f const aabbCenterNdc = mRenderContext.WorldToNdc(aabb->CalculateCenter(), mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
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

void ViewManager::InternalFocusOn(
    Geometry::AABB const & aabb,
    float widthMultiplier,
    float heightMultiplier,
    float zoomToleranceMultiplierMin,
    float zoomToleranceMultiplierMax,
    bool anchorAabbCenterAtCurrentScreenPosition)
{
    // This is only called when we have no auto-focus
    assert(!mAutoFocus.has_value());

    // Calculate required zoom
    float const newAutoFocusZoom = InternalCalculateZoom(
        aabb,
        widthMultiplier,
        heightMultiplier,
        8.0f); // No closer than this

    // Check it against tolerance
    float const currentZoom = mZoomParameterSmoother->GetValue();
    if (newAutoFocusZoom < currentZoom * zoomToleranceMultiplierMin || newAutoFocusZoom > currentZoom * zoomToleranceMultiplierMax)
    {
        // Accept this zoom
        mZoomParameterSmoother->SetValue(newAutoFocusZoom);

        //
        // Pan
        //

        vec2f const aabbWorldCenter = aabb.CalculateCenter();

        vec2f newWorldCenter;
        if (anchorAabbCenterAtCurrentScreenPosition)
        {
            // Calculate new world center so that NDC coords of AABB's center now matches NDC coords
            // of it after the zoom change
            vec2f const aabbCenterNdcOffsetWrtCamera = mRenderContext.WorldToNdc(aabbWorldCenter, currentZoom, mCameraWorldPositionParameterSmoother->GetValue());
            newWorldCenter = aabbWorldCenter - mRenderContext.NdcOffsetToWorldOffset(aabbCenterNdcOffsetWrtCamera, newAutoFocusZoom);
        }
        else
        {
            // Center on AABB's center
            newWorldCenter = aabbWorldCenter;
        }

        mCameraWorldPositionParameterSmoother->SetValue(newWorldCenter);
    }
}

float ViewManager::InternalCalculateZoom(
    Geometry::AABB const & aabb,
    float widthMultiplier,
    float heightMultiplier,
    float maxZoom) const
{
    // Clamp dimensions from below to 1.0 (don't want to zoom to smaller than 1m)
    float const width = std::max(aabb.GetWidth(), 1.0f) * widthMultiplier;
    float const height = std::max(aabb.GetHeight(), 1.0f) * heightMultiplier;

    return std::min(
        std::min(
            mRenderContext.CalculateZoomForWorldWidth(width / NdcFractionZoomTarget),
            mRenderContext.CalculateZoomForWorldHeight(height / NdcFractionZoomTarget)),
        maxZoom);
}
