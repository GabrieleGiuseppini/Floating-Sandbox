﻿/***************************************************************************************
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
    FishSpeciesDatabase const & fishSpeciesDatabase,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::shared_ptr<TaskThreadPool> taskThreadPool,
    GameParameters const & gameParameters,
    VisibleWorld const & /*visibleWorld*/)
    : mCurrentSimulationTime(0.0f)
    //
    , mGameEventHandler(std::move(gameEventDispatcher))
    , mEventRecorder(nullptr)
    , mTaskThreadPool(std::move(taskThreadPool))
    //
    , mAllShips()
    , mStars()
    , mStorm(*this, mGameEventHandler)
    , mWind(mGameEventHandler)
    , mClouds()
    , mOceanSurface(*this, mGameEventHandler)
    , mOceanFloor(std::move(oceanFloorTerrain))
    , mFishes(fishSpeciesDatabase, mGameEventHandler)
    //
    , mAllAABBs()
{
    // Initialize world pieces that need to be initialized now
    mStars.Update(gameParameters);
    mStorm.Update(mCurrentSimulationTime, gameParameters);
    mWind.Update(mStorm.GetParameters(), gameParameters);
    mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), gameParameters);
    mOceanSurface.Update(mCurrentSimulationTime, mWind, gameParameters);
    mOceanFloor.Update(gameParameters);
}

std::tuple<ShipId, RgbaImageData> World::AddShip(
    ShipDefinition && shipDefinition,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    GameParameters const & gameParameters)
{
    ShipId const shipId = static_cast<ShipId>(mAllShips.size());

    // Build ship
    auto [ship, textureImage] = ShipBuilder::Create(
        shipId,
        *this,
        mGameEventHandler,
        mTaskThreadPool,
        std::move(shipDefinition),
        materialDatabase,
        shipTexturizer,
        gameParameters);

    // Set event recorder in new ship (if any)
    ship->SetEventRecorder(mEventRecorder);

    // Store ship
    mAllShips.push_back(std::move(ship));

    return std::make_tuple(shipId, std::move(textureImage));
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

void World::SetEventRecorder(EventRecorder * eventRecorder)
{
    mEventRecorder = eventRecorder;

    // Set in all ships
    for (auto & ship : mAllShips)
    {
        ship->SetEventRecorder(eventRecorder);
    }
}

void World::ReplayRecordedEvent(
    RecordedEvent const & event,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        if (ship->ReplayRecordedEvent(event, gameParameters))
        {
            break;
        }
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

vec2f World::GetShipSize(ShipId shipId) const
{
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->GetSize();
}

bool World::IsUnderwater(ElementId elementId) const
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->IsUnderwater(elementId.GetLocalObjectId());
}

//////////////////////////////////////////////////////////////////////////////
// Interactions
//////////////////////////////////////////////////////////////////////////////

void World::ScareFish(
    vec2f const & position,
    float radius,
    std::chrono::milliseconds delay)
{
    mFishes.DisturbAt(position, radius, delay);
}

void World::AttractFish(
    vec2f const & position,
    float radius,
    std::chrono::milliseconds delay)
{
    mFishes.AttractAt(position, radius, delay);
}

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

std::optional<ElementId> World::PickObjectForPickAndPull(
    vec2f const & pickPosition,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        auto elementIndex = ship->PickObjectForPickAndPull(
            pickPosition,
            gameParameters);

        if (elementIndex.has_value())
        {
            return ElementId(ship->GetId(), *elementIndex);
        }
    }

    // No luck
    return std::nullopt;
}

void World::Pull(
    ElementId elementId,
    vec2f const & target,
    GameParameters const & gameParameters)
{
    auto const shipId = elementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->Pull(
        elementId.GetLocalObjectId(),
        target,
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

    // Also scare fishes at bit
    mFishes.DisturbAt(
        targetPos,
        6.5f + radiusMultiplier,
        std::chrono::milliseconds(0));
}

void World::RepairAt(
    vec2f const & targetPos,
    float radiusMultiplier,
    SequenceNumber repairStepId,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->RepairAt(
            targetPos,
            radiusMultiplier,
            repairStepId,
            mCurrentSimulationTime,
            gameParameters);
    }
}

void World::SawThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    bool isFirstSegment,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->SawThrough(
            startPos,
            endPos,
            isFirstSegment,
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

void World::ApplyBlastAt(
    vec2f const & targetPos,
    float radius,
    float forceMultiplier,
    GameParameters const & gameParameters)
{
    for (auto & ship : mAllShips)
    {
        ship->ApplyBlastAt(
            targetPos,
            radius,
            forceMultiplier,
            gameParameters);
    }

    // Displace ocean surface
    {
        // Explosion depth (positive when underwater)
        float const explosionDepth = mOceanSurface.GetHeightAt(targetPos.x) - targetPos.y;

        // Calculate length of chord on circle given (clamped) distance of
        // ocean surface from center of blast (i.e. depth)
        float const circleRadius = radius * 2.0f; // Displacement sphere radius larger than blast
        float const halfChordLength = std::sqrt(
            std::max(
                circleRadius * circleRadius - explosionDepth * explosionDepth,
                0.0f));

        // Apply displacement along chord length
        for (float r = 0.0f; r < halfChordLength; r += 0.5f)
        {
            float const d =
                1.0f  // Magic number
                * (1.0f - r / halfChordLength) // Max at center, zero at extremes
                * (explosionDepth <= 0.0f ? -1.0f : 1.0f); // Follow depth sign

            mOceanSurface.DisplaceAt(targetPos.x - r, d);
            mOceanSurface.DisplaceAt(targetPos.x + r, d);
        }
    }

    // Also scare fishes at bit
    mFishes.DisturbAt(
        targetPos,
        radius,
        std::chrono::milliseconds(0));
}

bool World::ApplyElectricSparkAt(
    vec2f const & targetPos,
    float progress,
    GameParameters const & gameParameters)
{
    bool atLeastOneShipApplied = false;

    for (auto & ship : mAllShips)
    {
        bool isApplied = ship->ApplyElectricSparkAt(
            targetPos,
            progress,
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

std::optional<bool> World::TogglePhysicsProbeAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    // Stop at first ship that successfully places or removes a probe
    size_t iShip = mAllShips.size();
    do
    {
        --iShip;

        auto result = mAllShips[iShip]->TogglePhysicsProbeAt(targetPos, gameParameters);
        if (result.has_value())
        {
            // The probe has been placed or removed on this ship

            if (*result)
            {
                // The probe has been placed on this ship, remove it from all others
                for (size_t iShip2 = 0; iShip2 < mAllShips.size(); ++iShip2)
                {
                    if (iShip2 != iShip)
                    {
                        mAllShips[iShip2]->RemovePhysicsProbe();
                    }
                }
            }

            return result;
        }

        // No luck...
        // ...continue with other ships
    } while(iShip > 0);

    return std::nullopt;
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

std::optional<bool> World::AdjustOceanFloorTo(
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
        bool const hasScrubbed = ship->ScrubThrough(
            startPos,
            endPos,
            gameParameters);

        anyHasScrubbed |= hasScrubbed;
    }

    return anyHasScrubbed;
}

bool World::RotThrough(
    vec2f const & startPos,
    vec2f const & endPos,
    GameParameters const & gameParameters)
{
    // Rot all ships
    bool anyHasRotted = false;
    for (auto & ship : mAllShips)
    {
        bool const hasRotted = ship->RotThrough(
            startPos,
            endPos,
            gameParameters);

        anyHasRotted |= hasRotted;
    }

    return anyHasRotted;
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

    // Apply to fishes

    float constexpr DisturbanceRadius = 100.0f;

    mFishes.DisturbAt(
        vec2f(leftFrontX, 0.0f),
        DisturbanceRadius,
        std::chrono::milliseconds(0));

    mFishes.DisturbAt(
        vec2f(rightFrontX, 0.0f),
        DisturbanceRadius,
        std::chrono::milliseconds(0));
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

    // Apply to fishes
    DisturbOceanAt(
        targetPos,
        500.0f,
        std::chrono::milliseconds(0));
}

void World::TriggerTsunami()
{
    mOceanSurface.TriggerTsunami(mCurrentSimulationTime);

    DisturbOcean(std::chrono::milliseconds(0));
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

void World::HighlightElectricalElement(ElectricalElementId electricalElementId)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->HighlightElectricalElement(electricalElementId);
}

void World::SetSwitchState(
    ElectricalElementId electricalElementId,
    ElectricalState switchState,
    GameParameters const & gameParameters)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->SetSwitchState(
        electricalElementId,
        switchState,
        gameParameters);
}

void World::SetEngineControllerState(
    ElectricalElementId electricalElementId,
    int telegraphValue,
    GameParameters const & gameParameters)
{
    auto const shipId = electricalElementId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    mAllShips[shipId]->SetEngineControllerState(
        electricalElementId,
        telegraphValue,
        gameParameters);
}

void World::SetSilence(float silenceAmount)
{
    mWind.SetSilence(silenceAmount);
}

bool World::DestroyTriangle(ElementId triangleId)
{
    auto const shipId = triangleId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->DestroyTriangle(triangleId.GetLocalObjectId());
}

bool World::RestoreTriangle(ElementId triangleId)
{
    auto const shipId = triangleId.GetShipId();
    assert(shipId >= 0 && shipId < mAllShips.size());

    return mAllShips[shipId]->RestoreTriangle(triangleId.GetLocalObjectId());
}

//////////////////////////////////////////////////////////////////////////////
// Simulation
//////////////////////////////////////////////////////////////////////////////

void World::Update(
    GameParameters const & gameParameters,
    VisibleWorld const & visibleWorld,
    PerfStats & perfStats)
{
    // Update current time
    mCurrentSimulationTime += GameParameters::SimulationStepTimeDuration<float>;

    // Prepare all AABBs
    mAllAABBs.Clear();

    //
    // Update all subsystems
    //

    mStars.Update(gameParameters);

    mStorm.Update(mCurrentSimulationTime, gameParameters);

    mWind.Update(mStorm.GetParameters(), gameParameters);

    mClouds.Update(mCurrentSimulationTime, mWind.GetBaseAndStormSpeedMagnitude(), mStorm.GetParameters(), gameParameters);

    mOceanSurface.Update(mCurrentSimulationTime, mWind, gameParameters);

    mOceanFloor.Update(gameParameters);

    for (auto & ship : mAllShips)
    {
        ship->Update(
            mCurrentSimulationTime,
            mStorm.GetParameters(),
            gameParameters,
            mAllAABBs);
    }

    {
        auto const startTime = std::chrono::steady_clock::now();

        mFishes.Update(mCurrentSimulationTime, mOceanSurface, mOceanFloor, gameParameters, visibleWorld, mAllAABBs);

        perfStats.TotalFishUpdateDuration.Update(std::chrono::steady_clock::now() - startTime);
    }
}

void World::RenderUpload(
    GameParameters const & gameParameters,
    Render::RenderContext & renderContext,
    PerfStats & /*perfStats*/)
{
    mStars.Upload(renderContext);

    mWind.Upload(renderContext);

    mStorm.Upload(renderContext);

    mClouds.Upload(renderContext);

    mOceanFloor.Upload(gameParameters, renderContext);

    mOceanSurface.Upload(renderContext);

    mFishes.Upload(renderContext);

    // Ships
    {
        renderContext.UploadShipsStart();

        for (auto const & ship : mAllShips)
        {
            ship->RenderUpload(renderContext);
        }

        renderContext.UploadShipsEnd();
    }

    // AABBs
    if (renderContext.GetShowAABBs())
    {
        renderContext.UploadAABBsStart(mAllAABBs.GetCount());

        auto constexpr color = rgbaColor(18, 8, 255, 255).toVec4f();

        for (auto const & aabb : mAllAABBs.GetItems())
        {
            renderContext.UploadAABB(
                aabb,
                color);
        }

        renderContext.UploadAABBsEnd();
    }
}

}