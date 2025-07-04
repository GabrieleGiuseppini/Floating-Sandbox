/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-02-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ThreadPool.h"

#include "Log.h"
#include "SysSpecifics.h"

#include <algorithm>

ThreadPool::ThreadPool(
    ThreadManager::ThreadTaskKind threadTaskKind,
    size_t parallelism,
    ThreadManager & threadManager)
    : mThreadTaskKind(threadTaskKind)
    , mLock()
    , mThreads()
    , mWorkerThreadSignal()
    , mTasksToRun(nullptr)
    , mTasksToComplete(0)
    , mCompletedTasks(0)
    , mIsStop(false)
{
    LogMessage("ThreadPool: creating thread pool with parallelism=", parallelism);

    assert(parallelism > 0);

    // Start N-1 threads (main thread is one of them)
    for (size_t i = 0; i < parallelism - 1; ++i)
    {
        std::string threadName = "FS TPool " + std::to_string(i + 1);
        mThreads.emplace_back([this, threadName, i, &threadManager]()
            {
                ThreadLoop(threadName, i + 1, threadManager);
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
    assert(!tasks.empty());
    assert(mTasksToComplete <= 0);

    // Shortcut to avoid paying synchronization penalties
    // in trivial cases
    if (mThreads.empty() || tasks.size() == 1)
    {
        for (Task const & task : tasks)
        {
            RunTask(task);
        }

        return;
    }

    // Queue all the tasks
    {
        std::unique_lock const lock{ mLock };

        mTasksToRun = &tasks;
        mTasksToComplete.store(static_cast<int>(tasks.size()) - 1); // Take already the main thread one into account
        mCompletedTasks.store(1); // Take already the main thread one into account
    }

    // Signal threads that tasks are available
    mWorkerThreadSignal.notify_all();

    // Run the Nth task on the main thread
    RunTask(tasks.back());

    // Run the remaining tasks on main thread, if needed
    RunRemainingTasksLoop();

    // Only returns when there are no more tasks
    assert(mTasksToComplete.load() <= 0);

    // Wait until all tasks are completed
    {
        // ...in a spinlock
        while (true)
        {
            assert(mCompletedTasks.load() >= 0 && mCompletedTasks.load() <= tasks.size());
            if (mCompletedTasks.load() == tasks.size())
            {
                break;
            }
        }
    }
}

void ThreadPool::ThreadLoop(
    std::string threadName,
    size_t threadTaskIndex,
    ThreadManager & threadManager)
{
    //
    // Initialize thread
    //

    threadManager.InitializeThisThread(mThreadTaskKind, threadName, threadTaskIndex);

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

            // Wait for signal that tasks have been queued (or that we've been stopped)
            mWorkerThreadSignal.wait(
                lock,
                [this]()
                {
                    // Condition to leave the wait
                    // Note: other threads may empty mTasksToComplete - that's fine, this thread won't run
                    return mIsStop || mTasksToComplete.load() > 0;
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

        int const oldTasksToComplete = mTasksToComplete.fetch_sub(1);
        if (oldTasksToComplete <= 0)
        {
            // No more tasks
            return;
        }

        //
        // Run the task
        //

        RunTask((*mTasksToRun)[oldTasksToComplete - 1]);

        //
        // Signal task completion
        //

        ++mCompletedTasks;
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