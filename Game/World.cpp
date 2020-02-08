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
    OceanFloorTerrain && oceanFloorTerrain,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::shared_ptr<TaskThreadPool> taskThreadPool,
    GameParameters const & gameParameters)
    : mCurrentSimulationTime(0.0f)
    , mAllShips()
    , mStars()
    , mStorm(*this, gameEventDispatcher)
    , mWind(gameEventDispatcher)
    , mClouds()
    , mOceanSurface(gameEventDispatcher)
    , mOceanFloor(std::move(oceanFloorTerrain))
    , mGameEventHandler(std::move(gameEventDispatcher))
    , mTaskThreadPool(std::move(taskThreadPool))
{
    // Initialize world pieces
    mStars.Update(gameParameters);
    mStorm.Update(mCurrentSimulationTime, gameParameters);
    mWind.Update(mStorm.GetParameters(), gameParameters);
    mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), gameParameters);
    mOceanSurface.Update(mCurrentSimulationTime, mWind, gameParameters);
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
        mTaskThreadPool,
        shipDefinition,
        materialDatabase,
        gameParameters);

    mAllShips.push_back(std::move(ship));

    return shipId;
}

void World::Announce()
{
    // Nothing to announce in non-ship stuff...
    // ...ask all ships to announce
    for (auto & ship : mAllShips)
    {
        ship->Announce();
    }
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

void World::PickPointToMove(
    vec2f const & pickPosition,
    std::optional<ElementId> & elementId,
    GameParameters const & gameParameters) const
{
    for (auto & ship : mAllShips)
    {
        auto elementIndex = ship->PickPointToMove(
            pickPosition,
            gameParameters);

        if (!!elementIndex)
        {
            elementId = ElementId(ship->GetId(), *elementIndex);
            return;
        }
    }

    elementId = std::nullopt;
}

void World::MoveBy(
    ElementId elementId,
    vec2f const & offset,
    vec2f const & inertialVelocity,
    GameParameters const & gameParameters)
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->MoveBy(
        elementId.GetLocalObjectId(),
        offset,
        inertialVelocity,
        gameParameters);
}

void World::MoveBy(
    ShipId shipId,
    vec2f const & offset,
    vec2f const & inertialVelocity,
    GameParameters const & gameParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->MoveBy(
        offset,
        inertialVelocity,
        gameParameters);
}

void World::RotateBy(
    ElementId elementId,
    float angle,
    vec2f const & center,
    float inertialAngle,
    GameParameters const & gameParameters)
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->RotateBy(
        elementId.GetLocalObjectId(),
        angle,
        center,
        inertialAngle,
        gameParameters);
}

void World::RotateBy(
    ShipId shipId,
    float angle,
    vec2f const & center,
    float inertialAngle,
    GameParameters const & gameParameters)
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->RotateBy(
        angle,
        center,
        inertialAngle,
        gameParameters);
}

void World::DestroyAt(
    vec2f const & targetPos,
    float radiusFraction,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->DestroyAt(
            targetPos,
            radiusFraction,
            mCurrentSimulationTime,
            gameParameters);
    }
}

void World::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    RepairSessionId sessionId,
    RepairSessionStepId sessionStepId,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->RepairAt(
            targetPos,
            radiusMultiplier,
            sessionId,
            sessionStepId,
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

bool World::ApplyHeatBlasterAt(
    vec2f const & targetPos,
    HeatBlasterActionType action,
    float radius,
    GameParameters const & gameParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool isApplied = ship->ApplyHeatBlasterAt(
            targetPos,
            action,
            radius,
            gameParameters);

        atLeastOneShipApplied |= isApplied;
    }

    return atLeastOneShipApplied;
}

bool World::ExtinguishFireAt(
    vec2f const & targetPos,
    float radius,
    GameParameters const & gameParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool isApplied = ship->ExtinguishFireAt(
            targetPos,
            radius,
            gameParameters);

        atLeastOneShipApplied |= isApplied;
    }

    return atLeastOneShipApplied;
}

void World::DrawTo(
    vec2f const & targetPos,
    float strengthFraction,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->DrawTo(
            targetPos,
            strengthFraction,
            gameParameters);
    }
}

void World::SwirlAt(
    vec2f const & targetPos,
    float strengthFraction,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->SwirlAt(
            targetPos,
            strengthFraction,
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

void World::AdjustOceanSurfaceTo(std::optional<vec2f> const & worldCoordinates)
{
    mOceanSurface.AdjustTo(
        worldCoordinates,
        mCurrentSimulationTime);
}

bool World::AdjustOceanFloorTo(
    float x1,
    float targetY1,
    float x2,
    float targetY2)
{
    return mOceanFloor.AdjustTo(x1, targetY1, x2, targetY2);
}

bool World::ScrubThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    // Scrub all ships
    bool anyHasScrubbed = false;
    for (auto & ship : mAllShips)
    {
        bool hasScrubbed = ship->ScrubThrough(
            startPos,
            endPos,
            gameParameters);

        anyHasScrubbed |= hasScrubbed;
    }

    return anyHasScrubbed;
}

void World::ApplyThanosSnap(
    float centerX,
    float radius,
    float leftFrontX,
    float rightFrontX,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    // Apply to all ships
    for (auto & ship : mAllShips)
    {
        ship->ApplyThanosSnap(
            centerX,
            radius,
            leftFrontX,
            rightFrontX,
            currentSimulationTime,
            gameParameters);
    }

    // Apply to ocean surface
    mOceanSurface.ApplyThanosSnap(leftFrontX, rightFrontX);
}

std::optional<ElementId> World::GetNearestPointAt(
    vec2f const & targetPos,
    float radius) const
{
    std::optional<ElementId> bestPointId;
    float bestSquareDistance = std::numeric_limits<float>::max();

    for (auto const & ship : mAllShips)
    {
        auto shipBestPointIndex = ship->GetNearestPointAt(targetPos, radius);
        if (NoneElementIndex != shipBestPointIndex)
        {
            float squareDistance = (ship->GetPoints().GetPosition(shipBestPointIndex) - targetPos).squareLength();
            if (squareDistance < bestSquareDistance)
            {
                bestPointId = ElementId(ship->GetId(), shipBestPointIndex);
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

std::optional<vec2f> World::FindSuitableLightningTarget() const
{
	// Try all ships until a target is found
	for (auto const & ship : mAllShips)
	{
		auto target = ship->FindSuitableLightningTarget();
		if (!!target)
			return target;
	}

	return std::nullopt;
}

void World::ApplyLightning(
	vec2f const & targetPos,
	float currentSimulationTime,
	GameParameters const & gameParameters)
{
	// Apply to all ships
	for (auto & ship : mAllShips)
	{
		ship->ApplyLightning(targetPos, currentSimulationTime, gameParameters);
	}
}

void World::TriggerTsunami()
{
    mOceanSurface.TriggerTsunami(mCurrentSimulationTime);
}

void World::TriggerStorm()
{
    mStorm.TriggerStorm();
}

void World::TriggerLightning()
{
	mStorm.TriggerLightning();
}

void World::TriggerRogueWave()
{
    mOceanSurface.TriggerRogueWave(
        mCurrentSimulationTime,
        mWind);
}

void World::SetSwitchState(ElectricalElementId electricalElementId, ElectricalState switchState)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->SetSwitchState(electricalElementId, switchState);
}

void World::SetSilence(float silenceAmount)
{
    mWind.SetSilence(silenceAmount);
}

//////////////////////////////////////////////////////////////////////////////
// Simulation
//////////////////////////////////////////////////////////////////////////////

void World::UpdateAndRender(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext,
    bool doUpdate,
    PerfStats & perfStats)
{
    if (doUpdate)
    {
        // Update current time
        mCurrentSimulationTime += GameParameters::SimulationStepTimeDuration<float>;
    }

    //
    // Update stars
    //

    if (doUpdate)
    {
        auto const updateStartTime = GameChronometer::now();

        mStars.Update(gameParameters);

        perfStats.TotalUpdateDuration += GameChronometer::now() - updateStartTime;
    }

    //
    // Render stars
    //

    {
        auto const renderStartTime = GameChronometer::now();

        // Upload
        mStars.Upload(renderContext);

        // Render
        renderContext.RenderStars();

        perfStats.TotalRenderDuration += GameChronometer::now() - renderStartTime;
    }

    //
    // Update clouds
    //

    if (doUpdate)
    {
        auto const updateStartTime = GameChronometer::now();

        //
        // Update storm
        //

        mStorm.Update(mCurrentSimulationTime, gameParameters);

        //
        // Update wind
        //

        mWind.Update(mStorm.GetParameters(), gameParameters);

        //
        // Update clouds
        //

        mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), gameParameters);


        perfStats.TotalUpdateDuration += GameChronometer::now() - updateStartTime;
    }

    //
    // Render clouds
    //

    {
        auto const renderStartTime = GameChronometer::now();

        renderContext.RenderCloudsStart();

        //
        // Upload storm
        //

        mStorm.Upload(renderContext);

        //
        // Upload clouds
        //

        mClouds.Upload(renderContext);

        renderContext.RenderCloudsEnd();

        auto const elapsed = GameChronometer::now() - renderStartTime;
        perfStats.TotalCloudRenderDuration += elapsed;
        perfStats.TotalRenderDuration += elapsed;
    }

    //
    // Update Ocean
    //

    if (doUpdate)
    {
        auto const updateStartTime = GameChronometer::now();


        //
        // Update ocean surface
        //

        mOceanSurface.Update(mCurrentSimulationTime, mWind, gameParameters);

        perfStats.TotalOceanSurfaceUpdateDuration += GameChronometer::now() - updateStartTime;

        //
        // Update ocean floor
        //

        mOceanFloor.Update(gameParameters);


        perfStats.TotalUpdateDuration += GameChronometer::now() - updateStartTime;
    }

    //
    // Render Ocean
    //

    {
        auto const renderStartTime = GameChronometer::now();


        //
        // Upload land
        //

        mOceanFloor.Upload(gameParameters, renderContext);

        //
        // Upload ocean surface
        //

        mOceanSurface.Upload(gameParameters, renderContext);

        //
        // Render ocean (opaquely over sky)
        //

        renderContext.RenderOceanOpaquely();


        perfStats.TotalRenderDuration += GameChronometer::now() - renderStartTime;
    }

    //
    // Update Ships
    //

    if (doUpdate)
    {
        auto const updateStartTime = GameChronometer::now();

        for (auto & ship : mAllShips)
        {
            ship->Update(
                mCurrentSimulationTime,
                mStorm.GetParameters(),
                gameParameters,
                renderContext);
        }

        auto const elapsed = GameChronometer::now() - updateStartTime;
        perfStats.TotalShipsUpdateDuration += elapsed;
        perfStats.TotalUpdateDuration += elapsed;
    }

    //
    // Render Ships and Remaining Parts
    //

    {
        auto const renderStartTime = GameChronometer::now();

        {
            renderContext.RenderShipsStart();

            for (auto const & ship : mAllShips)
            {
                ship->Render(
                    gameParameters,
                    renderContext);
            }

            renderContext.RenderShipsEnd();

            perfStats.TotalShipsRenderDuration += GameChronometer::now() - renderStartTime;
        }

        //
        // Render the ocean transparently, if we want to see the ship *in* the ocean instead
        //

        if (!renderContext.GetShowShipThroughOcean())
        {
            renderContext.RenderOceanTransparently();
        }

        //
        // Render the ocean floor
        //

        {
            auto const renderStartTime1 = GameChronometer::now();

            renderContext.RenderLand();

            perfStats.TotalOceanFloorRenderDuration += GameChronometer::now() - renderStartTime1;
        }

        perfStats.TotalRenderDuration += GameChronometer::now() - renderStartTime;
    }
}

}