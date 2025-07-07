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

    enum class ThreadTaskKind
    {
        MainAndSimulation,
        Simulation,
        Render,
        Audio,
        Other
    };

    static size_t GetNumberOfProcessors();

public:

    using PlatformSpecificThreadInitializationFunction = std::function<void(ThreadTaskKind, std::string const &, size_t)>;

    ThreadManager(
        bool isRenderingMultithreaded,
        size_t simulationParallelism,
        PlatformSpecificThreadInitializationFunction && platformSpecificThreadInitializationFunctor);

    bool IsRenderingMultiThreaded() const
    {
        return mIsRenderingMultithreaded;
    }

    void InitializeThisThread(
        ThreadTaskKind threadTaskKind,
        std::string const & threadName,
        size_t threadTaskIndex);

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

    bool const mIsRenderingMultithreaded;
    size_t const mMaxSimulationParallelism; // Calculated via hardware concurrency; never changes
    PlatformSpecificThreadInitializationFunction const mPlatformSpecificThreadInitializationFunctor;

    std::unique_ptr<ThreadPool> mSimulationThreadPool;
};

#include "ThreadPool.h"