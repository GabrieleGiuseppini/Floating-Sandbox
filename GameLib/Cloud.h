/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include "Vectors.h"

namespace Physics
{	

class Cloud
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
        float t, 
        float windSpeed)
    {
        float const scaledSpeed = t * windSpeed;

        mX = mOffsetX + (mSpeedX1 * scaledSpeed) + (mAmpX * sinf(mSpeedX2 * scaledSpeed));
        mY = mOffsetY + (mAmpY * sinf(mSpeedY * scaledSpeed));
        mScale = mOffsetScale + (mAmpScale * sinf(mSpeedScale * scaledSpeed));
    }

    float GetX() const
    {
        return mX;
    }

    float GetY() const
    {
        return mY;
    }

    float GetScale() const
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

}
