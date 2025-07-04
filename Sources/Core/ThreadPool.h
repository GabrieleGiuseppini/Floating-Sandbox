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
        return mThreads.size() + 1;
    }

    /*
     * The first task is guaranteed to run on the main thread.
     */
    void Run(std::vector<Task> const & tasks);

    /*
     * The first task is guaranteed to run on the main thread.
     */
    inline void RunAndClear(std::vector<Task> & tasks)
    {
        Run(tasks);
        tasks.clear();
    }

private:

    void ThreadLoop(
        std::string threadName,
        size_t threadTaskIndex,
        ThreadManager & threadManager);

    void RunRemainingTasksLoop();

    void RunTask(Task const & task);

private:

    ThreadManager::ThreadTaskKind const mThreadTaskKind;

    // Our thread lock
    std::mutex mLock;

    // Our threads (N-1, as main thread also plays)
    std::vector<std::thread> mThreads;

    // The condition variable to wake up threads
    std::condition_variable mWorkerThreadSignal;

    // The tasks currently awaiting to be picked up;
    // threads takes all but the last one
    std::vector<Task> const * mTasksToRun;

    // Also serves as proxy to index of next task to pick.
    // Begins with N-1, as last task is for main thread, and can go lower than zero if too many threads are eager to work
    std::atomic<int> mTasksToComplete;

    // Number of tasks that still have to complete. Trails opposite of mTasksToComplete
    std::atomic<size_t> mCompletedTasks;

    // Set to true when have to stop
    bool mIsStop;
};
