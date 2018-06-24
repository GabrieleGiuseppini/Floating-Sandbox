/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"

/*
 * A singleton generating unique object IDs.
 *
 * No object ID is ever generated twice.
 */
class ObjectIdGenerator
{
public:

    static ObjectIdGenerator & GetInstance()
    {
        static ObjectIdGenerator instance;
        
        return instance;
    }

    inline ObjectId Generate()
    {
        return mNextObjectId++;
    }

private:
    
    ObjectIdGenerator()
        : mNextObjectId(1)
    {
    }

    ObjectId mNextObjectId;
};
