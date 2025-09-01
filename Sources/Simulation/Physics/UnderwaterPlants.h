/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationParameters.h"

#include <Render/GameTextureDatabases.h>
#include <Render/RenderContext.h>

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <vector>

namespace Physics
{

class UnderwaterPlants
{

public:

    explicit UnderwaterPlants(size_t speciesCount);

    void Update(
        float currentSimulationTime,
        Wind const & wind,
        OceanSurface const & oceanSurface,
        OceanFloor const & oceanFloor,
        SimulationParameters const & simulationParameters);

    void Upload(RenderContext & renderContext);

private:

    void RepopulatePlants(
        OceanSurface const & oceanSurface,
        OceanFloor const & oceanFloor,
        SimulationParameters const & simulationParameters);

    void RecalculateBottomYs(OceanFloor const & oceanFloor);

    void RecalculateScales(float sizeMultiplier);

private:

    size_t const mSpeciesCount;

    //
    // Container
    //

    struct Plant
    {
        float const CenterX;
        float BottomY;

        size_t const SpeciesIndex;
        float const BasisScale;
        float Scale;
        float const PersonalitySeed;
        bool const IsSpecular;

        Plant(
            float centerX,
            size_t speciesIndex,
            float basisScale,
            float personalitySeed,
            bool isSpecular)
            : CenterX(centerX)
            , BottomY(0.0f) // Will be recalculated
            , SpeciesIndex(speciesIndex)
            , BasisScale(basisScale)
            , Scale(basisScale) // Will be recalculated
            , PersonalitySeed(personalitySeed)
            , IsSpecular(isSpecular)
        {}
    };

    std::vector<Plant> mPlants;

    bool mArePlantsDirtyForRendering;

    std::vector<OceanSurface::CoordinatesProxy> mOceanSurfaceCoordinatesProxies;

    std::vector<float> mOceanDepths;

    //
    // Calculated values
    //

    float mCurrentRotationAngle;
    bool mIsCurrentRotationAngleDirtyForRendering;

    //
    // Parameters that the calculated values are current with
    //

    float mCurrentDensity;
    float mCurrentSizeMultiplier;
    float mCurrentWindBaseSpeedMagnitude;
};

}
