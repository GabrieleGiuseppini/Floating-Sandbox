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
    , mThreadSignal()
    , mRemainingTasks()
    , mIsStop(false)
{
    LogMessage("Number of hardware threads: ", hardwareThreads);

    // Start threads
    for (size_t i = 0; i < hardwareThreads - 1; ++i)
    {
        mThreads.emplace_back(&TaskThreadPool::ThreadLoop, this);
    }
}

TaskThreadPool::~TaskThreadPool()
{
    // Tell all threads to stop
    mIsStop = true;
    mThreadSignal.notify_all();

    // Wait for all threads to exit
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
        std::lock_guard const lock{ mLock };

        for (auto const & task : tasks)
        {
            mRemainingTasks.push_back(task);
        }
    }

    //
    // Signal threads
    //

    mThreadSignal.notify_all();

    //
    // Run the task loop for our own thread
    //

    RunTaskLoop();

    assert(mRemainingTasks.empty());

    //
    // Make sure all threads are done
    //

    // TODOHERE
}

void TaskThreadPool::ThreadLoop()
{
    while (true)
    {
        {
            std::unique_lock lock{ mLock };

            // Wait for a signal
            mThreadSignal.wait(
                lock,
                [this]
                {
                    return mIsStop || !mRemainingTasks.empty();
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }
        }

        // Tasks have been queued...

        // ...run the tasks
        RunTaskLoop();
    }

    LogMessage("Thread exiting");
}

void TaskThreadPool::RunTaskLoop()
{
    //
    // Run tasks until queue is empty
    //

    while (true)
    {
        //
        // Pick a task
        //

        Task task;
        {
            std::lock_guard const lock{ mLock };

            if (!mRemainingTasks.empty())
            {
                task = mRemainingTasks.front();
                mRemainingTasks.pop_front();
            }
        }

        if (!task)
        {
            // No more tasks
            return;
        }

        //
        // Run the task
        //

        try
        {
            task();
        }
        catch (std::exception const & e)
        {
            LogMessage("Error running task: " + std::string(e.what()));

            // Keep going...
        }

        //
        // Signal task completion
        //

        // TODOHERE
    }
}