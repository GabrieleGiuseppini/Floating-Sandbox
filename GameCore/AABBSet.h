/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-11-18
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AABB.h"
#include "Vectors.h"

#include <algorithm>
#include <optional>
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

    inline bool Contains(vec2f const & point) const noexcept
    {
        return std::any_of(
            mAABBs.cbegin(),
            mAABBs.cend(),
            [&point](auto const & aabb)
            {
                return aabb.Contains(point);
            });
    }

    inline bool Contains(
        vec2f const & point,
        float margin) const noexcept
    {
        return std::any_of(
            mAABBs.cbegin(),
            mAABBs.cend(),
            [&point, &margin](auto const & aabb)
            {
                return aabb.Contains(point, margin);
            });
    }

    inline std::optional<AABB> MakeUnion() const
    {
        if (mAABBs.empty())
        {
            return std::nullopt;
        }
        else
        {
            AABB result;

            std::for_each(
                mAABBs.cbegin(),
                mAABBs.cend(),
                [&result](AABB const & elem)
                {
                    result.ExtendTo(elem);
                });

            return result;
        }
    }

    // Note: at this moment we assume that we don't need to track AABBs back to
    // their origin (being ships or whatever else);
    // if and when that is not the case anymore, then we will change the signature
    inline void Add(AABB const & aabb) noexcept
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