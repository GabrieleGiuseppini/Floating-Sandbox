/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <GameCore/GameMath.h>
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
    mAutoFocus.emplace(
        mZoomParameterSmoother->GetValue(),
        mCameraWorldPositionParameterSmoother->GetValue());
}

void ViewManager::OnViewModelUpdated()
{
    // Pickup eventual changes to view model properties
    mZoomParameterSmoother->SetValueImmediate(mRenderContext.GetZoom());
    mCameraWorldPositionParameterSmoother->SetValueImmediate(mRenderContext.GetCameraWorldPosition());
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

void ViewManager::PanImmediate(vec2f const & worldOffset)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition =
            mCameraWorldPositionParameterSmoother->GetValue()
            + worldOffset;

        mCameraWorldPositionParameterSmoother->SetValueImmediate(newTargetCameraWorldPosition);
    }
    else
    {
        // Note: not at all "immediate"
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

void ViewManager::ResetView()
{
    if (!mAutoFocus.has_value())
    {
        mCameraWorldPositionParameterSmoother->SetValueImmediate(vec2f::zero());
        mZoomParameterSmoother->SetValueImmediate(1.0f);
    }
    else
    {
        mAutoFocus->ResetUserOffsets();
    }
}

void ViewManager::TODODoAutoZoom(Geometry::AABBSet const & allAABBs)
{
    // TODO
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

            // Calculate needed zoom to fit in fraction of NDC space - choosing the furthest among the two dimensions
            float newAutoFocusZoom = std::min(
                mRenderContext.CalculateZoomForWorldWidth(unionAABB->GetWidth() / NdcFractionZoomTarget),
                mRenderContext.CalculateZoomForWorldHeight(unionAABB->GetHeight() / NdcFractionZoomTarget));

            // Check change against threshold
            if (std::abs(newAutoFocusZoom - mAutoFocus->CurrentAutoFocusZoom) > ZoomThreshold)
            {
                // Apply auto-focus
                mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;
            }
            else
            {
                // Stay
                newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom;
            }
            
            // Set zoom
            mZoomParameterSmoother->SetValue(newAutoFocusZoom * mAutoFocus->UserZoomOffset);

            // If we've clamped the zoom, erode lost zoom from user offset
            {
                float const impliedUserOffset = mZoomParameterSmoother->GetValue() / newAutoFocusZoom;

                mAutoFocus->UserZoomOffset = Clamp(
                    impliedUserOffset,
                    std::min(mAutoFocus->UserZoomOffset, 1.0f),
                    std::max(mAutoFocus->UserZoomOffset, 1.0f));
            }

            //
            // Pan
            //

            //float constexpr CameraPositionThreshold = 30.0f; // Attempt to avoid panning with waves
            float constexpr CameraPositionThreshold = 0.0f;

            // Calculate world offset required to center view onto AABB's center
            vec2f const aabbCenterWorld = unionAABB->CalculateCenter();
            vec2f const newAutoFocusCameraWorldPositionOffset = (aabbCenterWorld - mAutoFocus->CurrentAutoFocusCameraWorldPosition) / 2.0f;

            // Check change against threshold
            vec2f newAutoFocusCameraWorldPosition;
            if (std::abs(newAutoFocusCameraWorldPositionOffset.x) > CameraPositionThreshold
                || std::abs(newAutoFocusCameraWorldPositionOffset.y) > CameraPositionThreshold)
            {
                // Apply auto-focus
                newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionOffset;
                mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;
            }
            else
            {
                // Stay
                newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition;
            }
            
            // Set camera position
            mCameraWorldPositionParameterSmoother->SetValue(newAutoFocusCameraWorldPosition + mAutoFocus->UserCameraWorldPositionOffset);

            // If we've clamped the pan, erode lost panning from user offset
            {
                vec2f const impliedUserOffset = mCameraWorldPositionParameterSmoother->GetValue() - newAutoFocusCameraWorldPosition;

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
    }

    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}
