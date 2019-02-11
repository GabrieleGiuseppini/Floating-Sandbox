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
        , mIsDirty(false)
    {}

    void Update(GameParameters const & gameParameters)
    {
        if (mStars.size() != gameParameters.NumberOfStars)
        {
            GenerateStars(gameParameters.NumberOfStars);

            mIsDirty = true;
        }
    }

    void Upload(Render::RenderContext & renderContext) const
    {
        if (mIsDirty)
        {
            renderContext.UploadStarsStart(mStars.size());

            for (auto const & star : mStars)
            {
                renderContext.UploadStar(star.ndcX, star.ndcY, star.brightness);
            }

            renderContext.UploadStarsEnd();

            mIsDirty = false;
        }
    }

private:

    void GenerateStars(size_t NumberOfStars);

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

    mutable bool mIsDirty;
};

}
