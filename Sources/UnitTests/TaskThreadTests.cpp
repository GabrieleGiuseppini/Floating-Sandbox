#include <Core/TaskThread.h>

#include <thread>

#include "gtest/gtest.h"

TEST(TaskThreadTests, Synchronous)
{
    ThreadManager threadManager{ false, 16, [](ThreadManager::ThreadTaskKind, std::string const &, size_t) {} };
    TaskThread t(ThreadManager::ThreadTaskKind::MainAndSimulation, "Test thread", 0, true, threadManager);

    bool isDone = false;
    auto tc = t.QueueTask(
        [&isDone]()
        {
            isDone = true;
        });

    tc->Wait();

    EXPECT_TRUE(isDone);
}

TEST(TaskThreadTests, Asynchronous)
{
    ThreadManager threadManager{ false, 16, [](ThreadManager::ThreadTaskKind, std::string const &, size_t) {} };
    TaskThread t(ThreadManager::ThreadTaskKind::MainAndSimulation, "Test thread", 0, true, threadManager);

    bool isDone = false;
    t.QueueTask(
        [&isDone]()
        {
            isDone = true;
        });

    for (int i = 0; !isDone && i < 100; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_TRUE(isDone);
}

TEST(TaskThreadTests, RunSynchronously)
{
    ThreadManager threadManager{ false, 16, [](ThreadManager::ThreadTaskKind, std::string const &, size_t) {} };
    TaskThread t(ThreadManager::ThreadTaskKind::MainAndSimulation, "Test thread", 0, true, threadManager);

    bool isDone = false;
    t.RunSynchronously(
        [&isDone]()
        {
            isDone = true;
        });

    EXPECT_TRUE(isDone);
}

TEST(TaskThreadTests, QueueSynchronizationPoint)
{
    ThreadManager threadManager{ false, 16, [](ThreadManager::ThreadTaskKind, std::string const &, size_t) {} };
    TaskThread t(ThreadManager::ThreadTaskKind::MainAndSimulation, "Test thread", 0, true, threadManager);

    bool isDone = false;
    t.RunSynchronously(
        [&isDone]()
        {
            isDone = true;
        });

    auto tc = t.QueueSynchronizationPoint();
    tc->Wait();

    EXPECT_TRUE(isDone);
}