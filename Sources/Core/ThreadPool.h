/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ThreadManager.h"

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <deque>
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

    void ThreadLoop(ThreadManager & threadManager);

    void RunRemainingTasksLoop();

    void RunTask(Task const & task);

private:

    // Our thread lock
    std::mutex mLock;

    // Our threads
    std::vector<std::thread> mThreads;

    // The condition variable to wake up threads
    std::condition_variable mWorkerThreadSignal;

    // The condition variable to wake up the main thread
    std::condition_variable mMainThreadSignal;

    // The tasks currently awaiting to be picked up;
    // expected to be empty at each Run invocation
    std::deque<Task const *> mRemainingTasks;

    // The number of tasks awaiting for completion
    size_t mTasksToComplete;

    // Set to true when have to stop
    bool mIsStop;
};
