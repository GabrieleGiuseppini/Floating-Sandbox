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

            if (mStaticResults->TotalMass == 0.0f)
            {
                // No particles, we're done
                mCurrentState = StateType::Completed;
                return true;
            }
            else
            {
                // Continue to next state

                // TODOTEST
                mCurrentState = StateType::Completed;
                mWaterline.emplace(
                    vec2f(float(mModel.GetShipSize().width) / 2.0f, float(mModel.GetShipSize().height) / 2.0f),
                    vec2f(0.0f, -1.0f));


                return true;
            }
        }

        case StateType::Completed:
        {
            assert(false);
            return true;
        }
    }

    assert(false);
    return false;
}

WaterlineAnalyzer::StaticResults WaterlineAnalyzer::CalculateStaticResults()
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

    return StaticResults(
        totalMass,
        centerOfMass / (totalMass != 0.0f ? totalMass : 1.0f));
}

}