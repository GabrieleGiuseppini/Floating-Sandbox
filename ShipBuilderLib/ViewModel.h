/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <algorithm>
#include <cmath>

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
        : mZoom(1)
        , mCam(0, 0)
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

    int GetZoom() const
    {
        return mZoom;
    }

    int SetZoom(int zoom)
    {
        mZoom = Clamp(zoom, MinZoom, MaxZoom);

        RecalculateAttributes();

        return mZoom;
    }

    WorkSpaceCoordinates const & GetCameraWorkSpacePosition() const
    {
        return mCam;
    }

    WorkSpaceCoordinates const & SetCameraWorkSpacePosition(WorkSpaceCoordinates const & pos)
    {
        mCam = pos;

        RecalculateAttributes();

        return mCam;
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

        RecalculateAttributes();
    }

    WorkSpaceCoordinates GetVisibleWorkSpaceOrigin() const
    {
        // TODOHERE
        return WorkSpaceCoordinates(0, 0);
    }

    WorkSpaceSize GetVisibleWorkSpaceSize() const
    {
        // TODOHERE
        return WorkSpaceSize(10, 10);
    }

    //
    // Coordinate transformations
    //

    WorkSpaceCoordinates DisplayToWorkSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return WorkSpaceCoordinates(
            static_cast<int>(std::round(static_cast<float>(displayCoordinates.x) * mZoomFactor)),
            static_cast<int>(std::round(static_cast<float>(displayCoordinates.y) * mZoomFactor)));
    }

    ProjectionMatrix const & GetOrthoMatrix() const
    {
        return mOrthoMatrix;
    }

private:

    void RecalculateAttributes()
    {
        // Zoom factor
        mZoomFactor = std::ldexp(1.0f, -mZoom);

        // Ortho Matrix:
        //  WorkCoordinates * OrthoMatrix => NDC (-1.0, +1.0)
        //
        // SDsp is display scaled by zoom
        //
        //  2 / SDspW                0                        0                0
        //  0                        2 / SDspH                0                0
        //  0                        0                        0                0
        //  -2 * CamX / SDspW - 1    -2 * CamY / SDspH - 1    0                1

        float const sDspW = static_cast<float>(mDisplayPhysicalSize.width) * mZoomFactor;
        float const sDspH = static_cast<float>(mDisplayPhysicalSize.height) * mZoomFactor;

        // Recalculate Ortho Matrix cells (r, c)
        mOrthoMatrix[0][0] = 2.0f / sDspW;
        mOrthoMatrix[1][1] = 2.0f / sDspH;
        mOrthoMatrix[3][0] = -2.0f * mCam.x / sDspW - 1.0f;
        mOrthoMatrix[3][1] = -2.0f * mCam.y / sDspH - 1.0f;
    }

private:

    // Constants
    static int constexpr MaxZoom = 16;
    static int constexpr MinZoom = -8;

    // Primary inputs
    int mZoom; // >=0: display pixels occupied by one work space pixel
    WorkSpaceCoordinates mCam; // Work space coordinates of of work pixel that is visible at (0, 0) in display
    int const mLogicalToPhysicalPixelFactor;
    DisplayLogicalSize mDisplayLogicalSize;
    DisplayPhysicalSize mDisplayPhysicalSize;

    // Calculated attributes
    float mZoomFactor; // DisplayPhysical = Work * ZoomFactor
    ProjectionMatrix mOrthoMatrix;
};

}