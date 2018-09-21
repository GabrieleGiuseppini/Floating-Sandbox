#include "Utils.h"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>

static constexpr size_t Size = 10000000;

static void VectorNormalization_Naive_NoSize(benchmark::State& state) 
{
    auto vectors = MakeVectors(Size);
    float result = 0.0f;
    for (auto _ : state)
    {
        for (size_t i = 0; i < Size; ++i)
        {
            vec2f const normalizedVector = vectors[i].normalise();

            result += normalizedVector.x;
        }
    }

    benchmark::DoNotOptimize(result);
}
BENCHMARK(VectorNormalization_Naive_NoSize);
