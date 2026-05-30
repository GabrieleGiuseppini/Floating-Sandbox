/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ThreadPool.h"

#include "Log.h"
#include "SysSpecifics.h"

#include <algorithm>

ThreadPool::ThreadPool(
    ThreadManager::ThreadTaskKind threadTaskKind,
    size_t parallelism,
    ThreadManager & threadManager)
    : mThreadTaskKind(threadTaskKind)
    , mLock()
    , mThreads()
    , mWorkerThreadSignal()
    , mThreadAssignedTasks()
    , mThreadAssignedCompletedTasks(0)
    , mIsStop(false)
{
    LogMessage("ThreadPool: creating thread pool with parallelism=", parallelism);

    assert(parallelism > 0);

    //
    // Store calling thread
    //

    std::optional<ThreadManager::CpuInfo> callingThreadCpuInfo;
    if (mThreadTaskKind == ThreadManager::ThreadTaskKind::Simulation)
    {
        // Note: here we assume there is only one Simulation thread pool, hence
        // we eagerly steal the first N CPUs

        // Take fastest CPU
        callingThreadCpuInfo = threadManager.GetNthFastestCpu(0);
    }

    mThreads.emplace_back(
        std::nullopt,
        callingThreadCpuInfo);

    //
    // Start N-1 threads (calling thread is one of them)
    //

    mThreadAssignedTasks.resize(parallelism - 1, nullptr);

    for (size_t i = 0; i < parallelism - 1; ++i)
    {
        // Decide cpu ID
        std::optional<ThreadManager::CpuInfo> cpuInfo;
        std::string threadName;
        if (mThreadTaskKind == ThreadManager::ThreadTaskKind::Simulation)
        {
            // Note: here we assume there is only one Simulation thread pool, hence
            // we eagerly steal the first N CPUs

            // Take next fastest CPU
            size_t cpuResourceIndex =
                1 // Calling thread
                + i;
            cpuInfo = threadManager.GetNthFastestCpu(cpuResourceIndex);

            threadName = "FS SimTPool " + std::to_string(i + 1);
        }
        else
        {
            threadName = "FS XTPool " + std::to_string(i + 1);
        }

        mThreads.emplace_back(
            std::thread(
                [this, cpuInfo, i, threadName, &threadManager]()
                {
                    ThreadLoop(
                        cpuInfo.has_value() ? cpuInfo->CpuId : std::optional<size_t>(),
                        i + 1,
                        threadName,
                        threadManager);
                }),
            cpuInfo);
    }
}

ThreadPool::~ThreadPool()
{
    // Tell all threads to stop
    {
        std::unique_lock const lock{ mLock };

        mIsStop = true;
    }

    // Signal threads so they can check stop flag
    mWorkerThreadSignal.notify_all();

    // Wait for all threads to exit
    for (size_t t = 1; t < mThreads.size(); ++t)
    {
        assert(mThreads[t].Thread.has_value());

        mThreads[t].Thread->join();
    }
}

void ThreadPool::Run(std::vector<Task> const & tasks)
{
    assert(!tasks.empty());

    // Shortcut to avoid paying synchronization penalties
    // in trivial cases
    if (mThreads.size() < 2 || tasks.size() == 1)
    {
        for (Task const & task : tasks)
        {
            RunTask(task);
        }

        return;
    }

    //
    // Tasks run:
    //  - Task 0: calling thread
    //  - Task 1: thread 1
    //  - Task 2: thread 2
    //  - Task N: thread N
    //  - Tasks N+1, ...: calling thread
    //

    // Assign tasks to threads
    size_t const tasksAssignedToThreads = std::min(tasks.size() - 1, mThreads.size() - 1);
    {
        std::unique_lock const lock{ mLock };

        size_t t;
        for (t = 0; t < tasksAssignedToThreads; ++t)
        {
            mThreadAssignedTasks[t] = &tasks[t + 1]; // Reserve first task for caller
        }
        for (; t < mThreads.size() - 1; ++t)
        {
            mThreadAssignedTasks[t] = nullptr; // No tasks for extra threads
        }

        mThreadAssignedCompletedTasks.store(0);
    }

    // Signal threads that tasks are available
    mWorkerThreadSignal.notify_all();

    // Run our own task - the calling thread's task
    RunTask(tasks[0]);

    // Run the remaining tasks on calling thread, if needed
    // (we assume there are few here, at most 1)
    for (size_t t = 1 + tasksAssignedToThreads; t < tasks.size(); ++t)
    {
        RunTask(tasks[t]);
    }

    // Wait until all tasks are completed
    {
        // ...in a spinlock
        while (true)
        {
            assert(mThreadAssignedCompletedTasks.load() >= 0 && mThreadAssignedCompletedTasks.load() <= tasksAssignedToThreads);
            if (mThreadAssignedCompletedTasks.load() == tasksAssignedToThreads)
            {
                break;
            }
        }
    }
}

void ThreadPool::ThreadLoop(
    std::optional<size_t> cpuId,
    size_t threadTaskIndex,
    std::string const & threadName,
    ThreadManager & threadManager)
{
    assert(threadTaskIndex > 0);
    // Cannot really assert this, as mThreads might be not populated yet
    //assert(mThreadAssignedTasks.size() == mThreads.size() - 1);

    //
    // Initialize thread
    //

    threadManager.InitializeThisThread(mThreadTaskKind, cpuId, threadTaskIndex, threadName);

    //
    // Run thread loop until thread pool is destroyed
    //

    while (true)
    {
        Task const * task = nullptr;
        {
            std::unique_lock lock{ mLock };

            if (mIsStop)
            {
                // We're done!
                break;
            }

            // Wait for signal that tasks have been queued (or that we've been stopped)
            mWorkerThreadSignal.wait(
                lock,
                [this, threadTaskIndex]()
                {
                    // Condition to leave the wait
                    return mIsStop || mThreadAssignedTasks[threadTaskIndex - 1] != nullptr;
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }

            // Remove semaphore, while we're in a lock
            std::swap(task, mThreadAssignedTasks[threadTaskIndex - 1]);
        }

        // Tasks have been queued...
        assert(task != nullptr);

        //
        // Run the task
        //

        // TODOTEST
        //LogMessage("Running thread-assigned task ", threadTaskIndex - 1);

        RunTask(*task);

        // TODOTEST
        //LogMessage("Completed thread-assigned task ", threadTaskIndex - 1);

        //
        // Signal task completion
        //

        ++mThreadAssignedCompletedTasks;
    }

    LogMessage("Thread exiting");
}

void ThreadPool::RunTask(Task const & task)
{
    try
    {
        task();
    }
    catch (std::exception const & e)
    {
        assert(false); // Catch it in debug mode

        LogMessage("Error running task: " + std::string(e.what()));

        // Keep going...
    }
}