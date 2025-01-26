#include <Core/TemporallyCoherentPriorityQueue.h>

#include "gtest/gtest.h"

TEST(TemporallyCoherentPriorityQueueTest, Empty)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, OneElement)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, TwoElements)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Pop_OneElement)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1u, q.size());

    auto i = q.pop();

    EXPECT_EQ(5u, i);
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Pop_TwoElements)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2u, q.size());

    auto i = q.pop();

    EXPECT_EQ(8u, i);
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1u, q.size());
    EXPECT_TRUE(q.verify_heap());

    i = q.pop();

    EXPECT_EQ(5u, i);
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Clear)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(6, 1.0f);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(2u, q.size());

    q.clear();

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Sorting)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Sorting_CustomComparer)
{
    struct larger_first
    {
        bool operator()(float a, float b)
        {
            return a >= b;
        }
    };

    TemporallyCoherentPriorityQueue<float, larger_first> q(10);

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

TEST(TemporallyCoherentPriorityQueueTest, Update_Mid)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.add_or_update(5, 2.0f);

    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Update_Smallest)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.add_or_update(3, 13.0f);

    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    i = q.pop();
    EXPECT_EQ(3u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Update_Largest)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.add_or_update(2, 2.0f);

    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Update_SamePriority)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.add_or_update(2, 12.0f);

    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Update_NoRealChange)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.add_or_update(2, 11.0f);

    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Remove_Empty)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.remove_if_in(5);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Remove_NonExisting_OneElement)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);

    q.remove_if_in(9);

    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Remove_OneElement)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);

    q.remove_if_in(5);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Remove_Smallest)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.remove_if_in(3);

    EXPECT_EQ(3u, q.size());
    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    i = q.pop();
    EXPECT_EQ(2u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Remove_Largest)
{
    TemporallyCoherentPriorityQueue<float> q(10);

    q.add_or_update(5, 6.0f);
    q.add_or_update(8, 3.0f);
    q.add_or_update(3, 1.0f);
    q.add_or_update(2, 12.0f);

    ASSERT_EQ(4u, q.size());

    q.remove_if_in(2);

    EXPECT_EQ(3u, q.size());
    EXPECT_TRUE(q.verify_heap());

    auto i = q.pop();
    EXPECT_EQ(3u, i);

    i = q.pop();
    EXPECT_EQ(8u, i);

    i = q.pop();
    EXPECT_EQ(5u, i);

    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0u, q.size());
    EXPECT_TRUE(q.verify_heap());
}

TEST(TemporallyCoherentPriorityQueueTest, Populate_Asymmetrically)
{
    TemporallyCoherentPriorityQueue<float> q(100);

    q.add_or_update(1, 1.0f);
    q.add_or_update(2, 6.0f);
    q.add_or_update(3, 7.0f);
    q.add_or_update(4, 8.0f);
    q.add_or_update(5, 9.0f);
    q.add_or_update(6, 10.0f);
    q.add_or_update(7, 11.0f);
    q.add_or_update(8, 12.0f);
    q.add_or_update(9, 13.0f);

    EXPECT_TRUE(q.verify_heap());
}