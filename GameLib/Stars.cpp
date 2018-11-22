/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

void Stars::GenerateStars(size_t NumberOfStars)
{
    mStars.clear();
    mStars.reserve(NumberOfStars);

    for (size_t s = 0; s < NumberOfStars; ++s)
    {
        mStars.emplace_back(
            GameRandomEngine::GetInstance().GenerateRandomReal(-1.0f, +1.0f),
            GameRandomEngine::GetInstance().GenerateRandomReal(-1.0f, +1.0f),
            GameRandomEngine::GetInstance().GenerateRandomReal(0.25f, +1.0f));
    }
}

}