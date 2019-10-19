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
        , mCloudDarkening(0.0f)
    {}

    void Update(
        float currentSimulationTime,
        float baseAndStormSpeedMagnitude,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const
    {
        renderContext.UploadCloudsStart(
            mClouds.size() + mStormClouds.size(),
            mCloudDarkening);

        for (auto const & cloud : mClouds)
        {
            renderContext.UploadCloud(
                cloud->GetId(),
                cloud->GetX(),
                cloud->GetY(),
                cloud->GetScale());
        }

        for (auto const & cloud : mStormClouds)
        {
            renderContext.UploadCloud(
                cloud->GetId(),
                cloud->GetX(),
                cloud->GetY(),
                cloud->GetScale());
        }

        renderContext.UploadCloudsEnd();
    }

private:

    struct Cloud
    {
    public:

        Cloud(
            uint32_t id,
            float initialX,
            float initialY,
            float initialScale)
            : mId(id)
            , mX(initialX)
            , mY(initialY)
            , mScale(initialScale)
            , mLinearSpeedX(GameRandomEngine::GetInstance().GenerateUniformReal(0.003f, 0.007f))
            , mPeriodicSpeedXAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00006f)
            , mPeriodicSpeedXPeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.01f)
            , mPeriodicSpeedYAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.00007f)
            , mPeriodicSpeedYPeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.005f)
            , mPeriodicSpeedScaleAmp(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.0005f)
            , mPeriodicSpeedScalePeriod(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal() * 0.002f)
        {
        }

        inline void Update(
            float currentSimulationTime,
            float cloudSpeed)
        {
            float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

            mX += (mLinearSpeedX * cloudSpeed * dt) + (mPeriodicSpeedXAmp * sinf(mPeriodicSpeedXPeriod * cloudSpeed * currentSimulationTime));
            mY += (mPeriodicSpeedYAmp * sinf(mPeriodicSpeedYPeriod * cloudSpeed * currentSimulationTime));
            mScale += (mPeriodicSpeedScaleAmp * sinf(mPeriodicSpeedScalePeriod * cloudSpeed * currentSimulationTime));
        }

        inline uint32_t GetId() const
        {
            return mId;
        }

        inline float GetX() const
        {
            return mX;
        }

        inline float & GetX()
        {
            return mX;
        }

        inline float GetY() const
        {
            return mY;
        }

        inline float GetScale() const
        {
            return mScale;
        }

    private:

        uint32_t const mId; // Not consecutive, only guaranteed to be sticky and unique across all clouds

        float mX;
        float mY;
        float mScale;

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

    // Updated at Update()
    float mCloudDarkening;
};

}
