/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"
#include "ShipBuilder.h"

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
    , mCurrentSimulationTime(0.0f)
    , mCurrentVisitSequenceNumber(1u)
    , mGameEventHandler(std::move(gameEventHandler))
{
    // Initialize world pieces
    mStars.Update(gameParameters);
    mClouds.Update(mCurrentSimulationTime, gameParameters);
    mWaterSurface.Update(mCurrentSimulationTime, gameParameters);
    mOceanFloor.Update(gameParameters);
}

ShipId World::AddShip(
    ShipDefinition const & shipDefinition,
    std::shared_ptr<MaterialDatabase> materials,
    GameParameters const & gameParameters)
{
    ShipId shipId = static_cast<ShipId>(mAllShips.size());

    auto newShip = ShipBuilder::Create(
        shipId,
        *this,
        mGameEventHandler,
        shipDefinition,
        materials,
        gameParameters,
        mCurrentVisitSequenceNumber);

    mAllShips.push_back(std::move(newShip));

    return shipId;
}

size_t World::GetShipPointCount(ShipId shipId) const
{
    return mAllShips[shipId]->GetPointCount();
}

void World::MoveBy(
    ShipId shipId,
    vec2f const & offset)
{
    assert(shipId < mAllShips.size());
    mAllShips[shipId]->MoveBy(offset);
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
    float strength)
{
    for (auto & ship : mAllShips)
    {
        ship->DrawTo(
            targetPos,
            strength);
    }
}

void World::SwirlAt(
    vec2f const & targetPos,
    float strength)
{
    for (auto & ship : mAllShips)
    {
        ship->SwirlAt(
            targetPos,
            strength);
    }
}

void World::TogglePinAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully pins or unpins a point
    for (auto const & ship : mAllShips)
    {
        if (ship->TogglePinAt(targetPos, gameParameters))
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
    for (auto const & ship : mAllShips)
    {
        if (ship->ToggleTimerBombAt(targetPos, gameParameters))
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
    for (auto const & ship : mAllShips)
    {
        if (ship->ToggleRCBombAt(targetPos, gameParameters))
        {
            // Found!
            return;
        }

        // No luck...
        // search other ships
    }
}

void World::ToggleAntiMatterBombAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a bomb
    for (auto const & ship : mAllShips)
    {
        if (ship->ToggleAntiMatterBombAt(targetPos, gameParameters))
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

std::optional<ObjectId> World::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    std::optional<ObjectId> bestPointId;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const & ship : mAllShips)
    {
        auto shipBestPointIndex = ship->GetNearestPointIndexAt(targetPos, radius);
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

void World::Update(GameParameters const & gameParameters)
{
    // Update current time
    mCurrentSimulationTime += GameParameters::SimulationStepTimeDuration<float>;

    // Generate a new visit sequence number
    ++mCurrentVisitSequenceNumber;
    if (NoneVisitSequenceNumber == mCurrentVisitSequenceNumber)
        mCurrentVisitSequenceNumber = 1u;

    // Update world parts
    mStars.Update(gameParameters);
    mClouds.Update(mCurrentSimulationTime, gameParameters);
    mWaterSurface.Update(mCurrentSimulationTime, gameParameters);
    mOceanFloor.Update(gameParameters);

    // Update all ships
    for (auto & ship : mAllShips)
    {
        ship->Update(
            mCurrentSimulationTime,
            mCurrentVisitSequenceNumber,
            gameParameters);
    }
}

void World::Render(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext) const
{
    // Upload stars
    mStars.Upload(renderContext);

    // Upload land and water data (before clouds and stars are rendered, as the latters
    // need the water stencil)
    UploadLandAndWater(gameParameters, renderContext);

    // Render the clouds (and stars)
    mClouds.Render(renderContext);

    // Render the water now, if we want to see the ship through the water
    if (renderContext.GetShowShipThroughSeaWater())
    {
        renderContext.RenderWater();
    }

    // Render all ships
    for (auto const & ship : mAllShips)
    {
        ship->Render(
            gameParameters,
            renderContext);
    }

    // Render the water now, if we want to see the ship *in* the water instead
    if (!renderContext.GetShowShipThroughSeaWater())
    {
        renderContext.RenderWater();
    }

    // Render the ocean floor
    renderContext.RenderLand();
}

///////////////////////////////////////////////////////////////////////////////////
// Private Helpers
///////////////////////////////////////////////////////////////////////////////////

void World::UploadLandAndWater(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext) const
{
    static constexpr size_t SlicesCount = 500;

    float const visibleWorldWidth = renderContext.GetVisibleWorldWidth();
    float const sliceWidth = visibleWorldWidth / static_cast<float>(SlicesCount);
    float sliceX = renderContext.GetCameraWorldPosition().x - (visibleWorldWidth / 2.0f);

    renderContext.UploadLandAndWaterStart(SlicesCount);

    // We do one extra iteration as the number of slices isthe number of quads, and the last vertical
    // quad side must be at the end of the width
    for (size_t i = 0; i <= SlicesCount; ++i, sliceX += sliceWidth)
    {
        renderContext.UploadLandAndWater(
            sliceX,
            mOceanFloor.GetFloorHeightAt(sliceX),
            mWaterSurface.GetWaterHeightAt(sliceX),
            gameParameters.SeaDepth);
    }

    renderContext.UploadLandAndWaterEnd();
}

}