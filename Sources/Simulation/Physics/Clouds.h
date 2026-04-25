/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <Core/Buffer.h>
#include <Core/GameMath.h>
#include <Core/GameRandomEngine.h>
#include <Core/PrecalculatedFunction.h>

#include <cmath>
#include <deque>
#include <memory>
#include <vector>

namespace Physics
{

class Clouds
{

public:

    Clouds();

    void UpdateTornadoClouds(float visibilityAlpha)
    {
        mTornadoCloudsAlpha = visibilityAlpha;
    }

    void Update(
        float currentSimulationTime,
        float baseAndStormSpeedMagnitude,
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters);

    void Upload(RenderContext & renderContext);

private:

    struct Cloud;

    inline void InitializeTornadoCloud(Cloud & cloud);

    inline void UpdateShadows(
        std::vector<std::unique_ptr<Cloud>> const & clouds,
        ViewModel const & viewModel);

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
        float Y; // 0.0 -> 1.0 (above horizon) | For Tornado clouds, -1.0 -> 1.0, to be mapped by Render into screen band
        float Z; // 0.0 -> 1.0
        float Scale;
        float Darkening; // 0.0: dark, 1.0: light
        float Alpha;
        float VolumetricGrowthProgress; // 0.0 -> +INF; used for "volumetric" growth
        float LinearSpeedX;

        Cloud(
            uint32_t id,
            float initialX,
            float y,
            float z,
            float scale,
            float darkening,
            float alpha,
            float volumetricGrowthProgress,
            float linearSpeedX)
            : Id(id)
            , X(initialX)
            , Y(y)
            , Z(z)
            , Scale(scale)
            , Darkening(darkening)
            , Alpha(alpha)
            , VolumetricGrowthProgress(volumetricGrowthProgress)
            , LinearSpeedX(linearSpeedX)
        {
        }

        inline void Update(float globalCloudSpeed)
        {
            float const dx = LinearSpeedX * globalCloudSpeed * SimulationParameters::SimulationStepTimeDuration<float>;

            // Update position
            X += dx;

            // Update progress: mix of time and traveled step
            VolumetricGrowthProgress += SimulationParameters::SimulationStepTimeDuration<float> + std::abs(dx) * 3.5f;
        }
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;
    std::deque<std::unique_ptr<Cloud>> mTornadoClouds;

    size_t mLastTornadoBandChosen; // 0..2

    float mTornadoCloudsAlpha;

    //
    // Shadows
    //

    Buffer<float> mShadowBuffer;
};

}
