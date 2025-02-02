/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameMath.h>
#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <algorithm>

/*
 * This class encapsulates the management of view and projection parameters.
 */
class ViewModel
{
public:

    using ProjectionMatrix = float[4][4];

public:

    ViewModel(
        FloatSize const & maxWorldSize,
        float zoom,
        vec2f cameraWorldPosition,
        DisplayLogicalSize const & logicalCanvasSize,
        int logicalToPhysicalPixelFactor)
        : mHalfMaxWorldWidth(maxWorldSize.width / 2.0f)
        , mHalfMaxWorldHeight(maxWorldSize.height / 2.0f)
        , mZoom(zoom)
        , mCam(cameraWorldPosition)
        , mCanvasLogicalSize(logicalCanvasSize)
        , mCanvasPhysicalSize(
            logicalCanvasSize.width * logicalToPhysicalPixelFactor,
            logicalCanvasSize.height * logicalToPhysicalPixelFactor)
        , mLogicalToPhysicalDisplayFactor(logicalToPhysicalPixelFactor)
        , mPixelOffsetX(0.0f)
        , mPixelOffsetY(0.0f)
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

        RecalculateAspectRatio();

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

        float const maxWorldLeft = -mHalfMaxWorldWidth;
        float const maxWorldRight = mHalfMaxWorldWidth;

        float visibleWorldWidth = CalculateVisibleWorldWidth(clampedZoom);

        if (mCam.x - visibleWorldWidth / 2.0f < maxWorldLeft)
        {
            clampedZoom = visibleWorldWidth * clampedZoom / ((mCam.x - maxWorldLeft) * 2.0f);
            visibleWorldWidth = CalculateVisibleWorldWidth(clampedZoom);
        }

        if (mCam.x + visibleWorldWidth / 2.0f > maxWorldRight)
        {
            clampedZoom = visibleWorldWidth * clampedZoom / ((maxWorldRight - mCam.x) * 2.0f);
        }

        //
        // Height
        //

        float const maxWorldTop = mHalfMaxWorldHeight;
        float const maxWorldBottom = -mHalfMaxWorldHeight;

        float visibleWorldHeight = CalculateVisibleWorldHeight(clampedZoom);

        if (mCam.y + visibleWorldHeight / 2.0 > maxWorldTop)
        {
            clampedZoom = visibleWorldHeight * clampedZoom / ((maxWorldTop - mCam.y) * 2.0f);
            visibleWorldHeight = CalculateVisibleWorldHeight(clampedZoom);
        }

        if (mCam.y - visibleWorldHeight / 2.0 < maxWorldBottom)
        {
            clampedZoom = visibleWorldHeight * clampedZoom / ((mCam.y - maxWorldBottom) * 2.0f);
        }

        if (clampedZoom > MaxZoom)
        {
            clampedZoom = MaxZoom;
        }

        return clampedZoom;
    }

    /*
     * Zoom is higher numerically when zooming in, and smaller (towards zero) when zooming out.
     */
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
        clampedPos.x += std::max(0.0f, -mHalfMaxWorldWidth - newVisibleWorldLeft);
        float newVisibleWorldRight = clampedPos.x + mVisibleWorld.Width / 2.0f;
        clampedPos.x += std::min(0.0f, mHalfMaxWorldWidth - newVisibleWorldRight);

        float newVisibleWorldTop = clampedPos.y + mVisibleWorld.Height / 2.0f; // Top<->Positive
        clampedPos.y += std::min(0.0f, mHalfMaxWorldHeight - newVisibleWorldTop);
        float newVisibleWorldBottom = clampedPos.y - mVisibleWorld.Height / 2.0f;
        clampedPos.y += std::max(0.0f, -mHalfMaxWorldHeight - newVisibleWorldBottom);

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

    float const GetHalfMaxWorldWidth() const
    {
        return mHalfMaxWorldWidth;
    }

    float const GetHalfMaxWorldHeight() const
    {
        return mHalfMaxWorldHeight;
    }

    DisplayLogicalSize const & GetCanvasLogicalSize() const
    {
        return mCanvasLogicalSize;
    }

    DisplayPhysicalSize const & GetCanvasPhysicalSize() const
    {
        return mCanvasPhysicalSize;
    }

    /*
     * Display physical width / display physical height.
     */
    float GetAspectRatio() const
    {
        return mAspectRatio;
    }

    void SetCanvasLogicalSize(DisplayLogicalSize const & canvasSize)
    {
        mCanvasLogicalSize = canvasSize;

        mCanvasPhysicalSize = DisplayPhysicalSize(
            canvasSize.width * mLogicalToPhysicalDisplayFactor,
            canvasSize.height * mLogicalToPhysicalDisplayFactor);

        RecalculateAspectRatio();

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

    float GetCanvasWidthToHeightRatio() const
    {
        return mAspectRatio;
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

    /*
     * Equivalent of the transformation we usually perform in vertex shaders.
     */
    inline vec2f WorldToNdc(
        vec2f const & worldCoordinates,
        float zoom,
        vec2f const & cameraWorldPosition) const
    {
        float const visibleWorldWidth = CalculateVisibleWorldWidth(zoom);
        float const visibleWorldHeight = CalculateVisibleWorldHeight(zoom);
        return vec2f(
            (worldCoordinates.x - cameraWorldPosition.x) * 2.0f / visibleWorldWidth,
            (worldCoordinates.y - cameraWorldPosition.y) * 2.0f / visibleWorldHeight);
    }

    /*
     * Returns the pixels in the specified world offset. Identical in any direction.
     */
    inline float WorldOffsetToPhysicalDisplayOffset(float worldOffset) const
    {
        return worldOffset * mWorldToPhysicalDisplayFactor;
    }

    inline vec2f ScreenToNdc(DisplayLogicalCoordinates const & screenCoordinates) const
    {
        vec2f const ndcCoordinates = vec2f(
            static_cast<float>(screenCoordinates.x * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.width) * 2.0f - 1.0f,
            -static_cast<float>(screenCoordinates.y * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.height) * 2.0f + 1.0f);

        return ndcCoordinates;
    }

    inline DisplayPhysicalCoordinates ScreenToPhysicalDisplay(DisplayLogicalCoordinates const & screenCoordinates) const
    {
        DisplayPhysicalCoordinates const pixelCoordinates = DisplayPhysicalCoordinates(
            screenCoordinates.x * mLogicalToPhysicalDisplayFactor,
            mCanvasPhysicalSize.height - screenCoordinates.y * mLogicalToPhysicalDisplayFactor);

        return pixelCoordinates;
    }

    inline vec2f NdcOffsetToWorldOffset(
        vec2f const & ndcOffset,
        float zoom) const
    {
        float const visibleWorldWidth = CalculateVisibleWorldWidth(zoom);
        float const visibleWorldHeight = CalculateVisibleWorldHeight(zoom);
        return vec2f(
            ndcOffset.x / 2.0f * visibleWorldWidth,
            ndcOffset.y / 2.0f * visibleWorldHeight);
    }

    inline vec2f ScreenToWorld(DisplayLogicalCoordinates const & screenCoordinates) const
    {
        vec2f const worldCoordinates = vec2f(
            Clamp(
                (static_cast<float>(screenCoordinates.x * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.width) - 0.5f) * mVisibleWorld.Width + mCam.x,
                -mHalfMaxWorldWidth,
                mHalfMaxWorldWidth),
            Clamp(
                (static_cast<float>(screenCoordinates.y * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.height) - 0.5f) * -mVisibleWorld.Height + mCam.y,
                -mHalfMaxWorldHeight,
                mHalfMaxWorldHeight));

        return worldCoordinates;
    }

    inline vec2f ScreenOffsetToWorldOffset(DisplayLogicalSize const & screenOffset) const
    {
        return vec2f(
            static_cast<float>(screenOffset.width * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.width) * mVisibleWorld.Width,
            static_cast<float>(-screenOffset.height * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.height) * mVisibleWorld.Height);
    }

    inline float ScreenOffsetToWorldOffset(int screenOffset) const
    {
        // Note: width or height is the same
        return static_cast<float>(screenOffset * mLogicalToPhysicalDisplayFactor) / static_cast<float>(mCanvasPhysicalSize.width) * mVisibleWorld.Width;
    }

    inline float ScreenFractionToWorldOffset(float screenFraction) const
    {
        // Use smallest
        return std::min(mVisibleWorld.Width, mVisibleWorld.Height) * screenFraction;
    }

    inline float ScreenFractionToPhysicalDisplay(float screenFraction) const
    {
        // Use smallest
        return static_cast<float>(std::min(mCanvasPhysicalSize.width, mCanvasPhysicalSize.height)) * screenFraction;
    }

    inline float PhysicalDisplayOffsetToWorldOffset(float pixelOffset) const
    {
        return pixelOffset / mWorldToPhysicalDisplayFactor;
    }

    /*
     * Calculates the zoom required to ensure that the specified world
     * width is fully visible in the canvas.
     */
    inline float CalculateZoomForWorldWidth(float worldWidth) const
    {
        assert(worldWidth > 0.0f);
        return ZoomHeightConstant * mAspectRatio / worldWidth;
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

    inline void UpdateShipOrthoMatrixForLayer(
        float shipZRegionStart,
        float shipZRegionWidth,
        int iShip,
        int nShips,
        int maxMaxPlaneId,
        int iLayer,
        int nLayers,
        ProjectionMatrix & matrix) const
    {
        // Beginning of Z range for this ship
        float const shipZStart =
            shipZRegionStart
            + shipZRegionWidth * static_cast<float>(iShip) / static_cast<float>(nShips);

        // Fractional Z value for this plane, to account for layer
        float const layerZFraction =
            shipZRegionWidth / static_cast<float>(nShips)
            * static_cast<float>(iLayer)
            / static_cast<float>(nLayers * (maxMaxPlaneId + 1));

        matrix[3][2] = shipZStart + layerZFraction;
    }

private:

    float CalculateVisibleWorldWidth(float zoom) const
    {
        return CalculateVisibleWorldHeight(zoom) * mAspectRatio;
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

        mWorldToPhysicalDisplayFactor = static_cast<float>(mCanvasPhysicalSize.height) / mVisibleWorld.Height;

        // Ortho Matrix: transforms world into NDC (-1, ..., +1)
        //
        //  2 / WrdW            0                   0                0
        //  0                   2 / WrdH            0                0
        //  0                   0                   WrdZMult         0
        //  -2 * CamX / WrdW    -2 * CamY / WrdH    ZOffset          1

        // Recalculate kernel Ortho Matrix cells
        mKernelOrthoMatrix[0][0] = 2.0f / mVisibleWorld.Width;
        mKernelOrthoMatrix[1][1] = 2.0f / mVisibleWorld.Height;
        mKernelOrthoMatrix[3][0] = -2.0f * (mCam.x + (mPixelOffsetX / mWorldToPhysicalDisplayFactor)) / mVisibleWorld.Width;
        mKernelOrthoMatrix[3][1] = -2.0f * (mCam.y + (mPixelOffsetY / mWorldToPhysicalDisplayFactor)) / mVisibleWorld.Height;
    }

    void RecalculateAspectRatio()
    {
        mAspectRatio = static_cast<float>(mCanvasPhysicalSize.width) / static_cast<float>(mCanvasPhysicalSize.height);
    }

private:

    // Constants
    static float constexpr MaxZoom = 100.0f;
    static float constexpr ZoomHeightConstant = 2.0f * 70.0f; // World height at zoom=1.0

    // Primary inputs
    float const mHalfMaxWorldWidth;
    float const mHalfMaxWorldHeight;
    float mZoom;
    vec2f mCam; // World coordinates
    DisplayLogicalSize mCanvasLogicalSize;
    DisplayPhysicalSize mCanvasPhysicalSize;
    int mLogicalToPhysicalDisplayFactor;
    float mPixelOffsetX;
    float mPixelOffsetY;

    // Calculated attributes
    float mAspectRatio;
    VisibleWorld mVisibleWorld;
    float mWorldToPhysicalDisplayFactor;
    ProjectionMatrix mKernelOrthoMatrix; // Common subset of all ortho matrices
};
