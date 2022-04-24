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
            //        fall in a specific sub-window of the physical display window - not too wide and 
            //        not too narrow
            //      - User zoom is the zoom offset exherted by the user
            //      - The final zoom is the sum of the two zooms
            // - Pan:
            //      - Auto-focus pan is the pan required to ensure that the AABB is contained in a
            //        specific sub-window of the physical display window, after zoom is calculated
            //      - User pan is the pan offset exherted by the user
            //      - The final pan is the sum of the two pans
            //

            float constexpr NdcLimitMax = 0.75f; // We want things to fit inside [-NdcLimitMax, +NdcLimitMax]
            float constexpr NdcLimitMin = 0.25f; // We want things to fit outside [-NdcLimitMin, +NdcLimitMin]
            float constexpr NdcLimitAvg = (NdcLimitMin + NdcLimitMax) / 2.0f;

            // Calculate NDC coords of AABB (-1,...,+1)
            vec2f aabbNdcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());
            vec2f aabbNdcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());

            // Calculate NDC extent of AABB's extent at current ViewModel params (0,...,2)
            vec2f const aabbNdcExtent = vec2f(
                aabbNdcTopRight.x - aabbNdcBottomLeft.x,
                aabbNdcTopRight.y - aabbNdcBottomLeft.y);

            // TODOTEST

            float const autoFocusZoom = (aabbNdcExtent.x >= aabbNdcExtent.y)
                ? mRenderContext.CalculateZoomForNdcWidth(aabbNdcExtent.x / NdcLimitAvg)
                : mRenderContext.CalculateZoomForNdcHeight(aabbNdcExtent.y / NdcLimitAvg);

            mZoomParameterSmoother->SetValue(autoFocusZoom);

            // Re-calculate AABB
            aabbNdcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());
            aabbNdcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mZoomParameterSmoother->GetValue(), mCameraWorldPositionParameterSmoother->GetValue());

            vec2f const offsetWorld = vec2f(
                (aabbNdcBottomLeft.x + aabbNdcTopRight.x) / 2.0f / 2.0f * mRenderContext.GetVisibleWorld().Width,
                (aabbNdcBottomLeft.y + aabbNdcTopRight.y) / 2.0f / 2.0f * mRenderContext.GetVisibleWorld().Height);

            vec2f const newTargetCameraWorldPosition =
                mCameraWorldPositionParameterSmoother->GetValue()
                + offsetWorld;

            mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);


            /* TODOOLD
            //
            // Zoom
            //

            // Calculate NDC extent of AABB's extent at current ViewModel params (0,...,2)
            vec2f const aabbNdcExtent = vec2f(
                aabbNdcTopRight.x - aabbNdcBottomLeft.x,
                aabbNdcTopRight.y - aabbNdcBottomLeft.y);

            // See if it falls out of our target quad; if it does, adjust zoom 
            // so that the offending dimension lies exactly in-between the limits

            if (aabbNdcExtent.x > 2.0f * NdcLimitMax
                || aabbNdcExtent.y > 2.0f * NdcLimitMax)
            {
                LogMessage("  TODO: Offence: zoom excedence");

                float const autoFocusZoom = (aabbNdcExtent.x >= aabbNdcExtent.y)
                    ? mRenderContext.CalculateZoomForNdcWidth(aabbNdcExtent.x / NdcLimitAvg)
                    : mRenderContext.CalculateZoomForNdcHeight(aabbNdcExtent.y / NdcLimitAvg);

                // TODOHERE: incorporate user offset
                mZoomParameterSmoother->SetValue(autoFocusZoom);

                // Re-calculate AABB
                aabbNdcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft);
                aabbNdcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight);
            }
            else if (aabbNdcExtent.x < 2.0f * NdcLimitMin
                && aabbNdcExtent.y < 2.0f * NdcLimitMin)
            {
                LogMessage("  TODO: Offence: zoom underwhelming");

                float const autoFocusZoom = (aabbNdcExtent.x < aabbNdcExtent.y)
                    ? mRenderContext.CalculateZoomForNdcWidth(aabbNdcExtent.x / NdcLimitAvg)
                    : mRenderContext.CalculateZoomForNdcHeight(aabbNdcExtent.y / NdcLimitAvg);

                // TODOHERE: incorporate user offset
                mZoomParameterSmoother->SetValue(autoFocusZoom);

                // Re-calculate AABB
                aabbNdcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft);
                aabbNdcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight);
            }

            //
            // Pan
            //

            // We assume the AABB fits in the window, so we now take care of imbalances

            // X
            if (aabbNdcBottomLeft.x < -NdcLimitMax
                || aabbNdcTopRight.x > NdcLimitMax)
            {
                LogMessage("  TODO: Pan offence: too much left/right (extent: ", aabbNdcExtent.x, " L:", aabbNdcBottomLeft.x, " R:", aabbNdcTopRight.x, " offset:", ((aabbNdcBottomLeft.x + aabbNdcTopRight.x) / 2.0f));

                float const xOffsetWorld = 
                    (aabbNdcBottomLeft.x + aabbNdcTopRight.x) / 2.0f / 2.0f
                    * mRenderContext.GetVisibleWorld().Width;

                // TODOHERE: incorporate user offset

                vec2f const newTargetCameraWorldPosition =
                    mCameraWorldPositionParameterSmoother->GetValue()
                    + vec2f(xOffsetWorld, 0.0f);

                mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
            }

            // Y
            if (aabbNdcBottomLeft.y < -NdcLimitMax
                || aabbNdcTopRight.y > NdcLimitMax)
            {
                LogMessage("  TODO: Offence: too much above/below");

                float const yOffsetWorld =
                    (aabbNdcBottomLeft.y + aabbNdcTopRight.y) / 2.0f / 2.0f
                    * mRenderContext.GetVisibleWorld().Height;

                // TODOHERE: incorporate user offset

                vec2f const newTargetCameraWorldPosition =
                    mCameraWorldPositionParameterSmoother->GetValue()
                    + vec2f(0.0f, yOffsetWorld);

                mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
            }
            */
        }
    }

    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}
