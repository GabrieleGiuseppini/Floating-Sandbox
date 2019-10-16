/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

namespace Physics
{

class Storm
{

public:

    Storm()
        : mParameters()
    {}

    void Update(GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

public:

    struct Parameters
    {
    };

    Parameters const & GetParameters() const
    {
        return mParameters;
    }

private:

    Parameters mParameters;
};

}
