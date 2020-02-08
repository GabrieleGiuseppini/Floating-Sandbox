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

    void Run(std::vector<Task> const & tasks);

private:

    void ThreadLoop();

    void RunTaskLoop();

private:

    // Our thread lock
    std::mutex mLock;

    // Our threads
    std::vector<std::thread> mThreads;

    // The condition variable to wake up threads
    std::condition_variable mThreadSignal;

    // The tasks currently awaiting to be picked up;
    // expected to be empty at each Run invocation
    std::deque<Task> mRemainingTasks;

    // Set to true when have to stop
    bool mIsStop;
};
