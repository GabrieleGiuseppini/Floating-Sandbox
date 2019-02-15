/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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
        , mCamX(cameraWorldPosition.x)
        , mCamY(cameraWorldPosition.y)
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

    void SetZoom(float zoom)
    {
        mZoom = zoom;

        RecalculateAttributes();
    }

    void AdjustZoom(float amount)
    {
        mZoom *= amount;

        RecalculateAttributes();
    }

    vec2f GetCameraWorldPosition() const
    {
        return vec2f(mCamX, mCamY);
    }

    void SetCameraWorldPosition(vec2f const & pos)
    {
        mCamX = pos.x;
        mCamY = pos.y;

        RecalculateAttributes();
    }

    void AdjustCameraWorldPosition(vec2f const & offset)
    {
        mCamX += offset.x;
        mCamY += offset.y;

        RecalculateAttributes();
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

    float GetCanvasToVisibleWorldHeightRatio() const
    {
        return mCanvasToVisibleWorldHeightRatio;
    }

    //
    // Transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates)
    {
        return vec2f(
            (screenCoordinates.x / static_cast<float>(mCanvasWidth) - 0.5f) * mVisibleWorldWidth + mCamX,
            (screenCoordinates.y / static_cast<float>(mCanvasHeight) - 0.5f) * -mVisibleWorldHeight + mCamY);
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
        //      - Lower plane ID values => further (z->+1), higher plane ID values => nearer (z->-1)
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

    void RecalculateAttributes()
    {
        mVisibleWorldHeight = 2.0f * 70.0f / (mZoom + 0.001f);
        mVisibleWorldWidth = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight;
        mCanvasToVisibleWorldHeightRatio = static_cast<float>(mCanvasHeight) / mVisibleWorldHeight;

        // Recalculate kernel Ortho Matrix cells
        mKernelOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
        mKernelOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
        mKernelOrthoMatrix[3][0] = -2.0f * mCamX / mVisibleWorldWidth;
        mKernelOrthoMatrix[3][1] = -2.0f * mCamY / mVisibleWorldHeight;
    }

private:

    // Primary inputs
    float mZoom;
    float mCamX;
    float mCamY;
    int mCanvasWidth;
    int mCanvasHeight;

    // Calculated attributes
    float mVisibleWorldWidth;
    float mVisibleWorldHeight;
    float mCanvasToVisibleWorldHeightRatio;
    ProjectionMatrix mKernelOrthoMatrix; // Common subset of all ortho matrices
};

}
