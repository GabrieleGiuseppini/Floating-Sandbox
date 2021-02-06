/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "VisibleWorld.h"

#include <GameCore/GameMath.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <GameCore/Log.h>

#include <algorithm>

namespace Render
{

/*
 * This class encapsulates the management of view and projection parameters.
 */
class ViewModel
{
public:

    using ProjectionMatrix = float[4][4];

public:

    ViewModel(
        float zoom,
        vec2f cameraWorldPosition,
        LogicalPixelSize const & logicalCanvasSize,
        int logicalToPhysicalPixelFactor)
        : mZoom(zoom)
        , mCam(cameraWorldPosition)
        , mCanvasLogicalPixelSize(logicalCanvasSize)
        , mCanvasPhysicalPixelSize(
            logicalCanvasSize.width * logicalToPhysicalPixelFactor,
            logicalCanvasSize.height * logicalToPhysicalPixelFactor)
        , mLogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , mPixelOffsetX(0.0f)
        , mPixelOffsetY(0.0f)
    {
        // TODOTEST
        LogMessage("TODOTEST: Physical canvas size: ", mCanvasPhysicalPixelSize.width, "x", mCanvasPhysicalPixelSize.height);

        //
        // Initialize kernel ortho matrix
        //

        std::fill(
            &(mKernelOrthoMatrix[0][0]),
            &(mKernelOrthoMatrix[0][0]) + sizeof(mKernelOrthoMatrix) / sizeof(float),
            0.0f);

        mKernelOrthoMatrix[3][3] = 1.0f;

        //
        // Recalculate calculated attributes
        //

        RecalculateAttributes();
    }

    float const & GetZoom() const
    {
        return mZoom;
    }

    /*
     * Clamps the specified zoom so that the resulting view is still within the maximum world boundaries.
     */
    float ClampZoom(float const & zoom) const
    {
        float clampedZoom = zoom;

        //
        // Width
        //

        float constexpr MaxWorldLeft = -GameParameters::HalfMaxWorldWidth;
        float constexpr MaxWorldRight = GameParameters::HalfMaxWorldWidth;

        float visibleWorldWidth = CalculateVisibleWorldWidth(clampedZoom);

        if (mCam.x - visibleWorldWidth / 2.0f < MaxWorldLeft)
        {
            clampedZoom = visibleWorldWidth * clampedZoom / ((mCam.x - MaxWorldLeft) * 2.0f);
            visibleWorldWidth = CalculateVisibleWorldWidth(clampedZoom);
        }

        if (mCam.x + visibleWorldWidth / 2.0f > MaxWorldRight)
        {
            clampedZoom = visibleWorldWidth * clampedZoom / ((MaxWorldRight - mCam.x) * 2.0f);
        }

        //
        // Height
        //

        float constexpr MaxWorldTop = GameParameters::HalfMaxWorldHeight;
        float constexpr MaxWorldBottom = -GameParameters::HalfMaxWorldHeight;

        float visibleWorldHeight = CalculateVisibleWorldHeight(clampedZoom);

        if (mCam.y + visibleWorldHeight / 2.0 > MaxWorldTop)
        {
            clampedZoom = visibleWorldHeight * clampedZoom / ((MaxWorldTop - mCam.y) * 2.0f);
            visibleWorldHeight = CalculateVisibleWorldHeight(clampedZoom);
        }

        if (mCam.y - visibleWorldHeight / 2.0 < MaxWorldBottom)
        {
            clampedZoom = visibleWorldHeight * clampedZoom / ((mCam.y - MaxWorldBottom) * 2.0f);
        }

        if (clampedZoom > MaxZoom)
        {
            clampedZoom = MaxZoom;
        }

        return clampedZoom;
    }

    float const & SetZoom(float zoom)
    {
        float newZoom = ClampZoom(zoom);

        mZoom = newZoom;

        RecalculateAttributes();

        return mZoom;
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mCam;
    }

    /*
     * Clamps the specified camera position so that the resulting view is still within the maximum world boundaries.
     */
    vec2f ClampCameraWorldPosition(vec2f const & pos) const
    {
        vec2f clampedPos = pos;

        float newVisibleWorldLeft = clampedPos.x - mVisibleWorld.Width / 2.0f;
        clampedPos.x += std::max(0.0f, -GameParameters::MaxWorldWidth / 2.0f - newVisibleWorldLeft);
        float newVisibleWorldRight = clampedPos.x + mVisibleWorld.Width / 2.0f;
        clampedPos.x += std::min(0.0f, GameParameters::MaxWorldWidth / 2.0f - newVisibleWorldRight);

        float newVisibleWorldTop = clampedPos.y + mVisibleWorld.Height / 2.0f; // Top<->Positive
        clampedPos.y += std::min(0.0f, GameParameters::MaxWorldHeight / 2.0f - newVisibleWorldTop);
        float newVisibleWorldBottom = clampedPos.y - mVisibleWorld.Height / 2.0f;
        clampedPos.y += std::max(0.0f, -GameParameters::MaxWorldHeight / 2.0f - newVisibleWorldBottom);

        return clampedPos;
    }

    vec2f const & SetCameraWorldPosition(vec2f const & pos)
    {
        vec2f newPos = ClampCameraWorldPosition(pos);

        mCam = newPos;

        RecalculateAttributes();

        return mCam;
    }

    VisibleWorld const & GetVisibleWorld() const
    {
        return mVisibleWorld;
    }

    LogicalPixelSize const & GetCanvasLogicalPixelSize() const
    {
        return mCanvasLogicalPixelSize;
    }

    PhysicalPixelSize const & GetCanvasPhysicalPixelSize() const
    {
        return mCanvasPhysicalPixelSize;
    }

    float GetAspectRatio() const
    {
        return static_cast<float>(mCanvasPhysicalPixelSize.width) / static_cast<float>(mCanvasPhysicalPixelSize.height);
    }

    void SetCanvasLogicalPixelSize(LogicalPixelSize const & canvasSize)
    {
        mCanvasPhysicalPixelSize = PhysicalPixelSize(
            canvasSize.width * mLogicalToPhysicalPixelFactor,
            canvasSize.height * mLogicalToPhysicalPixelFactor);

        // Adjust zoom so that the new visible world dimensions are contained within the maximum
        SetZoom(mZoom);

        RecalculateAttributes();
    }

    void SetPixelOffset(float x, float y)
    {
        mPixelOffsetX = x;
        mPixelOffsetY = y;

        RecalculateAttributes();
    }

    void ResetPixelOffset()
    {
        mPixelOffsetX = 0.0f;
        mPixelOffsetY = 0.0f;

        RecalculateAttributes();
    }

    float GetCanvasToVisibleWorldHeightRatio() const
    {
        return mCanvasToVisibleWorldHeightRatio;
    }

    float GetCanvasWidthToHeightRatio() const
    {
        return mCanvasWidthToHeightRatio;
    }

    //
    // Coordinate transformations
    //

    /*
     * Equivalent of the transformation we usually perform in vertex shaders.
     */
    inline vec2f WorldToNdc(vec2f const & worldCoordinates) const
    {
        return vec2f(
            worldCoordinates.x * mKernelOrthoMatrix[0][0] + mKernelOrthoMatrix[3][0],
            worldCoordinates.y * mKernelOrthoMatrix[1][1] + mKernelOrthoMatrix[3][1]);
    }

    inline vec2f ScreenToWorld(LogicalPixelCoordinates const & screenCoordinates) const
    {
        return vec2f(
            Clamp(
                (static_cast<float>(screenCoordinates.x * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.width) - 0.5f) * mVisibleWorld.Width + mCam.x,
                -GameParameters::HalfMaxWorldWidth,
                GameParameters::HalfMaxWorldWidth),
            Clamp(
                (static_cast<float>(screenCoordinates.y * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.height) - 0.5f) * -mVisibleWorld.Height + mCam.y,
                -GameParameters::HalfMaxWorldHeight,
                GameParameters::HalfMaxWorldHeight));
    }

    inline vec2f ScreenOffsetToWorldOffset(LogicalPixelSize const & screenOffset) const
    {
        return vec2f(
            static_cast<float>(screenOffset.width * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.width) * mVisibleWorld.Width,
            static_cast<float>(-screenOffset.height * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.height) * mVisibleWorld.Height);
    }

    inline float PixelWidthToWorldWidth(float pixelWidth) const
    {
        // Width in NDC coordinates (between 0 and 2.0)
        float const ndcW = 2.0f * pixelWidth / static_cast<float>(mCanvasPhysicalPixelSize.width);

        // An NDC width of 2 is the entire visible world width
        return (ndcW / 2.0f) * mVisibleWorld.Width;
    }

    inline float PixelHeightToWorldHeight(float pixelHeight) const
    {
        // Height in NDC coordinates (between 0 and 2.0)
        float const ndcH = 2.0f * pixelHeight / static_cast<float>(mCanvasPhysicalPixelSize.height);

        // An NDC height of 2 is the entire visible world height
        return (ndcH / 2.0f) * mVisibleWorld.Height;
    }

    /*
     * Calculates the zoom required to ensure that the specified world
     * width is fully visible in the canvas.
     */
    inline float CalculateZoomForWorldWidth(float worldWidth) const
    {
        assert(worldWidth > 0.0f);
        return ZoomHeightConstant * GetAspectRatio() / worldWidth;
    }

    /*
     * Calculates the zoom required to ensure that the specified world
     * height is fully visible in the canvas.
     */
    inline float CalculateZoomForWorldHeight(float worldHeight) const
    {
        assert(worldHeight > 0.0f);
        return ZoomHeightConstant / worldHeight;
    }

    //
    // Projection matrixes
    //

    inline void CalculateGlobalOrthoMatrix(
        float zFar,
        float zNear,
        ProjectionMatrix & matrix) const
    {
        // Copy kernel ortho matrix
        std::copy(
            &(mKernelOrthoMatrix[0][0]),
            &(mKernelOrthoMatrix[0][0]) + sizeof(mKernelOrthoMatrix) / sizeof(float),
            &(matrix[0][0]));

        // Calculate global ortho matrix-specific cells
        matrix[2][2] = -2.0f / (zFar - zNear);
        matrix[3][2] = -(zFar + zNear) / (zFar - zNear);
    }

    inline void CalculateShipOrthoMatrix(
        float shipZRegionStart,
        float shipZRegionWidth,
        int iShip,
        int nShips,
        int maxMaxPlaneId,
        int iLayer,
        int nLayers,
        ProjectionMatrix & matrix) const
    {
        //
        // Our Z-depth strategy for ships is as follows.
        //
        // - An entire range of Z values is allocated for all the ships: from +1 (far) to -1 (near)
        //      - Range: ShipZRegionStart (far), ShipZRegionStart + ShipZRegionZWidth (near)
        // - The range is divided among all ships into equal segments
        //      - Each segment width is ShipZRegionZWidth/nShips
        // - Each ship segment is divided into sub-segments for each distinct plane ID
        //      - So a total of maxMaxPlaneId sub-segments
        //      - Lower plane ID values => nearer (z -> -1), higher plane ID values => further (z -> +1)
        // - Each plane sub-segment is divided into nLayers layers
        //


        // Copy kernel ortho matrix
        std::copy(
            &(mKernelOrthoMatrix[0][0]),
            &(mKernelOrthoMatrix[0][0]) + sizeof(mKernelOrthoMatrix) / sizeof(float),
            &(matrix[0][0]));

        //
        // Calculate Z cells: (2,2)==planeCoeff and (3,2)==planeOffset
        //
        // z' = OM(2,2)*z + OM(3,2)
        //

        // Beginning of Z range for this ship
        float const shipZStart =
            shipZRegionStart
            + shipZRegionWidth * static_cast<float>(iShip) / static_cast<float>(nShips);

        // Fractional Z value for this plane, to account for layer
        float const layerZFraction =
            shipZRegionWidth / static_cast<float>(nShips)
            * static_cast<float>(iLayer)
            / static_cast<float>(nLayers * (maxMaxPlaneId + 1));

        // Multiplier of world Z
        float const worldZMultiplier =
            shipZRegionWidth / static_cast<float>(nShips)
            / static_cast<float>(maxMaxPlaneId + 1);

        matrix[2][2] = worldZMultiplier;
        matrix[3][2] = shipZStart + layerZFraction;
    }

private:

    float CalculateVisibleWorldWidth(float zoom) const
    {
        return CalculateVisibleWorldHeight(zoom) * GetAspectRatio();
    }

    float CalculateVisibleWorldHeight(float zoom) const
    {
        assert(zoom != 0.0f);

        return ZoomHeightConstant / zoom;
    }

    void RecalculateAttributes()
    {
        mVisibleWorld.Center = mCam;

        mVisibleWorld.Width = CalculateVisibleWorldWidth(mZoom);
        mVisibleWorld.Height = CalculateVisibleWorldHeight(mZoom);

        mVisibleWorld.TopLeft = vec2f(
            mCam.x - (mVisibleWorld.Width / 2.0f),
            mCam.y + (mVisibleWorld.Height / 2.0f));
        mVisibleWorld.BottomRight = vec2f(
            mCam.x + (mVisibleWorld.Width /2.0f),
            mCam.y - (mVisibleWorld.Height / 2.0f));

        mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasPhysicalPixelSize.height) / mVisibleWorld.Height;
        mCanvasWidthToHeightRatio = static_cast<float>(mCanvasPhysicalPixelSize.width) / static_cast<float>(mCanvasPhysicalPixelSize.height);

        // Recalculate kernel Ortho Matrix cells
        mKernelOrthoMatrix[0][0] = 2.0f / mVisibleWorld.Width;
        mKernelOrthoMatrix[1][1] = 2.0f / mVisibleWorld.Height;
        mKernelOrthoMatrix[3][0] = -2.0f * (mCam.x + PixelWidthToWorldWidth(mPixelOffsetX)) / mVisibleWorld.Width;
        mKernelOrthoMatrix[3][1] = -2.0f * (mCam.y + PixelHeightToWorldHeight(mPixelOffsetY)) / mVisibleWorld.Height;
    }

private:

    // Constants
    static float constexpr MaxZoom = 1000.0f;
    static float constexpr ZoomHeightConstant = 2.0f * 70.0f; // World height at zoom=1.0

    // Primary inputs
    float mZoom;
    vec2f mCam; // World coordinates
    LogicalPixelSize mCanvasLogicalPixelSize;
    PhysicalPixelSize mCanvasPhysicalPixelSize;
    int mLogicalToPhysicalPixelFactor;
    float mPixelOffsetX;
    float mPixelOffsetY;

    // Calculated attributes
    VisibleWorld mVisibleWorld;
    float mCanvasToVisibleWorldHeightRatio;
    float mCanvasWidthToHeightRatio;
    ProjectionMatrix mKernelOrthoMatrix; // Common subset of all ortho matrices
};

}
