/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "GameTypes.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "RunningAverage.h"
#include "ShipDefinition.h"
#include "Vectors.h"

#include <optional>
#include <vector>

namespace Physics
{

class Ship 
    : public Bomb::IPhysicsHandler
{
public:

    Ship(
        int id,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        Points && points,
        Springs && springs,
        Triangles && triangles,
        ElectricalElements && electricalElements,
        std::shared_ptr<MaterialDatabase> materialDatabase,
        VisitSequenceNumber currentVisitSequenceNumber);

    ~Ship();

    unsigned int GetId() const { return mId; }

    World const & GetParentWorld() const { return mParentWorld; }
    World & GetParentWorld() { return mParentWorld; }

    size_t GetPointCount() const { return mPoints.GetElementCount(); }

    auto const & GetPoints() const { return mPoints; }
    auto & GetPoints() { return mPoints; }

    auto const & GetSprings() const { return mSprings; }
    auto & GetSprings() { return mSprings; }

    auto const & GetTriangles() const { return mTriangles; }
    auto & GetTriangles() { return mTriangles; }

    auto const & GetElectricalElements() const { return mElectricalElements; }
    auto & GetElectricalElements() { return mElectricalElements; }

    void DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strength);

    void SwirlAt(
        vec2f const & targetPos,
        float strength);

    bool TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

    ElementIndex GetNearestPointIndexAt(
        vec2f const & targetPos,
        float radius) const;

    void Update(
        float currentSimulationTime,
        VisitSequenceNumber currentVisitSequenceNumber,
        GameParameters const & gameParameters);

    void Render(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

public:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    // Mechanical

    void UpdateMechanicalDynamics(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void UpdatePointForces(GameParameters const & gameParameters);

    void UpdateSpringForces(GameParameters const & gameParameters);    

    void IntegrateAndResetPointForces();

    void HandleCollisionsWithSeaFloor();

    // Water

    void UpdateWaterDynamics(GameParameters const & gameParameters);

    void UpdateWaterInflow(
        GameParameters const & gameParameters,
        float & waterTaken);

    void UpdateWaterVelocities(
        GameParameters const & gameParameters,
        float & waterSplashed);

    // Electrical 

    void UpdateElectricalDynamics(
        GameWallClock::time_point currentWallclockTime,
        VisitSequenceNumber currentVisitSequenceNumber,
        GameParameters const & gameParameters);

    void UpdateElectricalConnectivity(VisitSequenceNumber currentVisitSequenceNumber);

    void DiffuseLight(GameParameters const & gameParameters);

    // Ephemeral particles

    void UpdateEphemeralParticles(
        float currentSimulationTime,
        GameParameters const & gameParameters);

private:

    void DetectConnectedComponents(VisitSequenceNumber currentVisitSequenceNumber);

    void DestroyConnectedTriangles(ElementIndex pointElementIndex);

    void DestroyConnectedTriangles(
        ElementIndex pointAElementIndex,
        ElementIndex pointBElementIndex);

    void PointDestroyHandler(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void SpringDestroyHandler(
        ElementIndex springElementIndex,
        bool destroyAllTriangles,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void TriangleDestroyHandler(ElementIndex triangleElementIndex);

    void ElectricalElementDestroyHandler(ElementIndex electricalElementIndex);

private:

    /////////////////////////////////////////////////////////////////////////
    // Bomb::IPhysicsHandler
    /////////////////////////////////////////////////////////////////////////

    virtual void DoBombExplosion(
        vec2f const & blastPosition,
        float sequenceProgress,
        ConnectedComponentId connectedComponentId,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombPreimplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombImplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) override;

    virtual void DoAntiMatterBombExplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) override;

private:

    unsigned int const mId;
    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // All the ship elements - never removed, the repositories maintain their own size forever
    Points mPoints;
    Springs mSprings;
    Triangles mTriangles;
    ElectricalElements mElectricalElements;

    // The material database
    std::shared_ptr<MaterialDatabase> const mMaterialDatabase;

    // Connected components metadata
    std::vector<std::size_t> mConnectedComponentSizes;

    // Flag remembering whether points (elements) and/or springs (incl. ropes) and/or triangles have changed
    // since the last step.
    // When this flag is set, we'll re-detect connected components and re-upload elements
    // to the rendering context
    bool mutable mAreElementsDirty;

    // Sinking detection
    bool mIsSinking;

    // Water
    float mTotalWater;
    RunningAverage<30> mWaterSplashedRunningAverage;

    // Pinned points
    PinnedPoints mPinnedPoints;

    // Bombs
    Bombs mBombs;

    // Force fields to apply at next iteration
    std::vector<std::unique_ptr<ForceField>> mCurrentForceFields;
};

}