/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "GameTypes.h"
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
        ShipId id,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        Points && points,
        Springs && springs,
        Triangles && triangles,
        ElectricalElements && electricalElements,
        VisitSequenceNumber currentVisitSequenceNumber);

    ~Ship();

    ShipId GetId() const { return mId; }

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

    void MoveBy(
        vec2f const & offset,
        GameParameters const & gameParameters);

    void RotateBy(
        float angle,
        vec2f const & center,
        GameParameters const & gameParameters);

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
        float strength,
        GameParameters const & gameParameters);

    void SwirlAt(
        vec2f const & targetPos,
        float strength,
        GameParameters const & gameParameters);

    bool TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool ToggleTimerBombAt(
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
        GameParameters const & gameParameters,
        Render::RenderContext const & renderContext);

    void Render(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext);

public:

    /////////////////////////////////////////////////////////////////////////
    // Dynamics
    /////////////////////////////////////////////////////////////////////////

    // Mechanical

    void UpdateMechanicalDynamics(
        float currentSimulationTime,
        GameParameters const & gameParameters,
        Render::RenderContext const & renderContext);

    void UpdatePointForces(GameParameters const & gameParameters);

    void UpdateSpringForces(GameParameters const & gameParameters);

    void IntegrateAndResetPointForces(GameParameters const & gameParameters);

    void HandleCollisionsWithSeaFloor(GameParameters const & gameParameters);

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

    void GenerateDebris(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void GenerateSparkles(
        ElementIndex springElementIndex,
        vec2f const & cutDirectionStartPos,
        vec2f const & cutDirectionEndPos,
        float currentSimulationTime,
        GameParameters const & gameParameters);

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

#ifdef _DEBUG
    void VerifyInvariants();
#endif

private:

    ShipId const mId;
    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

    // All the ship elements - never removed, the repositories maintain their own size forever
    Points mPoints;
    Springs mSprings;
    Triangles mTriangles;
    ElectricalElements mElectricalElements;

    // Connected components metadata
    std::vector<std::size_t> mConnectedComponentSizes;

    // Flag remembering whether points (elements) and/or springs (incl. ropes) and/or triangles have changed
    // since the last step.
    // When this flag is set, we'll re-detect connected components and re-upload elements
    // to the rendering context
    bool mAreElementsDirty;

    // The ship render mode that was in effect the last time we've uploaded elements;
    // used to detect changes and eventually re-upload
    std::optional<ShipRenderMode> mLastShipRenderMode;

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