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
 *  - ShipSpace: has the pixel size of the structure (equivalent of ::World)
 *  - ShipSpaceCoordinates
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
        ShipSpaceSize initialShipSize,
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor)
        : mZoom(0)
        , mCam(0, 0)
        , mLogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , mDisplayLogicalSize(initialDisplaySize)
        , mDisplayPhysicalSize(
            initialDisplaySize.width * logicalToPhysicalPixelFactor,
            initialDisplaySize.height * logicalToPhysicalPixelFactor)
        , mShipSize(initialShipSize)
        , mDisplayPhysicalToShipSpaceFactor(0) // Will be recalculated
        , mCamLimits(0, 0) // Will be recalculated
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

    int CalculateIdealZoom() const
    {
        // Zoom>=0: display pixels occupied by one ship space pixel
        int idealZoom = 0;
        for (int candidateZoom = 1; candidateZoom <= MaxZoom; ++candidateZoom)
        {
            // Check if ship size fits with this zoom
            float const physicalWidth = static_cast<float>(mShipSize.width + MarginDisplayShipSize * 2) / CalculateDisplayPhysicalToShipSpaceFactor(candidateZoom);
            if (physicalWidth > mDisplayPhysicalSize.width)
            {
                break;
            }

            // Still good
            idealZoom = candidateZoom;
        }

        return idealZoom;
    }

    float CalculateGridPhysicalPixelStepSize() const
    {
        // Calculate grid step size based on current zoom;
        // we don't want the grid spacing to get too small
        float const stepSize = 1.0f / mDisplayPhysicalToShipSpaceFactor;
        return std::max(stepSize, 8.0f);
    }

    ShipSpaceCoordinates const & GetCameraShipSpacePosition() const
    {
        return mCam;
    }

    ShipSpaceCoordinates const & SetCameraShipSpacePosition(ShipSpaceCoordinates const & pos)
    {
        mCam = ShipSpaceCoordinates(
            std::min(pos.x, mCamLimits.width),
            std::min(pos.y, mCamLimits.height));

        RecalculateAttributes();

        return mCam;
    }

    ShipSpaceSize const & GetShipSize() const
    {
        return mShipSize;
    }

    void SetShipSize(ShipSpaceSize const & size)
    {
        mShipSize = size;

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

        RecalculateAttributes();
    }

    ShipSpaceSize GetCameraRange() const
    {
        ShipSpaceSize const visibleShipSpaceSize = GetVisibleShipSpaceSize();

        return ShipSpaceSize(
            mShipSize.width + MarginDisplayShipSize * 2,
            mShipSize.height + MarginDisplayShipSize * 2);
    }

    ShipSpaceSize GetCameraThumbSize() const
    {
        ShipSpaceSize const visibleShipSpaceSize = GetVisibleShipSpaceSize();

        return ShipSpaceSize(
            std::min(mShipSize.width + MarginDisplayShipSize * 2, visibleShipSpaceSize.width),
            std::min(mShipSize.height + MarginDisplayShipSize * 2, visibleShipSpaceSize.height));
    }

    ShipSpaceSize GetVisibleShipSpaceSize() const
    {
        return ShipSpaceSize(
            DisplayPhysicalToShipSpace(mDisplayPhysicalSize.width),
            DisplayPhysicalToShipSpace(mDisplayPhysicalSize.height));
    }

    //
    // Coordinate transformations
    //

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return ShipSpaceCoordinates(
            DisplayPhysicalToShipSpace(displayCoordinates.x * mLogicalToPhysicalPixelFactor) - MarginDisplayShipSize + mCam.x,
            mShipSize.height - 1 - (DisplayPhysicalToShipSpace(displayCoordinates.y * mLogicalToPhysicalPixelFactor) - MarginDisplayShipSize + mCam.y));
    }

    float GetShipSpaceForOnePhysicalDisplayPixel() const
    {
        return mDisplayPhysicalToShipSpaceFactor;
    }

    DisplayPhysicalSize ShipSpaceSizeToPhysicalDisplaySize(ShipSpaceSize const & size)
    {
        return DisplayPhysicalSize(
            static_cast<int>(ShipSpaceToDisplayLogical(static_cast<float>(size.width)) * mLogicalToPhysicalPixelFactor),
            static_cast<int>(ShipSpaceToDisplayLogical(static_cast<float>(size.height)) * mLogicalToPhysicalPixelFactor));
    }

    ProjectionMatrix const & GetOrthoMatrix() const
    {
        return mOrthoMatrix;
    }

private:

    void RecalculateAttributes()
    {
        // Display physicial => Ship factor
        mDisplayPhysicalToShipSpaceFactor = CalculateDisplayPhysicalToShipSpaceFactor(mZoom);

        // Recalculate pan limits
        mCamLimits = ShipSpaceSize(
            std::max(
                (2 + mShipSize.width) - DisplayPhysicalToShipSpace(mDisplayPhysicalSize.width),
                0),
            std::max(
                (2 + mShipSize.height) - DisplayPhysicalToShipSpace(mDisplayPhysicalSize.height),
                0));

        // Adjust camera accordingly
        mCam = ShipSpaceCoordinates(
            std::min(mCam.x, mCamLimits.width),
            std::min(mCam.y, mCamLimits.height));

        // Ortho Matrix:
        //  ShipCoordinates * OrthoMatrix => NDC
        //
        //  Ship: (0, W), (0, H) (positive right-top)
        //  NDC : (-1.0, -1.0 + 2W/DisplayW), (+1.0 - 2H/DisplayH, +1.0) (positive right-top)
        //
        // We add a (left, top) margin whose physical pixel size equals the physical pixel
        // size if one ship space pixel at max zoom.
        //
        // SDsp is display scaled by zoom.
        //
        //  2 / SDspW                0                               0                0
        //  0                        2 / SDspH                       0                0
        //  0                        0                               0                0
        //  -2 * CamX / SDspW - 1    1 - 2 * (H - CamY) / SDspH      0                1

        float const sDspW = static_cast<float>(mDisplayPhysicalSize.width) * mDisplayPhysicalToShipSpaceFactor;
        float const sDspH = static_cast<float>(mDisplayPhysicalSize.height) * mDisplayPhysicalToShipSpaceFactor;

        // Recalculate Ortho Matrix cells (r, c)
        mOrthoMatrix[0][0] = 2.0f / sDspW;
        mOrthoMatrix[1][1] = 2.0f / sDspH;
        mOrthoMatrix[3][0] = -2.0f * (static_cast<float>(mCam.x) - MarginDisplayShipSize) / sDspW - 1.0f;
        mOrthoMatrix[3][1] = 1.0f - 2.0f * static_cast<float>(mShipSize.height - mCam.y + MarginDisplayShipSize) / sDspH;
    }

    static float CalculateDisplayPhysicalToShipSpaceFactor(int zoom)
    {
        return std::ldexp(1.0f, -zoom);
    }

    int DisplayPhysicalToShipSpace(int size) const
    {
        return static_cast<int>(std::floor(static_cast<float>(size) * mDisplayPhysicalToShipSpaceFactor));
    }

    template<typename TValue>
    float ShipSpaceToDisplayLogical(TValue size) const
    {
        return static_cast<float>(size) / mDisplayPhysicalToShipSpaceFactor;
    }

private:

    // Constants
    static int constexpr MaxZoom = 6;
    static int constexpr MinZoom = -2;
    static int constexpr MarginDisplayShipSize = 1;

    // Primary inputs
    int mZoom; // >=0: display pixels occupied by one ship space pixel
    ShipSpaceCoordinates mCam; // Ship space coordinates of of ship pixel that is visible at (0, 0) in display
    int const mLogicalToPhysicalPixelFactor;
    DisplayLogicalSize mDisplayLogicalSize;
    DisplayPhysicalSize mDisplayPhysicalSize;
    ShipSpaceSize mShipSize;

    // Calculated attributes
    float mDisplayPhysicalToShipSpaceFactor; // # ship pixels for 1 display pixel
    ShipSpaceSize mCamLimits;
    ProjectionMatrix mOrthoMatrix;
};

}