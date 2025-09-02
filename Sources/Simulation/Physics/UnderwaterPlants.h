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

    static inline float CalculateBottomY(float x, OceanFloor const & oceanFloor);

    void RecalculateScales(float sizeMultiplier);

    static inline float CalculateScale(float basisScale, float sizeMultiplier);

private:

    size_t const mSpeciesCount;

    //
    // Container
    //

    struct Plant
    {
        float CenterX; // const
        float BottomY;

        size_t SpeciesIndex; // const
        float BasisScale; // const
        float Scale;
        float PersonalitySeed; // const
        bool IsSpecular; // const

        Plant(
            float centerX,
            float currentBottomY, // Initial
            size_t speciesIndex,
            float basisScale,
            float currentScale, // Initial
            float personalitySeed,
            bool isSpecular)
            : CenterX(centerX)
            , BottomY(currentBottomY)
            , SpeciesIndex(speciesIndex)
            , BasisScale(basisScale)
            , Scale(currentScale)
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
