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
                cloud->Z,
                cloud->Scale,
                cloud->Darkening);
        }

        for (auto const & cloud : mStormClouds)
        {
            renderContext.UploadCloud(
                cloud->Id,
                cloud->X,
                cloud->Z,
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
        float const Z; // 0.0 -> 1.0
        float Scale;
        float Darkening; // 0.0: dark, 1.0: light

        Cloud(
            uint32_t id,
            float initialX,
            float z,
            float scale,
            float darkening)
            : Id(id)
            , X(initialX)
            , Z(z)
            , Scale(scale)
            , Darkening(darkening)
            , mLinearSpeedX(GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f))
            , mPeriodicSpeedXAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00006f)
            , mPeriodicSpeedXPeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.01f)
        {
        }

        inline void Update(
            float currentSimulationTime,
            float cloudSpeed)
        {
            float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

            X += (mLinearSpeedX * cloudSpeed * dt) + (mPeriodicSpeedXAmp * sinf(mPeriodicSpeedXPeriod * cloudSpeed * currentSimulationTime));
        }

    private:

        float const mLinearSpeedX;
        float const mPeriodicSpeedXAmp;
        float const mPeriodicSpeedXPeriod;
    };

    uint32_t mLastCloudId;

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;
};

}
