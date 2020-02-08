#include <GameCore/TaskThreadPool.h>

#include <algorithm>
#include <functional>
#include <vector>

#include "gtest/gtest.h"

class TaskCountTest : public testing::TestWithParam<size_t>
{
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

INSTANTIATE_TEST_CASE_P(
    TaskThreadPoolTests,
    TaskCountTest,
    ::testing::Values(
        0,
        1,
        2,
        10
    ));

TEST_P(TaskCountTest, One_Runs)
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

INSTANTIATE_TEST_CASE_P(
    Four_Runs,
    TaskCountTest,
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

TEST_P(TaskCountTest, Four_Runs)
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