/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <memory>

class ThreadPool;

class ThreadManager final
{
public:

    static size_t GetNumberOfProcessors();

    static void InitializeThisThread();

public:

    ThreadManager(
        bool isRenderingMultithreaded,
        size_t maxInitialParallelism,
        std::function<void()> && setCurrentThreadAsHighPriorityFunctor);

    size_t GetSimulationParallelism() const;

    void SetSimulationParallelism(size_t parallelism);

    size_t GetMinSimulationParallelism() const
    {
        return 1;
    }

    size_t GetMaxSimulationParallelism() const
    {
        return mMaxSimulationParallelism;
    }

    void SetCurrentThreadAsHighPriority();

    ThreadPool & GetSimulationThreadPool();

private:

    static size_t CalculateMaxSimulationParallelism(bool isRenderingMultithreaded);

private:

    size_t const mMaxSimulationParallelism; // Calculated via init args and hardware concurrency; never changes
    std::function<void()> mSetCurrentThreadAsHighPriorityFunctor;

    std::unique_ptr<ThreadPool> mSimulationThreadPool;
};

#include "ThreadPool.h"