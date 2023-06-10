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

size_t ThreadManager::GetNumberOfProcessors()
{
    return std::max(
        size_t(1),
        static_cast<size_t>(std::thread::hardware_concurrency()));
}

ThreadManager::ThreadManager(
    bool isRenderingMultithreaded,
    size_t maxInitialParallelism)
{
    auto const numberOfProcessors = GetNumberOfProcessors();

    // Calculate max parallelism

    int availableThreads = static_cast<int>(numberOfProcessors);    
    if (isRenderingMultithreaded)
        --availableThreads;

    mMaxSimulationParallelism = std::max(availableThreads, 1);

    size_t const simulationParallelism = std::min(mMaxSimulationParallelism, maxInitialParallelism);

    LogMessage("ThreadManager: isRenderingMultithreaded=", (isRenderingMultithreaded ? "YES" : "NO"),
        " maxSimulationParallelism=", mMaxSimulationParallelism,
        " simulationParallism=", simulationParallelism);

    // Setup current parallelism
    SetSimulationParallelism(simulationParallelism);
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

    LogMessage("ThreadManager: creating simulation thread pool with parallelism=", parallelism);
    mSimulationThreadPool = std::make_unique<ThreadPool>(parallelism, *this);
}

ThreadPool & ThreadManager::GetSimulationThreadPool()
{
    return *mSimulationThreadPool;
}

void ThreadManager::InitializeThisThread()
{
    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif
}
