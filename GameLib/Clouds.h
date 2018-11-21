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

    void Update(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void Render(Render::RenderContext & renderContext) const;

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
            : mX(0.0f)
            , mY(0.0f)
            , mScale(0.0f)
            , mOffsetX(offsetX)
            , mSpeedX1(speedX1)
            , mAmpX(ampX)
            , mSpeedX2(speedX2)
            , mOffsetY(offsetY)
            , mAmpY(ampY)
            , mSpeedY(speedY)
            , mOffsetScale(offsetScale)
            , mAmpScale(ampScale)
            , mSpeedScale(speedScale)
        {
        }

        inline void Update(
            float currentSimulationTime,
            float windSpeed)
        {
            float const scaledSpeed = currentSimulationTime * windSpeed;

            mX = mOffsetX + (mSpeedX1 * scaledSpeed) + (mAmpX * sinf(mSpeedX2 * scaledSpeed));
            mY = mOffsetY + (mAmpY * sinf(mSpeedY * scaledSpeed));
            mScale = mOffsetScale + (mAmpScale * sinf(mSpeedScale * scaledSpeed));
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

        float const mOffsetX;
        float const mSpeedX1;
        float const mAmpX;
        float const mSpeedX2;

        float const mOffsetY;
        float const mAmpY;
        float const mSpeedY;

        float const mOffsetScale;
        float const mAmpScale;
        float const mSpeedScale;
    };

    std::vector<std::unique_ptr<Cloud>> mClouds;
};

}
