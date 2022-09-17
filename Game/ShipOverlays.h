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

class ShipOverlays
{
private:

    struct Center
    {
        PlaneId Plane;
        vec2f Position;

        Center(
            PlaneId plane,
            vec2f const & position)
            : Plane(plane)
            , Position(position)
        {}
    };

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

    ShipOverlays()
        : mCenters()
        , mIsCentersBufferDirty(false)
        , mPointToPointArrows()
        , mIsPointToPointArrowsBufferDirty(true)
    {}

    void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext);

public:

    void AddCenter(
        PlaneId planeId,
        vec2f const & center)
    {
        mCenters.emplace_back(
            planeId,
            center);

        mIsCentersBufferDirty = true;
    }

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
    // Centers
    //

    std::vector<Center> mCenters;

    bool mutable mIsCentersBufferDirty;

    //
    // Point-to-point arrows
    //

    std::vector<PointToPointArrow> mPointToPointArrows;

    bool mutable mIsPointToPointArrowsBufferDirty;
};

}
