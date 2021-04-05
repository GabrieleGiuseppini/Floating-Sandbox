/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <memory>

namespace Physics
{

class Stars
{
public:

    Stars()
        : mStars()
        , mIsDirtyForRendering(false)
    {}

    void Update(GameParameters const & gameParameters)
    {
        if (mStars.size() != gameParameters.NumberOfStars)
        {
            GenerateStars(gameParameters.NumberOfStars);

            mIsDirtyForRendering = true;
        }
    }

    void Upload(Render::RenderContext & renderContext) const
    {
        if (mIsDirtyForRendering)
        {
            renderContext.UploadStarsStart(mStars.size());

            for (auto const & star : mStars)
            {
                renderContext.UploadStar(star.ndcX, star.ndcY, star.brightness);
            }

            renderContext.UploadStarsEnd();

            mIsDirtyForRendering = false;
        }
    }

private:

    void GenerateStars(unsigned int NumberOfStars);

private:

    struct Star
    {
        float ndcX;
        float ndcY;
        float brightness;

        Star(
            float _ndcX,
            float _ndcY,
            float _brightness)
            : ndcX(_ndcX)
            , ndcY(_ndcY)
            , brightness(_brightness)
        {}
    };

    std::vector<Star> mStars;

    mutable bool mIsDirtyForRendering;
};

}
