/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-10-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/Utils.h>

#include <picojson.h>

namespace Physics {

Fishes::Fishes(FishSpeciesDatabase const & fishSpeciesDatabase)
    : mFishSpeciesDatabase(fishSpeciesDatabase)
    , mFishes()
{
}

void Fishes::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld)
{
    //
    // Update number of fish
    //

    // TODO: honor fish species' swarm size

    if (mFishes.size() > gameParameters.NumberOfFishes)
    {
        mFishes.resize(gameParameters.NumberOfFishes);
    }
    else
    {
        for (size_t f = mFishes.size(); f < gameParameters.NumberOfFishes; ++f)
        {
            // TODO
            mFishes.emplace_back(
                vec2f(0.0f, -40.0f),
                TextureFrameIndex(0));
        }
    }
}

void Fishes::Upload(Render::RenderContext & renderContext) const
{
    renderContext.UploadFishesStart(mFishes.size());

    for (auto const & fish : mFishes)
    {
        renderContext.UploadFish(
            TextureFrameId<Render::FishTextureGroups>(Render::FishTextureGroups::Fish, fish.RenderFrameIndex),
            fish.Position);
    }

    renderContext.UploadFishesEnd();
}

////////////////////////////////////////////////////////////////////////////////////////////////

}