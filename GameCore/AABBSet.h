/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-11-18
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AABB.h"
#include "Vectors.h"

#include <vector>

namespace Geometry {

// Set of AABBs
class AABBSet
{
public:

    inline size_t GetCount() const noexcept
    {
        return mAABBs.size();
    }

    inline std::vector<AABB> const & GetItems() const noexcept
    {
        return mAABBs;
    }

    // Note: at this moment we assume that we don't need to track AABBs back to
    // their origin (being ships or whatever else);
    // if and when that is not the case anymore, then we will change the signature
    inline void Add(AABB && aabb) noexcept
    {
        mAABBs.emplace_back(aabb);
    }

    void Clear()
    {
        mAABBs.clear();
    }

private:

    std::vector<AABB> mAABBs;
};

}