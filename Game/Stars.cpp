/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

void Stars::GenerateStars(unsigned int NumberOfStars)
{
    mStars.clear();
    mStars.reserve(NumberOfStars);

    for (unsigned int s = 0; s < NumberOfStars; ++s)
    {
        mStars.emplace_back(
            GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, +1.0f),
            GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, +1.0f),
            GameRandomEngine::GetInstance().GenerateUniformReal(0.25f, +1.0f));
    }
}

}