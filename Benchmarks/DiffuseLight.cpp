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

    auto pointPositions = MakeVectors(pointsSize);
    auto pointPlaneIds = MakePlaneIds(pointsSize);
    auto lampPositions = MakeVectors(lampsSize);
    auto lampPlaneIds = MakePlaneIds(lampsSize);
    auto lampDistanceCoeffs = MakeFloats(lampsSize);
    auto lampSpreadMaxDistances = MakeFloats(lampsSize);

    auto outLightBuffer = make_unique_buffer_aligned_to_vectorization_word<float>(pointsSize);

    for (auto _ : state)
    {
        Algorithms::DiffuseLight_Naive(
            pointPositions.get(),
            pointPlaneIds.get(),
            ElementIndex(pointsSize),
            lampPositions.get(),
            lampPlaneIds.get(),
            lampDistanceCoeffs.get(),
            lampSpreadMaxDistances.get(),
            ElementIndex(lampsSize),
            outLightBuffer.get());
    }

    benchmark::DoNotOptimize(outLightBuffer);
}
//BENCHMARK(DiffuseLight_Naive)->Arg(2)->Arg(10)->Arg(20)->Arg(50)->Arg(100);
BENCHMARK(DiffuseLight_Naive)->Arg(2)->Arg(4)->Arg(8);

static void DiffuseLight_Vectorized(benchmark::State & state)
{
    auto const pointsSize = MakeSize(SampleSize);
    auto const lampsSize = state.range(0);

    auto pointPositions = MakeVectors(pointsSize);
    auto pointPlaneIds = MakePlaneIds(pointsSize);
    auto lampPositions = MakeVectors(lampsSize);
    auto lampPlaneIds = MakePlaneIds(lampsSize);
    auto lampDistanceCoeffs = MakeFloats(lampsSize);
    auto lampSpreadMaxDistances = MakeFloats(lampsSize);

    auto outLightBuffer = make_unique_buffer_aligned_to_vectorization_word<float>(pointsSize);

    for (auto _ : state)
    {
        Algorithms::DiffuseLight_Vectorized(
            pointPositions.get(),
            pointPlaneIds.get(),
            ElementIndex(pointsSize),
            lampPositions.get(),
            lampPlaneIds.get(),
            lampDistanceCoeffs.get(),
            lampSpreadMaxDistances.get(),
            ElementIndex(lampsSize),
            outLightBuffer.get());
    }

    benchmark::DoNotOptimize(outLightBuffer);
}
//BENCHMARK(DiffuseLight_Vectorized)->Arg(2)->Arg(10)->Arg(20)->Arg(50)->Arg(100);
BENCHMARK(DiffuseLight_Vectorized)->Arg(2)->Arg(4)->Arg(8);
