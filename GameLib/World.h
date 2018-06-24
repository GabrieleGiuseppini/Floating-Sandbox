/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AABB.h"
#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ShipDefinition.h"
#include "Vectors.h"

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
        GameParameters const & gameParameters);

    int AddShip(
        ShipDefinition const & shipDefinition,
        MaterialDatabase const & materials,
        GameParameters const & gameParameters);

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

	void DestroyAt(
		vec2 const & targetPos, 
		float radius);

    void SawThrough(
        vec2 const & startPos,
        vec2 const & endPos);

	void DrawTo(
        vec2 const & targetPos,
        float strength);

    void SwirlAt(
        vec2 const & targetPos,
        float strength);

    void TogglePinAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    void ToggleTimerBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    void ToggleRCBombAt(
        vec2 const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    ElementIndex GetNearestPointAt(
        vec2 const & targetPos,
        float radius) const;

	void Update(GameParameters const & gameParameters);

	void Render(		
        GameParameters const & gameParameters,
		RenderContext & renderContext) const;

private:

    void UpdateClouds(GameParameters const & gameParameters);

    void RenderClouds(RenderContext & renderContext) const;

	void UploadLandAndWater(
		GameParameters const & gameParameters,
        RenderContext & renderContext) const;

private:

	// Repository
	std::vector<std::unique_ptr<Ship>> mAllShips;
    std::vector<std::unique_ptr<Cloud>> mAllClouds;
    WaterSurface mWaterSurface;
    OceanFloor mOceanFloor;

	// The current time 
	float mCurrentTime;

    // The current step sequence number; used to avoid zero-ing out things.
    // Guaranteed to never be zero, but expected to rollover
    uint64_t mCurrentStepSequenceNumber;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;
};

}
