/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class ThreadPool;

class ThreadManager final
{
public:

    static size_t GetNumberOfProcessors();

public:

    ThreadManager(
        bool isRenderingMultithreaded,
        size_t maxInitialParallelism,
        std::function<void(std::string const &, bool)> && platformSpecificThreadInitializationFunctor);

    bool IsRenderingMultiThreaded() const
    {
        return mIsRenderingMultithreaded;
    }

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

    void InitializeThisThread(
        std::string const & threadName,
        bool isHighPriority);

    ThreadPool & GetSimulationThreadPool();

private:

    static size_t CalculateMaxSimulationParallelism(bool isRenderingMultithreaded);

private:

    bool const mIsRenderingMultithreaded;

    size_t const mMaxSimulationParallelism; // Calculated via init args and hardware concurrency; never changes
    std::function<void(std::string const &, bool)> mPlatformSpecificThreadInitializationFunctor;

    std::unique_ptr<ThreadPool> mSimulationThreadPool;
};

#include "ThreadPool.h"