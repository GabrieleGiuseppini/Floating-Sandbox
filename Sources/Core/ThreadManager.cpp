/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ThreadManager.h"

#include "FloatingPoint.h"
#include "Log.h"
#include "SysSpecifics.h"

#include <cassert>

#if FS_IS_OS_WINDOWS()
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

size_t ThreadManager::GetNumberOfProcessors()
{
    return std::max(
        size_t(1),
        static_cast<size_t>(std::thread::hardware_concurrency()));
}

ThreadManager::ThreadManager(
    bool isRenderingMultithreaded,
    size_t maxInitialParallelism,
    std::function<void()> && setCurrentThreadAsHighPriorityFunctor)
    : mMaxSimulationParallelism(CalculateMaxSimulationParallelism(isRenderingMultithreaded))
    , mSetCurrentThreadAsHighPriorityFunctor(std::move(setCurrentThreadAsHighPriorityFunctor))
{
    size_t const simulationParallelism = std::min(mMaxSimulationParallelism, maxInitialParallelism);

    LogMessage("ThreadManager: isRenderingMultithreaded=", (isRenderingMultithreaded ? "YES" : "NO"),
        " maxSimulationParallelism=", mMaxSimulationParallelism,
        " simulationParallism=", simulationParallelism);

    // Set parallelism
    SetSimulationParallelism(simulationParallelism);
}

void ThreadManager::SetCurrentThreadAsHighPriority()
{
    mSetCurrentThreadAsHighPriorityFunctor();
}

size_t ThreadManager::GetSimulationParallelism() const
{
    return mSimulationThreadPool->GetParallelism();
}

void ThreadManager::SetSimulationParallelism(size_t parallelism)
{
    assert(parallelism >= 1 && parallelism <= GetMaxSimulationParallelism());

    //
    // (Re-)create thread pool
    //

    mSimulationThreadPool.reset();

    mSimulationThreadPool = std::make_unique<ThreadPool>(parallelism, *this);
}

ThreadPool & ThreadManager::GetSimulationThreadPool()
{
    return *mSimulationThreadPool;
}

void ThreadManager::InitializeThisThread()
{
#if FS_IS_OS_WINDOWS()
    LogMessage("Thread processor: ", GetCurrentProcessorNumber());
#endif

    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif
}

size_t ThreadManager::CalculateMaxSimulationParallelism(bool isRenderingMultithreaded)
{
    auto const numberOfProcessors = GetNumberOfProcessors();

    // Calculate max simulation parallelism

    int availableThreads = static_cast<int>(numberOfProcessors);
    if (isRenderingMultithreaded)
        --availableThreads;

    return std::max(availableThreads, 1); // Includes this thread
}