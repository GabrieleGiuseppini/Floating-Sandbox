/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>

#include <memory>
#include <vector>

namespace Physics
{

class Clouds
{

public:

    Clouds()
        : mLastCloudId(0)
        , mClouds()
        , mStormClouds()
    {}

    void Update(
        float currentSimulationTime,
        float baseAndStormSpeedMagnitude,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const
    {
        renderContext.UploadCloudsStart(mClouds.size() + mStormClouds.size());

        for (auto const & cloud : mClouds)
        {
            renderContext.UploadCloud(
                cloud->Id,
                cloud->X,
                cloud->Y,
                cloud->Z,
                cloud->Scale,
                cloud->Darkening,
                cloud->GrowthProgress);
        }

        for (auto const & cloud : mStormClouds)
        {
            renderContext.UploadCloud(
                cloud->Id,
                cloud->X,
                cloud->Y,
                cloud->Z,
                cloud->Scale,
                cloud->Darkening,
                cloud->GrowthProgress);
        }

        renderContext.UploadCloudsEnd();
    }

private:

    struct Cloud
    {
    public:

        uint32_t const Id; // Not consecutive, only guaranteed to be sticky and unique across all clouds (used as texture frame index)
        float X;
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
            float growthProgressPhase)
            : Id(id)
            , X(initialX)
            , Y(y)
            , Z(z)
            , Scale(scale)
            , Darkening(darkening)
            , GrowthProgress(0.0f)
            , mLinearSpeedX(linearSpeedX)
            , mGrowthProgressPhase(growthProgressPhase)
        {
        }

        inline void Update(
            float simulationTime,
            float globalCloudSpeed)
        {
            X += mLinearSpeedX * globalCloudSpeed * GameParameters::SimulationStepTimeDuration<float>;

            GrowthProgress = (1.0f + std::sinf((mGrowthProgressPhase + simulationTime / 20.0f) * Pi<float> * 2.0f)) / 2.0f;
        }

    private:

        float const mLinearSpeedX;
        float const mGrowthProgressPhase; // 0.0 -> 1.0
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;
};

}
