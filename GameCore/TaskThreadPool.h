/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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
class TaskThreadPool
{
public:

    using Task = std::function<void()>;

public:

    TaskThreadPool();

    TaskThreadPool(size_t hardwareThreads);

    ~TaskThreadPool();

    /*
     * The first task is guaranteed to run on the main thread.
     */
    void Run(std::vector<Task> const & tasks);

private:

    void ThreadLoop();

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
    std::deque<Task> mRemainingTasks;

    // The number of tasks awaiting for completion
    size_t mTasksToComplete;

    // Set to true when have to stop
    bool mIsStop;
};
