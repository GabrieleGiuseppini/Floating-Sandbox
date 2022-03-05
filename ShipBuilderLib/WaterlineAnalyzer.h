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

private:

    Model const & mModel;

    //
    // State
    //

    enum class StateType
    {
        GetStaticResults,
        Completed
    };

    StateType mCurrentState;

    std::optional<StaticResults> mStaticResults;
    std::optional<vec2f> mCenterOfBuoyancy;
    std::optional<Waterline> mWaterline;
};

}