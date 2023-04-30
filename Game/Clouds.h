/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>

#include <cmath>
#include <memory>
#include <vector>

namespace Physics
{

class Clouds
{

public:

    Clouds();

    void Update(
        float currentSimulationTime,
        float baseAndStormSpeedMagnitude,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

private:

    struct Cloud;

    void UpdateShadows(std::vector<std::unique_ptr<Cloud>> const & clouds);

private:

    //
    // Clouds
    //

    struct Cloud
    {
    public:

        uint32_t const Id; // Not consecutive, only guaranteed to be sticky and unique across all clouds (used as texture frame index)
        float X; // NDC
        float const Y; // 0.0 -> 1.0 (above horizon)
        float const Z; // 0.0 -> 1.0
        float Scale;
        float Darkening; // 0.0: dark, 1.0: light
        float GrowthProgress;

        Cloud(
            uint32_t id,
            float initialX,
            float y,
            float z,
            float scale,
            float darkening,
            float linearSpeedX,
            float initialGrowthProgressPhase)
            : Id(id)
            , X(initialX)
            , Y(y)
            , Z(z)
            , Scale(scale)
            , Darkening(darkening)
            , GrowthProgress(0.0f)
            , mLinearSpeedX(linearSpeedX)
            , mGrowthProgressPhase(initialGrowthProgressPhase)
        {
        }

        inline void Update(float globalCloudSpeed)
        {
            X += mLinearSpeedX * globalCloudSpeed * GameParameters::SimulationStepTimeDuration<float>;

            float const growthProgressSpeed =
                (1.0f / 45.0f) // Basal velocity
                + std::abs(globalCloudSpeed) / (400.0f);

            mGrowthProgressPhase += growthProgressSpeed * GameParameters::SimulationStepTimeDuration<float>;

            GrowthProgress = 0.3f + (1.0f + std::sin(mGrowthProgressPhase * Pi<float> * 2.0f)) * 0.7f / 2.0f;
        }

    private:

        float const mLinearSpeedX;
        float mGrowthProgressPhase;
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;

    //
    // Shadows
    //

    Buffer<float> mShadowBuffer;
};

}
