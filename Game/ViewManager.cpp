/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <GameCore/Log.h>

#include <cassert>

ViewManager::ViewManager(Render::RenderContext & renderContext)
    : mRenderContext(renderContext)
{
    float constexpr ControlParameterConvergenceFactor = 0.05f;

    mZoomParameterSmoother = std::make_unique<ParameterSmoother<float>>(
        [this]() -> float const &
        {
            return mRenderContext.GetZoom();
        },
        [this](float const & value) -> float const &
        {
            return mRenderContext.SetZoom(value);
        },
        [this](float const & value)
        {
            return mRenderContext.ClampZoom(value);
        },
        ControlParameterConvergenceFactor);

    mCameraWorldPositionParameterSmoother = std::make_unique<ParameterSmoother<vec2f>>(
        [this]() -> vec2f const &
        {
            return mRenderContext.GetCameraWorldPosition();
        },
        [this](vec2f const & value) -> vec2f const &
        {
            return mRenderContext.SetCameraWorldPosition(value);
        },
        [this](vec2f const & value)
        {
            return mRenderContext.ClampCameraWorldPosition(value);
        },
        ControlParameterConvergenceFactor);

    // TODOTEST
    mAutoFocus.emplace();
}

void ViewManager::OnViewModelUpdated()
{
    // Pickup eventual changes to view model properties
    mZoomParameterSmoother->SetValueImmediate(mRenderContext.GetZoom());
    mCameraWorldPositionParameterSmoother->SetValueImmediate(mRenderContext.GetCameraWorldPosition());
}

void ViewManager::Pan(vec2f const & worldOffset)
{
    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

    mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
}

void ViewManager::PanImmediate(vec2f const & worldOffset)
{
    vec2f const newTargetCameraWorldPosition =
        mCameraWorldPositionParameterSmoother->GetValue()
        + worldOffset;

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void ViewManager::PanToWorldX(float worldX)
{
    vec2f const newTargetCameraWorldPosition = vec2f(
        worldX,
        mCameraWorldPositionParameterSmoother->GetValue().y);

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void ViewManager::ResetPan()
{
    vec2f const newTargetCameraWorldPosition = vec2f(0, 0);

    mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
}

void ViewManager::AdjustZoom(float amount)
{
    float const newTargetZoom = mZoomParameterSmoother->GetValue() * amount;

    mZoomParameterSmoother->SetValue(newTargetZoom);
}

void ViewManager::ResetZoom()
{
    float const newTargetZoom = 1.0f;

    mZoomParameterSmoother->SetValueImmediate(newTargetZoom);
}

void ViewManager::TODODoAutoZoom(Geometry::AABBSet const & allAABBs)
{
    auto const unionAABB = allAABBs.MakeUnion();
    if (unionAABB.has_value())
    {
        // Calculate zoom to fit width and height (plus a nicely-looking margin)
        float const newZoom = std::min(
            mRenderContext.CalculateZoomForWorldWidth(unionAABB->GetWidth() + 5.0f),
            mRenderContext.CalculateZoomForWorldHeight(unionAABB->GetHeight() + 3.0f));

        // If calculated zoom requires zoom out: use it
        if (newZoom <= mZoomParameterSmoother->GetValue())
        {
            mZoomParameterSmoother->SetValueImmediate(newZoom);
        }
        else if (newZoom > 1.0f)
        {
            // Would need to zoom-in closer...
            // ...let's stop at 1.0 then
            mZoomParameterSmoother->SetValueImmediate(1.0f);
        }
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

            float constexpr NdcFractionZoomTarget = 0.5f; // Fraction of the [0, 2] NDC space that needs to be occupied by AABB

            //float constexpr ZoomThreshold = 0.1f;
            float constexpr ZoomThreshold = 0.0f;

            // Calculate NDC extent of AABB's extent at current ViewModel params [0, 2]
            vec2f const aabbNdcExtent = mRenderContext.WorldOffsetToNdc(
                vec2f(unionAABB->GetWidth(), unionAABB->GetHeight()),
                mZoomParameterSmoother->GetValue());

            // Calculate needed zoom to fit in fraction of NDC space - choosing the furthest among the two dimensions
            float const visibleWorldWidth = mRenderContext.CalculateVisibleWorldWidth(mZoomParameterSmoother->GetValue());
            float const visibleWorldHeight = mRenderContext.CalculateVisibleWorldHeight(mZoomParameterSmoother->GetValue());
            float const autoFocusZoom = std::min(
                mRenderContext.CalculateZoomForWorldWidth(aabbNdcExtent.x / NdcFractionZoomTarget * visibleWorldWidth / 2.0f),
                mRenderContext.CalculateZoomForWorldHeight(aabbNdcExtent.y / NdcFractionZoomTarget * visibleWorldHeight / 2.0f));

            // Set zoom, with threshold check

            float newAutoFocusZoom;
            float const oldZoom = mZoomParameterSmoother->GetValue();
            if (std::abs(autoFocusZoom - oldZoom) > ZoomThreshold)
            {
                // Apply auto-focus
                newAutoFocusZoom = autoFocusZoom;
            }
            else
            {
                // Stay
                newAutoFocusZoom = oldZoom;
            }
            
            mZoomParameterSmoother->SetValue(newAutoFocusZoom);

            //
            // Pan
            //

            //float constexpr CameraPositionThreshold = 30.0f; // Attempt to avoid panning with waves
            float constexpr CameraPositionThreshold = 0.0f;
            
            // Calculate AABB in NDC coordinates, using the target zoom
            vec2f const aabbNdcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());
            vec2f const aabbNdcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());

            // Calculate world offset required to center view onto AABB's center
            vec2f const autoFocusCameraWorldPositionOffset = vec2f(
                (aabbNdcBottomLeft.x + aabbNdcTopRight.x) / 2.0f * mRenderContext.CalculateVisibleWorldWidth(mZoomParameterSmoother->GetValue()) / 2.0f,
                (aabbNdcBottomLeft.y + aabbNdcTopRight.y) / 2.0f * mRenderContext.CalculateVisibleWorldHeight(mZoomParameterSmoother->GetValue()) / 2.0f);

            // Set camera position, with threshold check

            vec2f newAutoFocusCameraWorldPosition;
            if (std::abs(autoFocusCameraWorldPositionOffset.x) > CameraPositionThreshold
                || std::abs(autoFocusCameraWorldPositionOffset.y) > CameraPositionThreshold)
            {
                // Apply auto-focus
                newAutoFocusCameraWorldPosition = mCameraWorldPositionParameterSmoother->GetValue() + autoFocusCameraWorldPositionOffset;
            }
            else
            {
                // Stay
                newAutoFocusCameraWorldPosition = mCameraWorldPositionParameterSmoother->GetValue();
            }
            
            mCameraWorldPositionParameterSmoother->SetValue(newAutoFocusCameraWorldPosition);
        }
    }

    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}
