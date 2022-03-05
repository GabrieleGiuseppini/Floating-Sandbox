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
        vec2f CenterOfMass; // In ship coordinates

        StaticResults(
            float totalMass,
            vec2f centerOfMass)
            : TotalMass(totalMass)
            , CenterOfMass(centerOfMass)
        {}
    };

public:

    WaterlineAnalyzer(Model const & model);

    std::optional<StaticResults> const & GetStaticResults() const
    {
        return mStaticResults;
    }

    /*
     * Runs a step of the analysis. Returns true if the analysis is complete.
     */
    bool Update();

private:

    StaticResults CalculateStaticResults();

private:

    Model const & mModel;

    std::optional<StaticResults> mStaticResults;

    //
    // State
    //

    enum class StateType
    {
        GetStaticResults,
        Completed
    };

    StateType mCurrentState;
};

}