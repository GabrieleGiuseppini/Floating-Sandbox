/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "IModelObservable.h"

#include <Core/Vectors.h>

#include <optional>

namespace ShipBuilder {

class WaterlineAnalyzer final
{
public:

    struct StaticResults
    {
        float TotalMass;
        vec2f CenterOfMass; // Ship coordinates

        StaticResults(
            float totalMass,
            vec2f centerOfMass)
            : TotalMass(totalMass)
            , CenterOfMass(centerOfMass)
        {}
    };

    struct Waterline
    {
        vec2f Center; // Ship coordinates
        vec2f WaterDirection; // Normalized, pointing "down" into water

        Waterline(
            vec2f center,
            vec2f waterDirection)
            : Center(center)
            , WaterDirection(waterDirection)
        {}
    };

public:

    explicit WaterlineAnalyzer(IModelObservable const & model);

    ModelMacroProperties const & GetModelMacroProperties() const
    {
        return mModelMacroProperties;
    }

    std::optional<float> const & GetTotalBuoyantForceWhenFullySubmerged() const
    {
        return mTotalBuoyantForceWhenFullySubmerged;
    }

    std::optional<float> const & GetTotalBuoyantForce() const
    {
        return mTotalBuoyantForce;
    }

    std::optional<vec2f> const & GetCenterOfBuoyancy() const
    {
        return mCenterOfBuoyancy;
    }

    std::optional<Waterline> const & GetWaterline() const
    {
        return mWaterline;
    }

    /*
     * Runs a step of the analysis. Returns true if the analysis is complete.
     */
    bool Update();

private:

    std::tuple<float, float> CalculateLevelSearchLimits(
        vec2f const & center,
        vec2f const & direction);

    // Total buoyance force, center of buoyancy
    std::tuple<float, vec2f> CalculateBuoyancyWithWaterline(
        vec2f const & waterlineCenter, // Ship coordinates
        vec2f const & waterlineDirection);

private:

    IModelObservable const & mModel;
    ModelMacroProperties const mModelMacroProperties;

    //
    // Search state
    //

    std::optional<float> mTotalBuoyantForceWhenFullySubmerged;

    std::optional<float> mTotalBuoyantForce;
    std::optional<vec2f> mCenterOfBuoyancy;

    std::optional<Waterline> mWaterline;

    //
    // Direction: normalized vector that is normal to waterline
    //
    // Heading towards water
    //

    float mDirectionSearchCWAngleMax; // The maximum (most positive) CW direction (wrt Vertical) we're willing to go when following a negative (CW) torque
    float mDirectionSearchCWAngleMin; // The minimum (most negative) CW direction (wrt Vertical) we're willing to go when following a positive (CCW) torque
    vec2f mDirectionSearchCurrent; // Positive towards "bottom"

    //
    // Level: place along the CoM->Direction vector where the waterline meets the vector
    //
    // - opposite to direction
    // 0 at CoM
    // + along direction
    //

    float mLevelSearchLowest; // Same heading as direction, grows the further in same heading; "bottom"
    float mLevelSearchHighest; // Less in numerical value than lowest
    float mLevelSearchCurrent;
};

}