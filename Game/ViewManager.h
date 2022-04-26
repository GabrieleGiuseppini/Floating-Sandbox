/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderContext.h"

#include <GameCore/AABBSet.h>
#include <GameCore/ParameterSmoother.h>
#include <GameCore/Vectors.h>

#include <memory>
#include <optional>

class ViewManager
{
public:

    ViewManager(Render::RenderContext & renderContext);

    void OnViewModelUpdated();
    void Pan(vec2f const & worldOffset);
    void PanImmediate(vec2f const & worldOffset);
    void PanToWorldX(float worldX);
    void AdjustZoom(float amount);
    void ResetView();
    void TODODoAutoZoom(Geometry::AABBSet const & allAABBs);

    void Update(Geometry::AABBSet const & allAABBs);

private:

    Render::RenderContext & mRenderContext;

    std::unique_ptr<ParameterSmoother<float>> mZoomParameterSmoother;
    std::unique_ptr<ParameterSmoother<vec2f>> mCameraWorldPositionParameterSmoother;

    //
    // Auto-focus
    //

    struct AutoFocusSessionData
    {
        float CurrentAutoFocusZoom;
        vec2f CurrentAutoFocusCameraWorldPosition;

        float UserZoomOffset;
        vec2f UserCameraWorldPositionOffset;

        AutoFocusSessionData(
            float currentAutoFocusZoom,
            vec2f const & currentAutoFocusCameraWorldPosition)
            : CurrentAutoFocusZoom(currentAutoFocusZoom)
            , CurrentAutoFocusCameraWorldPosition(currentAutoFocusCameraWorldPosition)
        {
            ResetUserOffsets();
        }

        void ResetUserOffsets()
        {
            UserZoomOffset = 1.0f;
            UserCameraWorldPositionOffset = vec2f::zero();
        }
    };

    std::optional<AutoFocusSessionData> mAutoFocus;
};
