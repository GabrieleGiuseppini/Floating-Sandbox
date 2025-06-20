#include "Utils.h"

#include <Core/Algorithms.h>
#include <Core/SysSpecifics.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

static void MakeAABBWeightedUnion_Naive(benchmark::State & state)
{
    std::vector<Geometry::ShipAABB> aabbs;
    aabbs.reserve(SampleSize);
    for (size_t i = 0; i < SampleSize; ++i)
    {
        vec2f topRight(
            static_cast<float>((i * 79) % 133) + static_cast<float>((i % 17) / 17.0f),
            static_cast<float>((i * 61) % 119) + static_cast<float>((i % 27) / 27.0f));

        vec2f bottomLeft(
            -static_cast<float>((i * 47) % 129) + static_cast<float>((i % 17) / 17.0f),
            -static_cast<float>((i * 59) % 207) + static_cast<float>((i % 27) / 27.0f));

        ElementCount frontierEdgeCount = static_cast<ElementCount>((i * 97) % 101);

        aabbs.emplace_back(
            Geometry::ShipAABB(
                topRight,
                bottomLeft,
                frontierEdgeCount));
    }

    std::optional<Geometry::AABB> result;
    for (auto _ : state)
    {
        result = Algorithms::MakeAABBWeightedUnion_Naive(aabbs);
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(MakeAABBWeightedUnion_Naive);
