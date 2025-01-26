#include <Core/ThreadPool.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "gtest/gtest.h"

class ThreadPoolTests_OneRuns : public testing::TestWithParam<size_t>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}

protected:

    ThreadManager mThreadManager{ false, 16 };
};

INSTANTIATE_TEST_SUITE_P(
    ThreadPoolTests_OneRuns,
    ThreadPoolTests_OneRuns,
    ::testing::Values(
        0,
        1,
        2,
        10
    ));

TEST_P(ThreadPoolTests_OneRuns, One_Runs)
{
    std::vector<bool> results(GetParam(), false);

    std::vector<ThreadPool::Task> tasks;
    for (size_t t = 0; t < GetParam(); ++t)
    {
        tasks.emplace_back(
            [&results, idx=t]()
            {
                results[idx] = true;
            });
    }

    ASSERT_TRUE(std::none_of(results.cbegin(), results.cend(), [](bool b) { return b; }));

    // Run
    ThreadPool t(1, mThreadManager);
    t.Run(tasks);

    ASSERT_TRUE(std::all_of(results.cbegin(), results.cend(), [](bool b) { return b; }));
}

class ThreadPoolTests_FourRuns : public testing::TestWithParam<size_t>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}

protected:

    ThreadManager mThreadManager{ false, 16 };
};

INSTANTIATE_TEST_SUITE_P(
    ThreadPoolTests_FourRuns,
    ThreadPoolTests_FourRuns,
    ::testing::Values(
        0,
        1,
        2,
        3,
        4,
        5,
        7,
        8,
        9,
        10
    ));

TEST_P(ThreadPoolTests_FourRuns, Four_Runs)
{
    std::vector<bool> results(GetParam(), false);

    std::vector<ThreadPool::Task> tasks;
    for (size_t t = 0; t < GetParam(); ++t)
    {
        tasks.emplace_back(
            [&results, idx = t]()
            {
                results[idx] = true;
            });
    }

    ASSERT_TRUE(std::none_of(results.cbegin(), results.cend(), [](bool b) { return b; }));

    // Run
    ThreadPool t(4, mThreadManager);
    t.Run(tasks);

    ASSERT_TRUE(std::all_of(results.cbegin(), results.cend(), [](bool b) { return b; }));
}