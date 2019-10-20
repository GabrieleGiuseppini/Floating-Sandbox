/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

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
                cloud->Scale,
                cloud->Darkening);
        }

        for (auto const & cloud : mStormClouds)
        {
            renderContext.UploadCloud(
                cloud->Id,
                cloud->X,
                cloud->Y,
                cloud->Scale,
                cloud->Darkening);
        }

        renderContext.UploadCloudsEnd();
    }

private:

    struct Cloud
    {
    public:

        uint32_t const Id; // Not consecutive, only guaranteed to be sticky and unique across all clouds
        float X;
        float Y;
        float Scale;
        float Darkening; // 0.0: dark, 1.0: light

        Cloud(
            uint32_t id,
            float initialX,
            float initialY,
            float initialScale,
            float darkening)
            : Id(id)
            , X(initialX)
            , Y(initialY)
            , Scale(initialScale)
            , Darkening(darkening)
            , mLinearSpeedX(GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f))
            , mPeriodicSpeedXAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00006f)
            , mPeriodicSpeedXPeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.01f)
            , mPeriodicSpeedYAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00007f)
            , mPeriodicSpeedYPeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.005f)
            , mPeriodicSpeedScaleAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.0003f)
            , mPeriodicSpeedScalePeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.002f)
        {
        }

        inline void Update(
            float currentSimulationTime,
            float cloudSpeed)
        {
            float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

            X += (mLinearSpeedX * cloudSpeed * dt) + (mPeriodicSpeedXAmp * sinf(mPeriodicSpeedXPeriod * cloudSpeed * currentSimulationTime));
            Y += (mPeriodicSpeedYAmp * sinf(mPeriodicSpeedYPeriod * cloudSpeed * currentSimulationTime));
            Scale += (mPeriodicSpeedScaleAmp * sinf(mPeriodicSpeedScalePeriod * cloudSpeed * currentSimulationTime));
        }

    private:

        float const mLinearSpeedX;
        float const mPeriodicSpeedXAmp;
        float const mPeriodicSpeedXPeriod;

        float const mPeriodicSpeedYAmp;
        float const mPeriodicSpeedYPeriod;

        float const mPeriodicSpeedScaleAmp;
        float const mPeriodicSpeedScalePeriod;
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;
};

}
