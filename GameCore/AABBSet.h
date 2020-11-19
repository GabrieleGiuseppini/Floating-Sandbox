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

    struct ExtendedAABB : public AABB
    {
        vec2f Center;

        ExtendedAABB(AABB && aabb)
            : AABB(std::move(aabb))
            , Center(
                (TopRight.x + BottomLeft.x) / 2.0f,
                (TopRight.y + BottomLeft.y) / 2.0f)
        {}
    };

public:

    inline size_t GetCount() const noexcept
    {
        return mAABBs.size();
    }

    inline std::vector<ExtendedAABB> const & GetItems() const noexcept
    {
        return mAABBs;
    }

    // Note: at this moment we assume that we don't need to track AABBs back to
    // their origin (being ships or whatever else);
    // if and when that is not the case anymore, then we will change the signature
    inline void Add(AABB && aabb) noexcept
    {
        mAABBs.emplace_back(std::move(aabb));
    }

    void Clear()
    {
        mAABBs.clear();
    }

private:

    std::vector<ExtendedAABB> mAABBs;
};

}