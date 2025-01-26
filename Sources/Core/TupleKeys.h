/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <tuple>

namespace /* anonymous */ {

    template<typename Tuple, std::size_t... ids>
    std::size_t tupleHash(Tuple const & tuple, std::index_sequence<ids...> const &);

    template<typename T>
    std::size_t hashValue(T const & value)
    {
        std::hash<T> hasher;
        return hasher(value);
    }

    template<typename... Args>
    std::size_t hashValue(std::tuple<Args...> const & value)
    {
        return tupleHash(value, std::make_index_sequence<sizeof...(Args)>());
    }

    ////// Special case for the empty tuple
    ////std::size_t hashValue(std::tuple<> const & /*value*/)
    ////{
    ////    return 1;
    ////}

    template<typename Tuple, std::size_t... ids>
    std::size_t tupleHash(Tuple const & tuple, std::index_sequence<ids...> const &)
    {
        std::size_t  result = 0;
        for (auto const & hash : { hashValue(std::get<ids>(tuple))... })
        {
            result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
        }

        return result;
    };
}

template<typename Tuple>
struct TupleHasher
{
    std::size_t operator()(Tuple const & t) const noexcept
    {
        return hashValue(t);
    }
};

template <typename Tuple, typename Value>
using unordered_tuple_map = std::unordered_map<Tuple, Value, TupleHasher<Tuple>>;

template <typename Tuple>
using unordered_tuple_set = std::unordered_set<Tuple, TupleHasher<Tuple>>;