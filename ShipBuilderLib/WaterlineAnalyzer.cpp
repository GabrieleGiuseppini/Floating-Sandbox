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

            mCurrentState = StateType::Completed;
            return true;
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
    // TODOHERE
    return std::nullopt;
}

}