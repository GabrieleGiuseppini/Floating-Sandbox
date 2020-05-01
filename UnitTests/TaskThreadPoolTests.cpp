#include <GameCore/TaskThreadPool.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "gtest/gtest.h"

class TaskThreadPoolTests_OneRuns : public testing::TestWithParam<size_t>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    One_Runs,
    TaskThreadPoolTests_OneRuns,
    ::testing::Values(
        0,
        1,
        2,
        10
    ));

TEST_P(TaskThreadPoolTests_OneRuns, One_Runs)
{
    std::vector<bool> results(GetParam(), false);

    std::vector<TaskThreadPool::Task> tasks;
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
    TaskThreadPool t(1);
    t.Run(tasks);

    ASSERT_TRUE(std::all_of(results.cbegin(), results.cend(), [](bool b) { return b; }));
}

class TaskThreadPoolTests_FourRuns : public testing::TestWithParam<size_t>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_SUITE_P(
    Four_Runs,
    TaskThreadPoolTests_FourRuns,
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

TEST_P(TaskThreadPoolTests_FourRuns, Four_Runs)
{
    std::vector<bool> results(GetParam(), false);

    std::vector<TaskThreadPool::Task> tasks;
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
    TaskThreadPool t(4);
    t.Run(tasks);

    ASSERT_TRUE(std::all_of(results.cbegin(), results.cend(), [](bool b) { return b; }));
}