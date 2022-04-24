/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

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

void ViewManager::Update()
{
    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}
