#include <GameCore/TruncatedPriorityQueue.h>

#include "gtest/gtest.h"

#include <algorithm>
#include <vector>

namespace /* anonymous */ {

    template<typename T>
    std::vector<ElementIndex> MakeSortedVector(T const & pq)
    {
        std::vector<ElementIndex> r;
        for (size_t i = 0; i < pq.size(); ++i)
            r.push_back(pq[i]);

        std::sort(r.begin(), r.end());

        return r;
    }

}

TEST(TruncatedPriorityQueueTest, Empty)
{
    TruncatedPriorityQueue<float> q(10);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TruncatedPriorityQueueTest, OneElement)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TruncatedPriorityQueueTest, TwoElements)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);
    q.emplace(8, 3.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TruncatedPriorityQueueTest, Access_OneElement)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1, q.size());

    auto i = q[0];

    EXPECT_EQ(5u, i);
}

TEST(TruncatedPriorityQueueTest, Access_TwoElements)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);
    q.emplace(8, 3.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2u, q.size());

    auto sortedElems = MakeSortedVector(q);

    ASSERT_EQ(2, sortedElems.size());
    EXPECT_EQ(5u, sortedElems[0]);
    EXPECT_EQ(8u, sortedElems[1]);
}

TEST(TruncatedPriorityQueueTest, Clear)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);
    q.emplace(6, 1.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2u, q.size());

    q.clear();

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TruncatedPriorityQueueTest, KeepsTopN_LessThanMax)
{
    TruncatedPriorityQueue<float> q(10);

    q.emplace(5, 6.0f);
    q.emplace(8, 3.0f);
    q.emplace(3, 1.0f);
    q.emplace(2, 12.0f);

    ASSERT_EQ(4, q.size());

    auto sortedElems = MakeSortedVector(q);

    ASSERT_EQ(4, sortedElems.size());
    EXPECT_EQ(2u, sortedElems[0]);
    EXPECT_EQ(3u, sortedElems[1]);
    EXPECT_EQ(5u, sortedElems[2]);
    EXPECT_EQ(8u, sortedElems[3]);
}

TEST(TruncatedPriorityQueueTest, KeepsTopN_MoreThanMax)
{
    TruncatedPriorityQueue<float> q(4);

    q.emplace(5, 6.0f);
    q.emplace(8, 3.0f);
    q.emplace(3, 1.0f);
    q.emplace(2, 12.0f);
    q.emplace(12, 4.0f);

    ASSERT_EQ(4, q.size());

    auto sortedElems = MakeSortedVector(q);

    ASSERT_EQ(4, sortedElems.size());
    EXPECT_EQ(3u, sortedElems[0]);
    EXPECT_EQ(5u, sortedElems[1]);
    EXPECT_EQ(8u, sortedElems[2]);
    EXPECT_EQ(12u, sortedElems[3]);
}

/*
TEST(TruncatedPriorityQueueTest, Sorting_CustomComparer)
{
    struct larger_first
    {
        bool operator()(float a, float b)
        {
            return a >= b;
        }
    };

    TruncatedPriorityQueue<float, larger_first> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    auto i = q.pop();
    EXPECT_EQ(2u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(3u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}
*/

TEST(TruncatedPriorityQueueTest, Populate_Asymmetrically)
{
    TruncatedPriorityQueue<float> q(100);

    q.emplace(1, 1.0f);
    q.emplace(2, 6.0f);
    q.emplace(3, 7.0f);
    q.emplace(4, 8.0f);
    q.emplace(5, 9.0f);
    q.emplace(6, 10.0f);
    q.emplace(7, 11.0f);
    q.emplace(8, 12.0f);
    q.emplace(9, 13.0f);

    EXPECT_TRUE(q.verify_heap());
}

// TODO:
// - Clear with new max size
//      - smaller than before
//      - larger than before