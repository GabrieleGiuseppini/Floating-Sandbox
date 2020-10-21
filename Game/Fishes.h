/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FishSpeciesDatabase.h"
#include "GameParameters.h"
#include "RenderContext.h"
#include "RenderTypes.h"
#include "VisibleWorld.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <vector>

namespace Physics
{

class Fishes
{

public:

    explicit Fishes(FishSpeciesDatabase const & fishSpeciesDatabase);

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        VisibleWorld const & visibleWorld);

    void Upload(Render::RenderContext & renderContext) const;

private:

    FishSpeciesDatabase const & mFishSpeciesDatabase;


    struct Fish
    {
        vec2f Position;

        TextureFrameIndex RenderFrameIndex;

        Fish()
            : Position()
            , RenderFrameIndex(0)
        {}

        Fish(
            vec2f const & position,
            TextureFrameIndex const & renderFrameIndex)
            : Position(position)
            , RenderFrameIndex(renderFrameIndex)
        {}
    };

    std::vector<Fish> mFishes;
};

}
