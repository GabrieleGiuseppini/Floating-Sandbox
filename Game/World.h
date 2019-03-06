/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "ShipDefinition.h"

#include <GameCore/AABB.h>
#include <GameCore/Vectors.h>

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

namespace Physics
{

class World
{
public:

    World(
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        GameParameters const & gameParameters,
        ResourceLoader & resourceLoader);

    ShipId AddShip(
        ShipDefinition const & shipDefinition,
        MaterialDatabase const & materialDatabase,
        GameParameters const & gameParameters);

    size_t GetShipCount() const;

    size_t GetShipPointCount(ShipId shipId) const;

    inline float GetWaterHeightAt(float x) const
    {
        return mWaterSurface.GetWaterHeightAt(x);
    }

    inline bool IsUnderwater(vec2f const & position) const
    {
        return position.y < GetWaterHeightAt(position.x);
    }

    inline float GetOceanFloorHeightAt(float x) const
    {
        return mOceanFloor.GetFloorHeightAt(x);
    }

    inline vec2f const & GetCurrentWindSpeed() const
    {
        return mWind.GetCurrentWindSpeed();
    }

    void MoveBy(
        ShipId shipId,
        vec2f const & offset,
        GameParameters const & gameParameters);

    void RotateBy(
        ShipId shipId,
        float angle,
        vec2f const & center,
        GameParameters const & gameParameters);

    void DestroyAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        GameParameters const & gameParameters);

    void SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strength,
        GameParameters const & gameParameters);

    void SwirlAt(
        vec2f const & targetPos,
        float strength,
        GameParameters const & gameParameters);

    void TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool InjectBubblesAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        GameParameters const & gameParameters);

    void ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

    bool AdjustOceanFloorTo(
        float x,
        float targetY);

    std::optional<ObjectId> GetNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void QueryNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void Update(
        GameParameters const & gameParameters,
        Render::RenderContext const & renderContext);

    void Render(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

private:

    void UploadLandAndOcean(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

private:

    // Repository
    std::vector<std::unique_ptr<Ship>> mAllShips;
    Stars mStars;
    Clouds mClouds;
    WaterSurface mWaterSurface;
    OceanFloor mOceanFloor;
    Wind mWind;

    // The current simulation time
    float mCurrentSimulationTime;

    // The current step sequence number; used to avoid zero-ing out things.
    // Guaranteed to never be zero, but expected to rollover
    VisitSequenceNumber mCurrentVisitSequenceNumber;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;
};

}
