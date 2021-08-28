/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

namespace ShipBuilder {

/*
 * This is an anemic object model containing the whole representation of the ship being built.
 */
class Model
{
public:

    Model(WorkSpaceSize const & workSpaceSize);

    WorkSpaceSize const & GetWorkSpaceSize() const
    {
        return mWorkSpaceSize;
    }

private:

    WorkSpaceSize mWorkSpaceSize;
};

}