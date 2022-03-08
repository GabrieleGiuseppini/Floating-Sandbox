/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Model.h"

#include <GameCore/Vectors.h>

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

    WaterlineAnalyzer(Model const & model);

    std::optional<StaticResults> const & GetStaticResults() const
    {
        return mStaticResults;
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

    StaticResults CalculateStaticResults();

    std::tuple<float, float> CalculateLevelSearchLimits(
        vec2f const & center,
        vec2f const & direction);

    // Total buoyance force, center of buoyancy
    std::tuple<float, vec2f> CalculateBuoyancy(
        vec2f const & waterlineCenter, // Ship coordinates
        vec2f const & waterlineDirection);

private:

    Model const & mModel;

    //
    // Search state
    //

    enum class StateType
    {
        CalculateStaticResults,
        FindLevel,
        Completed
    };

    StateType mCurrentState;

    std::optional<StaticResults> mStaticResults;

    std::optional<float> mTotalBuoyantForce;
    std::optional<vec2f> mCenterOfBuoyancy;

    std::optional<Waterline> mWaterline;

    vec2f mDirectionSearchCurrent; // Positive towards "bottom"

    float mLevelSearchLowest; // Same heading as direction, grows the further in same heading; "bottom"
    float mLevelSearchHighest; // Less in numerical value than lowest
    float mLevelSearchCurrent;
};

}