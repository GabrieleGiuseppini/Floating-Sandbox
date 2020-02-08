/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TaskThreadPool.h"

#include "Log.h"

#include <algorithm>

TaskThreadPool::TaskThreadPool()
    : TaskThreadPool(std::max(size_t(1), static_cast<size_t>(std::thread::hardware_concurrency())))
{
}

TaskThreadPool::TaskThreadPool(size_t hardwareThreads)
    : mLock()
    , mThreads()
    , mRemainingTasks()
{
    LogMessage("Number of hardware threads: ", hardwareThreads);

    //
    // Start threads
    //

    for (size_t i = 0; i < hardwareThreads - 1; ++i)
    {
        mThreads.emplace_back(&TaskThreadPool::ThreadLoop, this);
    }
}

TaskThreadPool::~TaskThreadPool()
{
    //
    // Stop all threads
    //

    // TODOHERE

    //
    // Wait for threads to exit
    //

    for (auto & t : mThreads)
    {
        t.join();
    }
}

void TaskThreadPool::Run(std::vector<Task> const & tasks)
{
    //
    // Queue the tasks
    //

    {
        std::lock_guard<std::mutex> const lock(mLock);

        for (auto const & task : tasks)
        {
            mRemainingTasks.push(task);
        }
    }

    //
    // Signal threads
    //

    // TODOHERE
}

void TaskThreadPool::ThreadLoop()
{
    // TODOHERE
}