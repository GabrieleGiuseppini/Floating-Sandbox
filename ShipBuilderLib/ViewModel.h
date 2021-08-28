/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

namespace ShipBuilder {

/*
 * This class maintains the logic for transformations between the various coordinates.
 */
class ViewModel
{
public:

    ViewModel(
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor)
        : mLogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , mDisplayLogicalSize(initialDisplaySize)
        , mDisplayPhysicalSize(
            initialDisplaySize.width * logicalToPhysicalPixelFactor,
            initialDisplaySize.height * logicalToPhysicalPixelFactor)
    {}

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

private:

    void RecalculateAttributes()
    {}

private:

    int const mLogicalToPhysicalPixelFactor;

    DisplayLogicalSize mDisplayLogicalSize;
    DisplayPhysicalSize mDisplayPhysicalSize;
};

}