#include <GameCore/Algorithms.h>

#include "Utils.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

namespace Algorithms {

inline void DiffuseLight_Naive(
    vec2f const * pointPositions,
    PlaneId const * pointPlaneIds,
    ElementIndex const pointCount,
    vec2f const * lampPositions,
    PlaneId const * lampPlaneIds,
    float const * lampDistanceCoeffs,
    float const * lampSpreadMaxDistances,
    ElementIndex const lampCount,
    float * restrict outLightBuffer)
{
    for (ElementIndex p = 0; p < pointCount; ++p)
    {
        auto const pointPosition = pointPositions[p];
        auto const pointPlane = pointPlaneIds[p];

        float pointLight = 0.0f;

        // Go through all lamps;
        // can safely visit deleted lamps as their current will always be zero
        for (ElementIndex l = 0; l < lampCount; ++l)
        {
            if (pointPlane <= lampPlaneIds[l])
            {
                float const distance = (pointPosition - lampPositions[l]).length();

                float const newLight =
                    std::min(
                        lampDistanceCoeffs[l] * std::max(lampSpreadMaxDistances[l] - distance, 0.0f),
                        1.0f);

                // Point's light is just max, to avoid having to normalize everything to 1.0
                pointLight = std::max(
                    newLight,
                    pointLight);
            }
        }

        outLightBuffer[p] = pointLight;
    }
}

}

static void DiffuseLight_Naive(benchmark::State& state)
{
    auto const pointsSize = MakeSize(SampleSize);
    auto const lampsSize = state.range(0);

    std::vector<vec2f> pointPositions = MakeVectors(pointsSize);
    std::vector<PlaneId> pointPlaneIds = MakePlaneIds(pointsSize);
    std::vector<vec2f> lampPositions = MakeVectors(lampsSize);
    std::vector<PlaneId> lampPlaneIds = MakePlaneIds(lampsSize);
    std::vector<float> lampDistanceCoeffs = MakeFloats(lampsSize);
    std::vector<float> lampSpreadMaxDistances = MakeFloats(lampsSize);

    std::vector<float> outLightBuffer(pointsSize, 0.0f);

    for (auto _ : state)
    {
        Algorithms::DiffuseLight_Naive(
            pointPositions.data(),
            pointPlaneIds.data(),
            pointsSize,
            lampPositions.data(),
            lampPlaneIds.data(),
            lampDistanceCoeffs.data(),
            lampSpreadMaxDistances.data(),
            lampsSize,
            outLightBuffer.data());
    }

    benchmark::DoNotOptimize(outLightBuffer);
}
BENCHMARK(DiffuseLight_Naive)->Arg(10)->Arg(100);

static void DiffuseLight_Vectorized(benchmark::State & state)
{
    auto const pointsSize = MakeSize(SampleSize);
    auto const lampsSize = state.range(0);

    std::vector<vec2f> pointPositions = MakeVectors(pointsSize);
    std::vector<PlaneId> pointPlaneIds = MakePlaneIds(pointsSize);
    std::vector<vec2f> lampPositions = MakeVectors(lampsSize);
    std::vector<PlaneId> lampPlaneIds = MakePlaneIds(lampsSize);
    std::vector<float> lampDistanceCoeffs = MakeFloats(lampsSize);
    std::vector<float> lampSpreadMaxDistances = MakeFloats(lampsSize);

    std::vector<float> outLightBuffer(pointsSize, 0.0f);

    for (auto _ : state)
    {
        Algorithms::DiffuseLight(
            pointPositions.data(),
            pointPlaneIds.data(),
            pointsSize,
            lampPositions.data(),
            lampPlaneIds.data(),
            lampDistanceCoeffs.data(),
            lampSpreadMaxDistances.data(),
            lampsSize,
            outLightBuffer.data());
    }

    benchmark::DoNotOptimize(outLightBuffer);
}
BENCHMARK(DiffuseLight_Vectorized)->Arg(10)->Arg(100);
