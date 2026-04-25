/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ThreadPool;

class ThreadManager final
{
public:

    enum class ThreadTaskKind
    {
        MainAndSimulation, // Takes fastest CPU, and is task 0 in Simulation thread pool
        Simulation, // Takes remaining fastest CPUs, up to SimulationParallelism; are tasks 1...N in Simulation thread pool
        Render,
        Other
    };

    struct CpuInfo
    {
        size_t CpuId; // Platform-specific
        float Speed; // Unit doesn't matter; it's relative weights that do

        CpuInfo(
            size_t cpuId,
            float speed)
            : CpuId(cpuId)
            , Speed(speed)
        { }
    };

    static size_t GetNumberOfProcessors();

public:

    using PlatformSpecificThreadInitializationFunction = std::function<void(
        ThreadTaskKind,
        std::optional<size_t>, // CPU ID, if we want this thread to be affinitized to a specific CPU
        size_t, // ThreadTaskIndex
        std::string const &)>; // Thread name

    ThreadManager(
        bool isRenderingMultithreaded,
        size_t simulationParallelism,
        std::vector<CpuInfo> const & cpuResources,
        PlatformSpecificThreadInitializationFunction && platformSpecificThreadInitializationFunctor);

    bool IsRenderingMultiThreaded() const
    {
        return mIsRenderingMultithreaded;
    }

    CpuInfo const & GetNthFastestCpu(size_t n) // 0 is fastest
    {
        assert(n < mCpuResources.size());
        return mCpuResources[n];
    }

    void InitializeThisThread(
        ThreadTaskKind threadTaskKind,
        std::optional<size_t> cpuId,
        size_t threadTaskIndex,
        std::string const & threadName);

    static size_t GetThisThreadProcessor();

    //
    // Simulation Parallelism applies to all simulation tasks, including SpringRelaxation and LightDiffusion
    //

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

    ThreadPool & GetSimulationThreadPool();

private:

    static std::vector<CpuInfo> SortCpuResources(std::vector<CpuInfo> const & src);

private:

    bool const mIsRenderingMultithreaded;
    size_t const mMaxSimulationParallelism; // Calculated via hardware concurrency; never changes
    std::vector<CpuInfo> const mCpuResources; // Sorted by speef
    PlatformSpecificThreadInitializationFunction const mPlatformSpecificThreadInitializationFunctor;

    std::unique_ptr<ThreadPool> mSimulationThreadPool;
};

#include "ThreadPool.h"