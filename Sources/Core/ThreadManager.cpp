/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ThreadManager.h"

#include "FloatingPoint.h"
#include "Log.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <cassert>

#if FS_IS_OS_WINDOWS()
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#elif FS_IS_OS_ANDROID()
#include <sys/syscall.h>
#include <unistd.h>
#endif

size_t ThreadManager::GetNumberOfProcessors()
{
    return std::max(
        size_t(1),
        static_cast<size_t>(std::thread::hardware_concurrency()));
}

ThreadManager::ThreadManager(
    bool isRenderingMultithreaded,
    size_t simulationParallelism,
    std::vector<CpuInfo> const & cpuResources,
    PlatformSpecificThreadInitializationFunction && platformSpecificThreadInitializationFunctor)
    : mIsRenderingMultithreaded(isRenderingMultithreaded)
    , mMaxSimulationParallelism(GetNumberOfProcessors())
    , mCpuResources(SortCpuResources(cpuResources))
    , mPlatformSpecificThreadInitializationFunctor(std::move(platformSpecificThreadInitializationFunctor))
{
    assert(simulationParallelism <= cpuResources.size());
    assert(cpuResources.size() <= GetNumberOfProcessors());

    LogMessage("ThreadManager: isRenderingMultithreaded=", (mIsRenderingMultithreaded ? "YES" : "NO"),
        " simulationParallelism=", simulationParallelism,
        " cpuResources=[", mCpuResources.size(), "]",
        " maxSimulationParallelism=", mMaxSimulationParallelism);

    // Set parallelism
    SetSimulationParallelism(simulationParallelism);
}

void ThreadManager::InitializeThisThread(
    ThreadTaskKind threadTaskKind,
    std::optional<size_t> cpuId,
    size_t threadTaskIndex,
    std::string const & threadName)
{
    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif

    mPlatformSpecificThreadInitializationFunctor(threadTaskKind, cpuId, threadTaskIndex, threadName);
}

size_t ThreadManager::GetThisThreadProcessor()
{
#if FS_IS_OS_WINDOWS()
    return GetCurrentProcessorNumber();
#elif FS_IS_OS_ANDROID()
    unsigned cpu;
    if (syscall(__NR_getcpu, &cpu, NULL, NULL) < 0)
    {
        return static_cast<size_t>(-1);
    }
    else
    {
        return static_cast<size_t>(cpu);
    }
#else
    return static_cast<size_t>(-1);
#endif
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

    mSimulationThreadPool = std::make_unique<ThreadPool>(ThreadManager::ThreadTaskKind::Simulation, parallelism, *this);
}

ThreadPool const & ThreadManager::GetSimulationThreadPool() const
{
    return *mSimulationThreadPool;
}

ThreadPool & ThreadManager::GetSimulationThreadPool()
{
    return *mSimulationThreadPool;
}

std::vector<ThreadManager::CpuInfo> ThreadManager::SortCpuResources(std::vector<CpuInfo> const & src)
{
    std::vector<ThreadManager::CpuInfo> newCpuResources = src;
    std::sort(
        newCpuResources.begin(),
        newCpuResources.end(),
        [](CpuInfo const & c1, CpuInfo const & c2) -> bool
        {
            return c1.Speed > c2.Speed;
        });

    return newCpuResources;
}