/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

/*
 * The interface presented by the Ship to its subordinate elements.
 */
struct IShipStructureHandler
{
    virtual void StartExplosion(
        float currentSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastRadius,
        float blastHeat,
        ExplosionType explosionType,
        GameParameters const & gameParameters) = 0;

    //
    // Bombs
    //

    virtual void DoAntiMatterBombPreimplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) = 0;

    virtual void DoAntiMatterBombImplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) = 0;

    virtual void DoAntiMatterBombExplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) = 0;
};
