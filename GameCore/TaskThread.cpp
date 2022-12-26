/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TaskThread.h"

#include "Log.h"
#include "SystemThreadManager.h"

#include <cassert>

TaskThread::TaskThread()
    : TaskThread(false)
{}

TaskThread::TaskThread(bool doForceNoMultiThreading)
    : mIsStop(false)
{
    // Only use a real thread on multi-core boxes; on single-core
    // boxes, we'll just emulate multi-threading by running all
    // tasks directly - and synchronously - on the caller's thread
    mHasThread = SystemThreadManager::GetInstance().GetNumberOfProcessors() > 1;

    if (doForceNoMultiThreading)
        mHasThread = false;

    if (mHasThread)
    {
        LogMessage("TaskThread::TaskThread(): starting thread...");

        // Start thread
        mThread = std::thread(&TaskThread::ThreadLoop, this);
    }
    else
    {
        LogMessage("TaskThread::TaskThread(): not starting thread - will be simulating multi-threading");
    }
}

TaskThread::~TaskThread()
{
    if (mHasThread)
    {
        assert(mThread.joinable());

        // Notify stop
        {
            std::unique_lock const lock{ mThreadLock };

            mIsStop = true;
            mThreadSignal.notify_one();
        }

        LogMessage("TaskThread::~TaskThread(): signaled stop; waiting for thread now...");

        // Wait for thread
        mThread.join();

        LogMessage("TaskThread::~TaskThread(): ...thread stopped.");
    }
}

void TaskThread::ThreadLoop()
{
    assert(mHasThread); // This method only runs if we're truly multi-threaded

    //
    // Initialize thread
    //

    SystemThreadManager::GetInstance().InitializeThisThread();

    //
    // Run loop
    //

    while (true)
    {
        //
        // Extract task
        //

        QueuedTask queuedTask;

        {
            std::unique_lock<std::mutex> lock(mThreadLock);

            // Wait for signal
            mThreadSignal.wait(
                lock,
                [this]
                {
                    return mIsStop || !mTaskQueue.empty();
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }
            else
            {
                //
                // Deque task
                //

                assert(!mTaskQueue.empty());

                queuedTask = std::move(mTaskQueue.front());
                mTaskQueue.pop_front();
            }
        }

        //
        // Run task
        //

        assert(!!queuedTask.TaskToRun);

        try
        {
            queuedTask.TaskToRun();
        }
        catch (std::runtime_error const & exc)
        {
            // Store in task completion indicator
            // (safe to do without locks, as it will only be read by
            // main thread after it is signaled)
            queuedTask.CompletionIndicator->RegisterException(exc.what());
        }

        //
        // Signal task completion
        //

        {
            std::unique_lock const lock{ mThreadLock };

            queuedTask.CompletionIndicator->MarkCompleted();
            mThreadSignal.notify_one();
        }
    }

    LogMessage("TaskThread::ThreadLoop(): exiting");
}