/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <Core/GameMath.h>

#include <cassert>

float constexpr NdcFractionZoomTarget = 0.7f; // Fraction of the [0, 2] NDC space that needs to be occupied by AABB

float constexpr SmootherTerminationThreshold = 0.00005f; // How close to target we stop smoothing

ViewManager::ViewManager(RenderContext & renderContext)
    : mRenderContext(renderContext)
    // Defaults
    , mCameraSpeedAdjustment(1.0f)
    , mDoAutoFocusOnShipLoad(true)
    , mDoAutoFocusOnNpcPlacement(false)
    //
    , mInverseZoomParameterSmoother(
        [this]() -> float
        {
            return 1.0f / mRenderContext.GetZoom();
        },
        [this](float const & value) -> float
        {
            return 1.0f / mRenderContext.SetZoom(1.0f / value);
        },
        [this](float const & value) -> float
        {
            return 1.0f / mRenderContext.ClampZoom(1.0f / value);
        },
        CalculateParameterSmootherConvergenceFactor(mCameraSpeedAdjustment),
        SmootherTerminationThreshold)
    , mCameraWorldPositionParameterSmoother(
        [this]() -> vec2f
        {
            return mRenderContext.GetCameraWorldPosition();
        },
        [this](vec2f const & value) -> vec2f
        {
            return mRenderContext.SetCameraWorldPosition(value);
        },
        [this](vec2f const & value) -> vec2f
        {
            return mRenderContext.ClampCameraWorldPosition(value);
        },
        CalculateParameterSmootherConvergenceFactor(mCameraSpeedAdjustment),
        SmootherTerminationThreshold)
    , mCameraWorldPositionParameterSmootherContingentMultiplier(1.0f)
    //
    , mAutoFocus() // Set later
{
    // Default: continuous auto-focus is ON on ships
    SetAutoFocusTarget(AutoFocusTargetKindType::Ship);
}

float ViewManager::GetCameraSpeedAdjustment() const
{
    return mCameraSpeedAdjustment;
}

void ViewManager::SetCameraSpeedAdjustment(float value)
{
    mCameraSpeedAdjustment = value;

    float const convergenceFactor = CalculateParameterSmootherConvergenceFactor(mCameraSpeedAdjustment);

    mInverseZoomParameterSmoother.SetConvergenceFactor(convergenceFactor);
    mCameraWorldPositionParameterSmoother.SetConvergenceFactor(convergenceFactor);
}

bool ViewManager::GetDoAutoFocusOnShipLoad() const
{
    return mDoAutoFocusOnShipLoad;
}

void ViewManager::SetDoAutoFocusOnShipLoad(bool value)
{
    mDoAutoFocusOnShipLoad = value;
}

bool ViewManager::GetDoAutoFocusOnNpcPlacement() const
{
    return mDoAutoFocusOnNpcPlacement;
}

void ViewManager::SetDoAutoFocusOnNpcPlacement(bool value)
{
    mDoAutoFocusOnNpcPlacement = value;
}

std::optional<AutoFocusTargetKindType> ViewManager::GetAutoFocusTarget() const
{
    if (!mAutoFocus.has_value())
        return std::nullopt;
    else
        return mAutoFocus->AutoFocusTarget;
}

void ViewManager::SetAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & target)
{
    if (target.has_value())
    {
        // Start / Reset auto-focus
        mAutoFocus.emplace(
            target,
            1.0f / mInverseZoomParameterSmoother.GetValue(),
            mCameraWorldPositionParameterSmoother.GetValue());
    }
    else
    {
        // Stop auto-focus
        mAutoFocus.reset();
    }
}

void ViewManager::OnViewModelUpdated()
{
    // Pickup eventual changes to view model constraints
    mInverseZoomParameterSmoother.ReClamp();
    mInverseZoomParameterSmoother.ReClamp();
}

void ViewManager::Pan(vec2f const & worldOffset)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition =
            mCameraWorldPositionParameterSmoother.GetValue()
            + worldOffset;

        mCameraWorldPositionParameterSmoother.SetValue(newTargetCameraWorldPosition);
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
            mCameraWorldPositionParameterSmoother.GetValue().y);

        mCameraWorldPositionParameterSmoother.SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset.x = worldX;
    }
}

void ViewManager::PanToWorldY(float worldY)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition = vec2f(
            mCameraWorldPositionParameterSmoother.GetValue().x,
            worldY);

        mCameraWorldPositionParameterSmoother.SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset.y = worldY;
    }
}

void ViewManager::AdjustZoom(float amount)
{
    if (!mAutoFocus.has_value())
    {
        float const newTargetZoom = (1.0f / mInverseZoomParameterSmoother.GetValue()) * amount;

        mInverseZoomParameterSmoother.SetValue(1.0f / newTargetZoom);
    }
    else
    {
        mAutoFocus->UserZoomOffset *= amount;
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
    // One-shot

    // Invoked when there's no auto-focus
    assert(!mAutoFocus.has_value());

    InternalFocusOn(aabb, widthMultiplier, heightMultiplier, zoomToleranceMultiplierMin, zoomToleranceMultiplierMax, anchorAabbCenterAtCurrentScreenPosition);
}

void ViewManager::UpdateAutoFocus(std::optional<Geometry::AABB> const & aabb)
{
    // Invoked when auto-focus is enabled
    assert(mAutoFocus.has_value());

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

        mAutoFocus->CurrentAutoFocusZoom = InternalCalculateZoom(*aabb, 1.0f, 1.0f, mAutoFocus->AutoFocusTarget);

        //
        // Pan
        //

        // Calculate NDC offset required to center view onto AABB's center (net of user offsets)
        vec2f const aabbCenterNdc = mRenderContext.WorldToNdc(aabb->CalculateCenter(), mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
        vec2f const newAutoFocusCameraPositionNdcOffset = aabbCenterNdc;

        // Convert back into world offset
        vec2f const newAutoFocusCameraWorldPositionOffset = mRenderContext.NdcOffsetToWorldOffset(
            vec2f(
                newAutoFocusCameraPositionNdcOffset.x * SmoothStep(0.04f, 0.1f, std::abs(newAutoFocusCameraPositionNdcOffset.x)),    // Compress X displacement to reduce small oscillations
                newAutoFocusCameraPositionNdcOffset.y * SmoothStep(0.04f, 0.4f, std::abs(newAutoFocusCameraPositionNdcOffset.y))),   // Compress Y displacement to reduce effect of waves
            mAutoFocus->CurrentAutoFocusZoom);

        mAutoFocus->CurrentAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionOffset;

        // MayBeFuturework: turned off as it generates non-linear moves
        //// Calc speed multiplier:
        //// |ndcOffset|=0 -> 1.0
        //// |ndcOffset|=sqrt(2) -> 3.0
        ////mCameraWorldPositionParameterSmootherContingentMultiplier = 1.0f + 2.0f * std::min(newAutoFocusCameraPositionNdcOffset.length() / 1.4142f, 1.0f);
    }

    //
    // Set zoom
    //

    mInverseZoomParameterSmoother.SetValue(1.0f / (mAutoFocus->CurrentAutoFocusZoom * mAutoFocus->UserZoomOffset));

    // If we've clamped the zoom, erode lost zoom from user offset
    {
        float const impliedUserOffset = (1.0f / mInverseZoomParameterSmoother.GetValue()) / mAutoFocus->CurrentAutoFocusZoom;

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
    mCameraWorldPositionParameterSmoother.SetValue(clampedAutoFocusPan + mAutoFocus->UserCameraWorldPositionOffset);

    // If we've clamped the pan, erode lost panning from user offset
    {
        vec2f const impliedUserOffset = mCameraWorldPositionParameterSmoother.GetValue() - clampedAutoFocusPan;

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

void ViewManager::Update()
{
    // Invoked at each step
    mInverseZoomParameterSmoother.Update();
    mCameraWorldPositionParameterSmoother.Update(mCameraWorldPositionParameterSmootherContingentMultiplier);
    mCameraWorldPositionParameterSmootherContingentMultiplier = 1.0f; // Reset contingent multiplier
}

void ViewManager::ResetAutoFocusAlterations()
{
    assert(mAutoFocus.has_value());
    mAutoFocus->ResetUserOffsets();
}

///////////////////////////////////////////////////////////

float ViewManager::CalculateParameterSmootherConvergenceFactor(float cameraSpeedAdjustment)
{
    // SpeedAdjMin  => Min
    // SpeedAdj 1.0 => Mid
    // SpeedAdjMax  => Max

    float constexpr Min = 0.005f;
    float constexpr Mid = 0.05f;
    float constexpr Max = 0.2f;

    static_assert(GetMinCameraSpeedAdjustment() < 1.0 && 1.0 < GetMaxCameraSpeedAdjustment());

    if (cameraSpeedAdjustment < 1.0f)
    {
        return
            Min
            + (Mid - Min) * (cameraSpeedAdjustment - GetMinCameraSpeedAdjustment()) / (1.0f - GetMinCameraSpeedAdjustment());
    }
    else
    {
        return Mid
            + (Max - Mid) * (cameraSpeedAdjustment - 1.0f) / (GetMaxCameraSpeedAdjustment() - 1.0f);

    }
}

float ViewManager::CalculateAutoFocusMaxZoom(std::optional<AutoFocusTargetKindType> targetKind)
{
    if (targetKind == AutoFocusTargetKindType::Ship)
    {
        return 2.0f; // Arbitrary max zoom, to avoid getting to atomic scale with e.g. Thanos
    }
    else if (targetKind == AutoFocusTargetKindType::SelectedNpc)
    {
        return 8.0f; // Arbitrary max zoom
    }
    else
    {
        assert(!targetKind.has_value());
        return 8.0f; // Arbitrary
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
    // One-shot

    /*
     * Focuses on the specified AABB's center, moving the camera smoothly to it.
     * If anchoring, the movement ensures that the current AABB's center's position
     * stays at the same screen position as now; in this case, the focus might
     * be aborted if the movement would require a clamp - and thus a break of the
     * "anchoring" promise.
     */

    // This is only called when we have no auto-focus
    assert(!mAutoFocus.has_value());

    //
    // Calculate zoom
    //

    float const newAutoFocusZoom = InternalCalculateZoom(
        aabb,
        widthMultiplier,
        heightMultiplier,
        std::nullopt); // No closer than this

    // Check zoom against tolerance
    float const currentZoom = 1.0f / mInverseZoomParameterSmoother.GetValue();
    assert(currentZoom * zoomToleranceMultiplierMin <= currentZoom * zoomToleranceMultiplierMax);
    if (newAutoFocusZoom >= currentZoom * zoomToleranceMultiplierMin && newAutoFocusZoom <= currentZoom * zoomToleranceMultiplierMax)
    {
        // Doesn't pass tolerance
        return;
    }

    // Check if zoom needs to be clamped
    if (anchorAabbCenterAtCurrentScreenPosition
        && mRenderContext.ClampZoom(newAutoFocusZoom) != newAutoFocusZoom)
    {
        // Was clamped, can't continue
        return;
    }

    //
    // Calculate pan
    //

    vec2f const aabbWorldCenter = aabb.CalculateCenter();
    vec2f newWorldCenter;
    if (anchorAabbCenterAtCurrentScreenPosition)
    {
        // Calculate new world center so that NDC coords of AABB's center now matches NDC coords
        // of it after the zoom change
        vec2f const aabbCenterNdcOffsetWrtCamera = mRenderContext.WorldToNdc(aabbWorldCenter, currentZoom, mCameraWorldPositionParameterSmoother.GetValue());
        newWorldCenter = aabbWorldCenter - mRenderContext.NdcOffsetToWorldOffset(aabbCenterNdcOffsetWrtCamera, newAutoFocusZoom);

        // Check if pan needs to be clamped
        if (mRenderContext.ClampCameraWorldPosition(newWorldCenter) != newWorldCenter)
        {
            // Was clamped, can't continue
            return;
        }
    }
    else
    {
        // Center on AABB's center
        newWorldCenter = aabbWorldCenter;
    }

    //
    // Apply zoom and pan
    //

    mInverseZoomParameterSmoother.SetValue(1.0f / newAutoFocusZoom);
    mCameraWorldPositionParameterSmoother.SetValue(newWorldCenter);
}

float ViewManager::InternalCalculateZoom(
    Geometry::AABB const & aabb,
    float widthMultiplier,
    float heightMultiplier,
    std::optional<AutoFocusTargetKindType> targetKind) const
{
    // Clamp dimensions from below to 1.0 (don't want to zoom to smaller than 1m)
    float const width = std::max(aabb.GetWidth(), 1.0f) * widthMultiplier;
    float const height = std::max(aabb.GetHeight(), 1.0f) * heightMultiplier;

    // Calculate max zoom
    float const maxZoom = CalculateAutoFocusMaxZoom(targetKind);

    return std::min(
        std::min(
            mRenderContext.CalculateZoomForWorldWidth(width / NdcFractionZoomTarget),
            mRenderContext.CalculateZoomForWorldHeight(height / NdcFractionZoomTarget)),
        maxZoom);
}
