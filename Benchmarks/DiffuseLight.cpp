#include <GameCore/Algorithms.h>

#include "Utils.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t SampleSize = 20000000;

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
        Algorithms::DiffuseLight_Vectorized(
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
