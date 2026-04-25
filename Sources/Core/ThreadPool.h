/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ThreadManager.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <deque>
#include <optional>
#include <string>
#include <thread>
#include <vector>

/*
 * This class implements a thread pool that executes batches of tasks.
 *
 */
class ThreadPool final
{
public:

    using Task = std::function<void()>;

public:

    explicit ThreadPool(
        ThreadManager::ThreadTaskKind threadTaskKind,
        size_t parallelism,
        ThreadManager & threadManager);

    ~ThreadPool();

    size_t GetParallelism() const
    {
        return mThreads.size();
    }

    std::optional<ThreadManager::CpuInfo> const & GetThreadCpuInfo(size_t threadIndex) // Calling thread is 0
    {
        assert(threadIndex < mThreads.size());
        return mThreads[threadIndex].CpuInfo;
    }

    /*
     * The first task is guaranteed to run on the calling thread.
     */
    void Run(std::vector<Task> const & tasks);

    /*
     * The first task is guaranteed to run on the calling thread.
     */
    inline void RunAndClear(std::vector<Task> & tasks)
    {
        Run(tasks);
        tasks.clear();
    }

private:

    void ThreadLoop(
        std::optional<size_t> cpuId,
        size_t threadTaskIndex,
        std::string const & threadName,
        ThreadManager & threadManager);

    void RunTask(Task const & task);

private:

    struct ThreadData
    {
        std::optional<std::thread> Thread; // Null for calling thread
        std::optional<ThreadManager::CpuInfo> CpuInfo; // Only when assigned

        ThreadData(
            std::optional<std::thread> && thread,
            std::optional<ThreadManager::CpuInfo> const & cpuInfo)
            : Thread(std::move(thread))
            , CpuInfo(cpuInfo)
        { }
    };
private:

    ThreadManager::ThreadTaskKind const mThreadTaskKind;

    // Our thread lock
    std::mutex mLock;

    // Our threads, including calling thread
    std::vector<ThreadData> mThreads;

    // The condition variable to wake up threads
    std::condition_variable mWorkerThreadSignal;

    // The tasks assigned to threads
    std::vector<Task const *> mThreadAssignedTasks;

    // Counts how many task-assigned tasks have yet to complete;
    // serves as a semaphore
    // TODO: see if still needed
    std::atomic<int> mThreadAssignedTasksToComplete;

    // Number of task-assigned tasks that have completed. Trails opposite of mTasksToComplete
    std::atomic<size_t> mThreadAssignedCompletedTasks;

    // Set to true when have to stop
    bool mIsStop;
};
