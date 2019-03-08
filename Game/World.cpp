/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "ShipBuilder.h"

#include <GameCore/GameRandomEngine.h>

#include <algorithm>
#include <cassert>

namespace Physics {

World::World(
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    GameParameters const & gameParameters,
    ResourceLoader & resourceLoader)
    : mAllShips()
    , mStars()
    , mClouds()
    , mWaterSurface()
    , mOceanFloor(resourceLoader)
    , mWind(gameEventHandler)
    , mCurrentSimulationTime(0.0f)
    , mGameEventHandler(std::move(gameEventHandler))
{
    // Initialize world pieces
    mStars.Update(gameParameters);
    mWind.Update(gameParameters);
    mClouds.Update(mCurrentSimulationTime, gameParameters);
    mWaterSurface.Update(mCurrentSimulationTime, mWind, gameParameters);
    mOceanFloor.Update(gameParameters);
}

ShipId World::AddShip(
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materialDatabase,
    GameParameters const & gameParameters)
{
    ShipId shipId = static_cast<ShipId>(mAllShips.size());

    auto ship = ShipBuilder::Create(
        shipId,
        *this,
        mGameEventHandler,
        shipDefinition,
        materialDatabase,
        gameParameters);

    mAllShips.push_back(std::move(ship));

    return shipId;
}

size_t World::GetShipCount() const
{
    return mAllShips.size();
}

size_t World::GetShipPointCount(ShipId shipId) const
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->GetPointCount();
}

//////////////////////////////////////////////////////////////////////////////
// Interactions
//////////////////////////////////////////////////////////////////////////////

void World::MoveBy(
    ShipId shipId,
    vec2f const & offset,
    GameParameters const & gameParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->MoveBy(
        offset,
        gameParameters);
}

void World::RotateBy(
    ShipId shipId,
    float angle,
    vec2f const & center,
    GameParameters const & gameParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->RotateBy(
        angle,
        center,
        gameParameters);
}

void World::DestroyAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->DestroyAt(
            targetPos,
            radiusMultiplier,
            mCurrentSimulationTime,
            gameParameters);
    }
}

void World::SawThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->SawThrough(
            startPos,
            endPos,
            mCurrentSimulationTime,
            gameParameters);
    }
}

void World::DrawTo(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->DrawTo(
            targetPos,
            strength,
            gameParameters);
    }
}

void World::SwirlAt(
    vec2f const & targetPos,
    float strength,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->SwirlAt(
            targetPos,
            strength,
            gameParameters);
    }
}

void World::TogglePinAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully pins or unpins a point
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->TogglePinAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

bool World::InjectBubblesAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully injects
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->InjectBubblesAt(
            targetPos,
            mCurrentSimulationTime,
            gameParameters))
        {
            // Found!
            return true;
        }

        // No luck...
        // search other ships
    }

    return false;
}

bool World::FloodAt(
    vec2f const & targetPos,
    float waterQuantityMultiplier,
    GameParameters const & gameParameters)
{
    // Flood all ships
    bool anyHasFlooded = false;
    for (auto & ship : mAllShips)
    {
        bool hasFlooded = ship->FloodAt(
            targetPos,
            waterQuantityMultiplier,
            gameParameters);

        anyHasFlooded |= hasFlooded;
    }

    return anyHasFlooded;
}

void World::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleAntiMatterBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleImpactBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleImpactBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleRCBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleRCBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleTimerBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->ToggleTimerBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::DetonateRCBombs()
{
    for (auto const & ship : mAllShips)
    {
        ship->DetonateRCBombs();
    }
}

void World::DetonateAntiMatterBombs()
{
    for (auto const & ship : mAllShips)
    {
        ship->DetonateAntiMatterBombs();
    }
}

bool World::AdjustOceanFloorTo(
    float x,
    float targetY)
{
    return mOceanFloor.AdjustTo(x, targetY);
}

std::optional<ObjectId> World::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    std::optional<ObjectId> bestPointId;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const & ship : mAllShips)
    {
        auto shipBestPointIndex = ship->GetNearestPointAt(targetPos, radius);
        if (NoneElementIndex != shipBestPointIndex)
        {
            float squareDistance = (ship->GetPoints().GetPosition(shipBestPointIndex) - targetPos).squareLength();
            if (squareDistance < bestSquareDistance)
            {
                bestPointId = ObjectId(ship->GetId(), shipBestPointIndex);
                bestSquareDistance = squareDistance;
            }
        }
    }

    return bestPointId;
}

void World::QueryNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    // Stop at first ship that successfully queries
    for (auto it = mAllShips.rbegin(); it != mAllShips.rend(); ++it)
    {
        if ((*it)->QueryNearestPointAt(targetPos, radius))
            return;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Simulation
//////////////////////////////////////////////////////////////////////////////

void World::Update(
    GameParameters const & gameParameters,
    Render::RenderContext const & renderContext)
{
    // Update current time
    mCurrentSimulationTime += GameParameters::SimulationStepTimeDuration<float>;

    // Update world parts
    mStars.Update(gameParameters);
    mWind.Update(gameParameters);
    mClouds.Update(mCurrentSimulationTime, gameParameters);
    mWaterSurface.Update(mCurrentSimulationTime, mWind, gameParameters);
    mOceanFloor.Update(gameParameters);

    // Update all ships
    for (auto & ship : mAllShips)
    {
        ship->Update(
            mCurrentSimulationTime,
            gameParameters,
            renderContext);
    }
}

void World::Render(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext) const
{
    //
    // Upload land and ocean data (before clouds and stars are rendered, as the latters
    // need the ocean stencil)
    //

    UploadLandAndOcean(gameParameters, renderContext);


    //
    // Render sky
    //

    renderContext.RenderSkyStart();

    // Upload stars
    mStars.Upload(renderContext);

    // Upload clouds
    mClouds.Upload(renderContext);

    renderContext.RenderSkyEnd();


    //
    // Render the ocean now, if we want to see the ship through the ocean
    //

    if (renderContext.GetShowShipThroughOcean())
    {
        renderContext.RenderOcean();
    }


    //
    // Render all ships
    //

    renderContext.RenderShipsStart();

    for (auto const & ship : mAllShips)
    {
        ship->Render(
            gameParameters,
            renderContext);
    }

    renderContext.RenderShipsEnd();


    //
    // Render the ocean now, if we want to see the ship *in* the ocean instead
    //

    if (!renderContext.GetShowShipThroughOcean())
    {
        renderContext.RenderOcean();
    }


    //
    // Render the ocean floor
    //

    renderContext.RenderLand();
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

void World::UploadLandAndOcean(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext) const
{
    size_t constexpr SlicesCount = 500;

    float const visibleWorldWidth = renderContext.GetVisibleWorldWidth();
    float const sliceWidth = visibleWorldWidth / static_cast<float>(SlicesCount);
    float sliceX = renderContext.GetCameraWorldPosition().x - (visibleWorldWidth / 2.0f);

    renderContext.UploadLandAndOceanStart(SlicesCount);

    // We do one extra iteration as the number of slices is the number of quads, and the last vertical
    // quad side must be at the end of the width
    for (size_t i = 0; i <= SlicesCount; ++i, sliceX += sliceWidth)
    {
        renderContext.UploadLandAndOcean(
            sliceX,
            mOceanFloor.GetFloorHeightAt(sliceX),
            mWaterSurface.GetWaterHeightAt(sliceX),
            gameParameters.SeaDepth);
    }

    renderContext.UploadLandAndOceanEnd();
}

}