/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <memory>
#include <vector>

namespace Physics
{

class Clouds
{

public:

    Clouds()
        : mClouds()
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
                cloud->GetX(),
                cloud->GetY(),
                cloud->GetScale());
        }

        for (auto const & cloud : mStormClouds)
        {
            renderContext.UploadCloud(
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
            float offsetX,
            float speedX1,
            float ampX,
            float speedX2,
            float offsetY,
            float ampY,
            float speedY,
            float offsetScale,
            float ampScale,
            float speedScale)
            : mX(offsetX)
            , mY(offsetY)
            , mScale(offsetScale)
            , mSpeedX1(speedX1)
            , mAmpX(ampX)
            , mSpeedX2(speedX2)
            , mAmpY(ampY)
            , mSpeedY(speedY)
            , mAmpScale(ampScale)
            , mSpeedScale(speedScale)
        {
        }

        inline void Update(
            float currentSimulationTime,
            float cloudSpeed)
        {
            float constexpr dt = GameParameters::SimulationStepTimeDuration<float>;

            mX += (mSpeedX1 * cloudSpeed * dt) + (mAmpX * sinf(mSpeedX2 * cloudSpeed * currentSimulationTime));
            mY += (mAmpY * sinf(mSpeedY * cloudSpeed * currentSimulationTime));
            mScale += (mAmpScale * sinf(mSpeedScale * cloudSpeed * currentSimulationTime));
        }

        inline float GetX() const
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

        float mX;
        float mY;
        float mScale;

        float const mSpeedX1;
        float const mAmpX;
        float const mSpeedX2;

        float const mAmpY;
        float const mSpeedY;

        float const mAmpScale;
        float const mSpeedScale;
    };

    std::vector<std::unique_ptr<Cloud>> mClouds;
    std::vector<std::unique_ptr<Cloud>> mStormClouds;

    // Updated at Update()
    float mCloudDarkening;
};

}
