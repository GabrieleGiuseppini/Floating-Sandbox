/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-11-18
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AABB.h"
#include "Algorithms.h"
#include "Vectors.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace Geometry {

// Set of AABBs
template<typename TElement>
class _AABBSet
{
public:

    inline size_t GetCount() const noexcept
    {
        return mAABBs.size();
    }

    inline std::vector<TElement> const & GetItems() const noexcept
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

    inline std::optional<TElement> MakeUnion() const
    {
        if (mAABBs.empty())
        {
            return std::nullopt;
        }
        else
        {
            TElement result;

            std::for_each(
                mAABBs.cbegin(),
                mAABBs.cend(),
                [&result](TElement const & elem)
                {
                    result.ExtendTo(elem);
                });

            return result;
        }
    }

    inline std::optional<TElement> MakeUnion(float minArea) const
    {
        TElement result;

        std::for_each(
            mAABBs.cbegin(),
            mAABBs.cend(),
            [&result, &minArea](AABB const & elem)
            {
                if (elem.CalculateArea() >= minArea)
                {
                    result.ExtendTo(elem);
                }
            });

        if (result.GetWidth() > 0.0f && result.GetHeight() > 0.0f)
        {
            return result;
        }
        else
        {
            return std::nullopt;
        }
    }

    // Note: at this moment we assume that we don't need to track AABBs back to
    // their origin (being ships or whatever else);
    // if and when that is not the case anymore, then we will change the signature
    inline void Add(TElement const & aabb) noexcept
    {
        mAABBs.emplace_back(aabb);
    }

    void Clear()
    {
        mAABBs.clear();
    }

protected:

    std::vector<TElement> mAABBs;
};

class AABBSet: public _AABBSet<AABB>
{ };

class ShipAABBSet : public _AABBSet<ShipAABB>
{
public:

    inline std::optional<AABB> MakeWeightedUnion() const
    {
        return Algorithms::MakeAABBWeightedUnion(mAABBs);
    }
};

}