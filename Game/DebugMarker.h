/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-02-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderContext.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <vector>

namespace Physics
{

class DebugMarker
{
private:

    struct PointToPointArrow
    {
        PlaneId Plane;
        vec2f StartPoint;
        vec2f EndPoint;
        rgbColor Color;

        PointToPointArrow(
            PlaneId plane,
            vec2f const & startPoint,
            vec2f const & endPoint,
            rgbColor const & color)
            : Plane(plane)
            , StartPoint(startPoint)
            , EndPoint(endPoint)
            , Color(color)
        {}
    };

public:

    DebugMarker()
        : mPointToPointArrows()
        , mIsPointToPointArrowsBufferDirty(true)
    {}

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

public:

    void ClearPointToPointArrows();

    void AddPointToPointArrow(
        PlaneId planeId,
        vec2f const & startPoint,
        vec2f const & endPoint,
        rgbColor const & color)
    {
        mPointToPointArrows.emplace_back(
            planeId,
            startPoint,
            endPoint,
            color);

        mIsPointToPointArrowsBufferDirty = true;
    }

private:

    //
    // Point-to-point arrows
    //

    std::vector<PointToPointArrow> mPointToPointArrows;

    bool mutable mIsPointToPointArrowsBufferDirty;
};

}
