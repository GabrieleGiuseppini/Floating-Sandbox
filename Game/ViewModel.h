/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/Vectors.h>

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
        int canvasWidth,
        int canvasHeight)
        : mZoom(zoom)
        , mCam(cameraWorldPosition)
        , mCanvasWidth(canvasWidth)
        , mCanvasHeight(canvasHeight)
    {
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

    float GetZoom() const
    {
        return mZoom;
    }

    /*
     * Clamps the specified zoom so that the resulting view is still within the maximum world boundaries.
     */
    float ClampZoom(float zoom) const
    {
        float constexpr MaxWorldLeft = -GameParameters::MaxWorldWidth / 2.0f;
        float constexpr MaxWorldRight = GameParameters::MaxWorldWidth / 2.0f;

        float const visibleWorldWidth = CalculateVisibleWorldWidth(zoom);
        if (mCam.x - visibleWorldWidth / 2.0f < MaxWorldLeft)
        {
            zoom = visibleWorldWidth * zoom / ((mCam.x - MaxWorldLeft) * 2.0f);
        }
        else if (mCam.x + visibleWorldWidth / 2.0f > MaxWorldRight)
        {
            zoom = visibleWorldWidth * zoom / ((MaxWorldRight - mCam.x) * 2.0f);
        }

        float constexpr MaxWorldTop = GameParameters::MaxWorldHeight;
        float constexpr MaxWorldBottom = -GameParameters::MaxWorldHeight;

        float const visibleWorldHeight = CalculateVisibleWorldHeight(zoom);
        if (mCam.y + visibleWorldHeight / 2.0 > MaxWorldTop)
        {
            zoom = visibleWorldHeight * zoom / ((mCam.y - MaxWorldTop) * 2.0f);
        }
        else if (mCam.y - visibleWorldHeight / 2.0 < MaxWorldBottom)
        {
            zoom = visibleWorldHeight * zoom / ((MaxWorldBottom - mCam.y) * 2.0f);
        }

        return zoom;
    }

    float SetZoom(float zoom)
    {
        float newZoom = ClampZoom(zoom);

        mZoom = newZoom;

        RecalculateAttributes();

        return zoom;
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

        float newVisibleWorldLeft = clampedPos.x - mVisibleWorldWidth / 2.0f;
        clampedPos.x += std::max(0.0f, -GameParameters::MaxWorldWidth / 2.0f - newVisibleWorldLeft);
        float newVisibleWorldRight = clampedPos.x + mVisibleWorldWidth / 2.0f;
        clampedPos.x += std::min(0.0f, GameParameters::MaxWorldWidth / 2.0f - newVisibleWorldRight);

        float newVisibleWorldTop = clampedPos.y + mVisibleWorldHeight / 2.0f; // Top<->Positive
        clampedPos.y += std::min(0.0f, GameParameters::MaxWorldHeight / 2.0f - newVisibleWorldTop);
        float newVisibleWorldBottom = clampedPos.y - mVisibleWorldHeight / 2.0f;
        clampedPos.y += std::max(0.0f, -GameParameters::MaxWorldHeight / 2.0f - newVisibleWorldBottom);

        return clampedPos;
    }

    vec2f SetCameraWorldPosition(vec2f const & pos)
    {
        vec2f newPos = ClampCameraWorldPosition(pos);

        mCam = newPos;

        RecalculateAttributes();

        return newPos;
    }

    int GetCanvasWidth() const
    {
        return mCanvasWidth;
    }

    int GetCanvasHeight() const
    {
        return mCanvasHeight;
    }

    void SetCanvasSize(int width, int height)
    {
        mCanvasWidth = width;
        mCanvasHeight = height;

        // Adjust zoom so that the new visible world dimensions are contained within the maximum
        SetZoom(mZoom);

        RecalculateAttributes();
    }

    float GetVisibleWorldWidth() const
    {
        return mVisibleWorldWidth;
    }

    float GetVisibleWorldHeight() const
    {
        return mVisibleWorldHeight;
    }

    vec2f const & GetVisibleWorldTopLeft() const
    {
        return mVisibleWorldTopLeft;
    }

    vec2f const & GetVisibleWorldBottomRight() const
    {
        return mVisibleWorldBottomRight;
    }

    float GetCanvasToVisibleWorldHeightRatio() const
    {
        return mCanvasToVisibleWorldHeightRatio;
    }

    //
    // Coordinate transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates)
    {
        return vec2f(
            (screenCoordinates.x / static_cast<float>(mCanvasWidth) - 0.5f) * mVisibleWorldWidth + mCam.x,
            (screenCoordinates.y / static_cast<float>(mCanvasHeight) - 0.5f) * -mVisibleWorldHeight + mCam.y);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset)
    {
        return vec2f(
            screenOffset.x / static_cast<float>(mCanvasWidth) * mVisibleWorldWidth,
            -screenOffset.y / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight);
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
        return CalculateVisibleWorldHeight(zoom) * static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight);
    }

    float CalculateVisibleWorldHeight(float zoom) const
    {
        assert(zoom != 0.0f);

        return 2.0f * 70.0f / zoom;
    }

    void RecalculateAttributes()
    {
        mVisibleWorldWidth = CalculateVisibleWorldWidth(mZoom);
        mVisibleWorldHeight = CalculateVisibleWorldHeight(mZoom);

        mVisibleWorldTopLeft = vec2f(
            mCam.x - (mVisibleWorldWidth / 2.0f),
            mCam.y + (mVisibleWorldHeight / 2.0f));
        mVisibleWorldBottomRight = vec2f(
            mCam.x + (mVisibleWorldWidth /2.0f),
            mCam.y - (mVisibleWorldHeight / 2.0f));

        mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasHeight) / mVisibleWorldHeight;

        // Recalculate kernel Ortho Matrix cells
        mKernelOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
        mKernelOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
        mKernelOrthoMatrix[3][0] = -2.0f * mCam.x / mVisibleWorldWidth;
        mKernelOrthoMatrix[3][1] = -2.0f * mCam.y / mVisibleWorldHeight;
    }

private:

    // Primary inputs
    float mZoom;
    vec2f mCam;
    int mCanvasWidth;
    int mCanvasHeight;

    // Calculated attributes
    float mVisibleWorldWidth;
    float mVisibleWorldHeight;
    vec2f mVisibleWorldTopLeft;
    vec2f mVisibleWorldBottomRight;
    float mCanvasToVisibleWorldHeightRatio;
    ProjectionMatrix mKernelOrthoMatrix; // Common subset of all ortho matrices
};

}
