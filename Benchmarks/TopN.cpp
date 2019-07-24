#include "Utils.h"

#include <GameCore/TemporallyCoherentPriorityQueue.h>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

static constexpr size_t Size = 100000;

using Element = std::tuple<int64_t, float>;

static void TopN_Vector_EmplaceAndSort(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    size_t v = 0;

    std::vector<Element> results;

    for (auto _ : state)
    {
        results.clear();

        for (int64_t i = 0; i < state.range(0); ++i, ++v)
        {
            results.emplace_back(i, vals[v % Size]);
        }

        std::sort(
            results.begin(),
            results.end(),
            [](auto const & t1, auto const & t2)
            {
                return std::get<1>(t1) > std::get<1>(t2);
            });

        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(TopN_Vector_EmplaceAndSort)->Arg(20)->Arg(100)->Arg(500)->Arg(1000);



struct TupleComparer
{
    bool operator()(Element const & t1, Element const & t2)
    {
        return std::get<1>(t1) > std::get<1>(t2);
    }
};

class PriQueue : public std::priority_queue<Element, std::vector<Element>, TupleComparer>
{
public:

    void clear()
    {
        this->c.clear();
    }
};

static void TopN_PriorityQueue_Emplace(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    size_t v = 0;

    PriQueue results;

    for (auto _ : state)
    {
        results.clear();

        for (int64_t i = 0; i < state.range(0); ++i, ++v)
        {
            results.emplace(i, vals[v % Size]);
        }

        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(TopN_PriorityQueue_Emplace)->Arg(20)->Arg(100)->Arg(500)->Arg(1000);


static void TopN_PriorityQueue_EmplaceAndPop(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    size_t v = 0;

    PriQueue results;

    for (auto _ : state)
    {
        results.clear();

        for (int64_t i = 0; i < state.range(0); ++i, ++v)
        {
            results.emplace(i, vals[v % Size]);

            if (results.size() > 10)
                results.pop();
        }

        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(TopN_PriorityQueue_EmplaceAndPop)->Arg(20)->Arg(100)->Arg(500)->Arg(1000);


static void TopN_Vector_EmplaceAndNthElement(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    size_t v = 0;

    std::vector<Element> results;

    TupleComparer tc;

    for (auto _ : state)
    {
        results.clear();

        for (int64_t i = 0; i < state.range(0); ++i, ++v)
        {
            results.emplace_back(i, vals[v % Size]);
        }

        std::nth_element(
            results.begin(),
            results.begin() + 10,
            results.end(),
            tc);

        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(TopN_Vector_EmplaceAndNthElement)->Arg(20)->Arg(100)->Arg(500)->Arg(1000);


static void TopN_TemporallyCoherentPriorityQueue_Add(benchmark::State& state)
{
    auto vals = MakeFloats(Size);
    size_t v = 0;

    TemporallyCoherentPriorityQueue<float> results(state.range(0));

    for (auto _ : state)
    {
        results.clear();

        for (int64_t i = 0; i < state.range(0); ++i, ++v)
        {
            results.add_or_update(static_cast<ElementIndex>(i), vals[v % Size]);
        }

        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(TopN_TemporallyCoherentPriorityQueue_Add)->Arg(20)->Arg(100)->Arg(500)->Arg(1000);