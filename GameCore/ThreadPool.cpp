/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ThreadPool.h"

#include "Log.h"
#include "SysSpecifics.h"

#include <algorithm>

#if FS_IS_OS_WINDOWS()
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

ThreadPool::ThreadPool(
    size_t parallelism,
    ThreadManager & threadManager)
    : mLock()
    , mThreads()
    , mWorkerThreadSignal()
    , mMainThreadSignal()
    , mRemainingTasks()
    , mTasksToComplete(0)
    , mIsStop(false)
{
    assert(parallelism > 0);

    // Start N-1 threads (main thread is one of them)
    for (size_t i = 0; i < parallelism - 1; ++i)
    {
        mThreads.emplace_back([this, &threadManager]()
            {
                ThreadLoop(threadManager);
            });
    }
}

ThreadPool::~ThreadPool()
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

void ThreadPool::Run(std::vector<Task> const & tasks)
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
            mRemainingTasks.push_back(&(tasks[t]));
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

void ThreadPool::ThreadLoop(ThreadManager & threadManager)
{
    //
    // Initialize thread
    //

    threadManager.InitializeThisThread();

#if FS_IS_OS_WINDOWS()
    LogMessage("Thread processor: ", GetCurrentProcessorNumber());
#endif

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

void ThreadPool::RunRemainingTasksLoop()
{
    //
    // Run tasks until queue is empty
    //

    while (true)
    {
        //
        // De-queue a task
        //

        Task const * task = nullptr;
        {
            std::unique_lock const lock{ mLock };

            if (!mRemainingTasks.empty())
            {
                task = mRemainingTasks.front();
                mRemainingTasks.pop_front();
            }
        }

        if (task == nullptr)
        {
            // No more tasks
            return;
        }

        //
        // Run the task
        //

        RunTask(*task);

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