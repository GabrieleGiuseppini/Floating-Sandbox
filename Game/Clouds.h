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
#include <GameCore/PrecalculatedFunction.h>

#include <cmath>
#include <memory>
#include <vector>

namespace Physics
{

class Clouds
{

public:

    explicit Clouds(bool areShadowsEnabled);

    void SetShadowsEnabled(bool value);

    void Update(
        float currentSimulationTime,
        float baseAndStormSpeedMagnitude,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

private:

    struct Cloud;

    inline void UpdateShadows(std::vector<std::unique_ptr<Cloud>> const & clouds);
    inline void OffsetShadowsBuffer_Mean();
    inline void OffsetShadowsBuffer_Min();

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
        float VolumetricGrowthProgress; // 0.0 -> +INF; used for "volumetric" growth

        Cloud(
            uint32_t id,
            float initialX,
            float y,
            float z,
            float scale,
            float darkening,
            float volumetricGrowthProgress,
            float linearSpeedX)
            : Id(id)
            , X(initialX)
            , Y(y)
            , Z(z)
            , Scale(scale)
            , Darkening(darkening)
            , VolumetricGrowthProgress(volumetricGrowthProgress)
            , mLinearSpeedX(linearSpeedX)
        {
        }

        inline void Update(float globalCloudSpeed)
        {
            float const dx = mLinearSpeedX * globalCloudSpeed * GameParameters::SimulationStepTimeDuration<float>;

            // Update position
            X += dx;

            // Update progress: mix of time and traveled step
            VolumetricGrowthProgress += GameParameters::SimulationStepTimeDuration<float> + std::abs(dx) * 3.0f;
        }

    private:

        float const mLinearSpeedX;
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;

    //
    // Shadows
    //

    bool mAreShadowsEnabled; // Calculated from outside and given here; never read from here

    Buffer<float> mShadowBuffer;
};

}
