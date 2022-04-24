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
    void ResetPan();
    void AdjustZoom(float amount);
    void ResetZoom();
    void TODODoAutoZoom(Geometry::AABBSet const & allAABBs);

    void Update();

private:

    Render::RenderContext & mRenderContext;

    std::unique_ptr<ParameterSmoother<float>> mZoomParameterSmoother;
    std::unique_ptr<ParameterSmoother<vec2f>> mCameraWorldPositionParameterSmoother;
};
