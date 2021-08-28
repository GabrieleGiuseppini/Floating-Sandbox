/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <algorithm>

namespace ShipBuilder {

/*
 * This class maintains the logic for transformations between the various coordinates.
 *
 * Terminology:
 *  - WorkSpace: has the pixel size of the structure (equivalent of ::World)
 *  - WorkSpaceCoordinates
 *  - DisplayLogical: has the logical display (window) size (equivalent of ::Logical)
 *  - DisplayLogicalCoordinates: the logical display coordinates (type @ ShipBuilderTypes)
 *  - DisplayPixel: has the pixel display (window) size (equivalent of ::Pixel)
 *  - DisplayPixelCoordinates: the pixel display coordinates (type @ ShipBuilderTypes)
 */
class ViewModel
{
public:

    using ProjectionMatrix = float[4][4];

public:

    ViewModel(
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor)
        : mZoom(1.0f)
        , mCam(0.0f, 0.0f)
        , mLogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , mDisplayLogicalSize(initialDisplaySize)
        , mDisplayPhysicalSize(
            initialDisplaySize.width * logicalToPhysicalPixelFactor,
            initialDisplaySize.height * logicalToPhysicalPixelFactor)
    {
        //
        // Initialize ortho matrix
        //

        std::fill(
            &(mOrthoMatrix[0][0]),
            &(mOrthoMatrix[0][0]) + sizeof(mOrthoMatrix) / sizeof(float),
            0.0f);

        mOrthoMatrix[3][3] = 1.0f;

        //
        // Calculate all attributes based on current view
        //

        RecalculateAttributes();
    }

    DisplayPhysicalSize const & GetDisplayPhysicalSize() const
    {
        return mDisplayPhysicalSize;
    }

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mDisplayLogicalSize = logicalSize;

        mDisplayPhysicalSize = DisplayPhysicalSize(
            logicalSize.width * mLogicalToPhysicalPixelFactor,
            logicalSize.height * mLogicalToPhysicalPixelFactor);

        // TODO
        //// Adjust zoom so that the new visible world dimensions are contained within the maximum
        ////SetZoom(mZoom);

        RecalculateAttributes();
    }

    ProjectionMatrix const & GetOrthoMatrix() const
    {
        return mOrthoMatrix;
    }

    WorkSpaceCoordinates DisplayToWorkSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        // TODOHERE
        /*
        vec2f const worldCoordinates = vec2f(
            Clamp(
                (static_cast<float>(screenCoordinates.x * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.width) - 0.5f) * mVisibleWorld.Width + mCam.x,
                -GameParameters::HalfMaxWorldWidth,
                GameParameters::HalfMaxWorldWidth),
            Clamp(
                (static_cast<float>(screenCoordinates.y * mLogicalToPhysicalPixelFactor) / static_cast<float>(mCanvasPhysicalPixelSize.height) - 0.5f) * -mVisibleWorld.Height + mCam.y,
                -GameParameters::HalfMaxWorldHeight,
                GameParameters::HalfMaxWorldHeight));

        return worldCoordinates;
        */
        return WorkSpaceCoordinates(displayCoordinates.x, displayCoordinates.y);
    }

private:

    void RecalculateAttributes()
    {
        // Ortho Matrix:
        // TODO: zoom is here not taken into account, and we use display logical size in lieu of work space
        //
        //  2 / WkW                0                      0                0
        //  0                      2 / WkH                0                0
        //  0                      0                      0                0
        //  -2 * CamX / WkW - 1    -2 * CamY / WkH - 1    0                1

        // Recalculate Ortho Matrix cells (r, c)
        mOrthoMatrix[0][0] = 2.0f / static_cast<float>(mDisplayLogicalSize.width);
        mOrthoMatrix[1][1] = 2.0f / static_cast<float>(mDisplayLogicalSize.height);
        mOrthoMatrix[3][0] = -2.0f * mCam.x / static_cast<float>(mDisplayLogicalSize.width) - 1.0f;
        mOrthoMatrix[3][1] = -2.0f * mCam.y / static_cast<float>(mDisplayLogicalSize.height) - 1.0f;
    }

private:

    // Constants
    static float constexpr MaxZoom = 1000.0f;

    // Primary inputs
    float mZoom; // How many display pixels are occupied by one work space pixel; always >= 1
    vec2f mCam; // Work space coordinates of of work pixel that is visible at (0, 0) in display
    int const mLogicalToPhysicalPixelFactor;
    DisplayLogicalSize mDisplayLogicalSize;
    DisplayPhysicalSize mDisplayPhysicalSize;

    // Calculated attributes
    ProjectionMatrix mOrthoMatrix;
};

}