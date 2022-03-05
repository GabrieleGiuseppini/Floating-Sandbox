/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-04
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "WaterlineAnalyzer.h"

#include <cassert>

namespace ShipBuilder {

WaterlineAnalyzer::WaterlineAnalyzer(Model const & model)
    : mModel(model)
    , mCurrentState(StateType::GetStaticResults)
{
}

bool WaterlineAnalyzer::Update()
{
    switch (mCurrentState)
    {
        case StateType::GetStaticResults:
        {
            mStaticResults = CalculateStaticResults();

            if (mStaticResults.has_value())
            {
                // TODOTEST
                mCurrentState = StateType::Completed;
                return true;
            }
            else
            {
                // No particles, we're done
                mCurrentState = StateType::Completed;
                return true;
            }
        }

        case StateType::Completed:
        {
            assert(false);
            return true;
        }
    }
}

std::optional<WaterlineAnalyzer::StaticResults> WaterlineAnalyzer::CalculateStaticResults()
{
    float totalMass = 0.0f;
    vec2f centerOfMass = vec2f::zero();

    auto const & structuralLayerBuffer = mModel.GetStructuralLayer().Buffer;
    for (int y = 0; y < structuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayerBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            auto const * material = structuralLayerBuffer[coords].Material;
            if (material != nullptr)
            {
                totalMass += material->GetMass();
                centerOfMass += coords.ToFloat() * material->GetMass();
            }
        }
    }

    if (totalMass == 0.0f)
    {
        // No particles
        return std::nullopt;
    }

    return StaticResults(
        totalMass,
        centerOfMass / totalMass);
}

}