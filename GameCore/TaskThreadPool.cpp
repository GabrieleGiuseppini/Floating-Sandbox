/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TaskThreadPool.h"

#include "Log.h"
#include "SystemThreadManager.h"

#include <algorithm>

TaskThreadPool::TaskThreadPool()
    : TaskThreadPool(SystemThreadManager::GetInstance().GetNumberOfProcessors())
{
}

TaskThreadPool::TaskThreadPool(size_t numberOfProcessors)
    : mLock()
    , mThreads()
    , mWorkerThreadSignal()
    , mMainThreadSignal()
    , mRemainingTasks()
    , mTasksToComplete(0)
    , mIsStop(false)
{
    assert(numberOfProcessors > 0);

    // Cap threads to 2 as we don't need more than those
    size_t const threadCount = std::min(size_t(2), numberOfProcessors - 1);

    LogMessage("TaskThreadPool: number of processors: ", numberOfProcessors,
        " number of threads in pool: ", threadCount);

    // Start threads
    for (size_t i = 0; i < threadCount; ++i)
    {
        mThreads.emplace_back(&TaskThreadPool::ThreadLoop, this);
    }
}

TaskThreadPool::~TaskThreadPool()
{
    // Tell all threads to stop
    {
        std::unique_lock const lock{ mLock };

        mIsStop = true;
    }

    // Signal threads
    mWorkerThreadSignal.notify_all();

    // Wait for all threads to exit
    for (auto & t : mThreads)
    {
        t.join();
    }
}

void TaskThreadPool::Run(std::vector<Task> const & tasks)
{
    assert(mRemainingTasks.empty());
    assert(0 == mTasksToComplete);

    // Queue all the tasks except the first one,
    // which we're gonna run immediately now to guarantee
    // that the first task always runs on the main thread
    {
        std::unique_lock const lock{ mLock };

        for (size_t t = 1; t < tasks.size(); ++t)
        {
            mRemainingTasks.push_back(tasks[t]);
        }

        mTasksToComplete = mRemainingTasks.size();
    }

    // Signal threads
    mWorkerThreadSignal.notify_all();

    // Run the first task on the main thread
    if (!tasks.empty())
    {
        RunTask(tasks.front());
    }

    // Run the remaining tasks on own thread, if needed
    RunRemainingTasksLoop();

    // Only returns when there are no more tasks
    assert(mRemainingTasks.empty());

    // Wait until all tasks are completed
    {
        std::unique_lock lock{ mLock };

        if (0 != mTasksToComplete)
        {
            // Wait for signal
            mMainThreadSignal.wait(
                lock,
                [this]
                {
                    return 0 == mTasksToComplete;
                });

            assert(0 == mTasksToComplete);
        }
    }
}

void TaskThreadPool::ThreadLoop()
{
    //
    // Initialize thread
    //

    SystemThreadManager::GetInstance().InitializeThisThread();

    //
    // Run thread loop until thread pool is destroyed
    //

    while (true)
    {
        {
            std::unique_lock lock{ mLock };

            if (mIsStop)
            {
                // We're done!
                break;
            }

            // Wait for signal
            mWorkerThreadSignal.wait(
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

        // ...run the remaining tasks
        RunRemainingTasksLoop();
    }

    LogMessage("Thread exiting");
}

void TaskThreadPool::RunRemainingTasksLoop()
{
    //
    // Run tasks until queue is empty
    //

    while (true)
    {
        //
        // De-queue a task
        //

        Task task;
        {
            std::unique_lock const lock{ mLock };

            if (!mRemainingTasks.empty())
            {
                task = std::move(mRemainingTasks.front());
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

        RunTask(task);

        //
        // Signal task completion
        //

        {
            std::unique_lock const lock{ mLock };

            assert(mTasksToComplete > 0);

            --mTasksToComplete;
            if (0 == mTasksToComplete)
            {
                // All tasks completed...

                // ...signal main thread
                mMainThreadSignal.notify_all();
            }
        }
    }
}

void TaskThreadPool::RunTask(Task const & task)
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