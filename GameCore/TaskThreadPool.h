/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <functional>
#include <mutex>
#include <queue>
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

private:

    // The lock for everything
    std::mutex mLock;

    // Our threads
    std::vector<std::thread> mThreads;

    // The tasks currently awaiting to be picked up;
    // expected to be empty at each Run invocation
    std::queue<Task> mRemainingTasks;
};
