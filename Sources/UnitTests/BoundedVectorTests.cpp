#include <Core/BoundedVector.h>

#include <Core/GameTypes.h>

#include "gtest/gtest.h"

TEST(BoundedVectorTests, DefaultCctorMakesZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, CctorWithMaxSize)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ClearOnZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.clear();

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ClearOnEmpty)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.clear();

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, EmplaceBack)
{
    struct Elem
    {
        int val1;
        float val2;

        Elem(
            int _val1,
            float _val2)
            : val1(_val1)
            , val2(_val2)
        {}
    };

    BoundedVector<Elem> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    Elem & foo1 = vec.emplace_back(4, 0.4f);

    EXPECT_EQ(1u, vec.size());
    EXPECT_EQ(4, vec.back().val1);
    EXPECT_EQ(0.4f, vec.back().val2);
    EXPECT_EQ(4, foo1.val1);
    EXPECT_EQ(0.4f, foo1.val2);
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    Elem & foo2 = vec.emplace_back(2, 0.2f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2, vec.back().val1);
    EXPECT_EQ(0.2f, vec.back().val2);
    EXPECT_EQ(2, foo2.val1);
    EXPECT_EQ(0.2f, foo2.val2);
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
}

TEST(BoundedVectorTests, EmplaceAt)
{
    struct Elem
    {
        int val1;
        float val2;

        Elem(
            int _val1,
            float _val2)
            : val1(_val1)
            , val2(_val2)
        {}
    };

    BoundedVector<Elem> vec(2);

    vec.reset_full(2);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    Elem & foo1 = vec.emplace_at(1, 4, 0.4f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(4, vec[1].val1);
    EXPECT_EQ(0.4f, vec[1].val2);
    EXPECT_EQ(4, foo1.val1);
    EXPECT_EQ(0.4f, foo1.val2);

    Elem & foo2 = vec.emplace_at(0, 6, 0.6f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(6, vec[0].val1);
    EXPECT_EQ(0.6f, vec[0].val2);
    EXPECT_EQ(4, vec[1].val1);
    EXPECT_EQ(0.4f, vec[1].val2);
    EXPECT_EQ(6, foo2.val1);
    EXPECT_EQ(0.6f, foo2.val2);

    Elem & foo3 = vec.emplace_at(1, 8, 0.8f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(6, vec[0].val1);
    EXPECT_EQ(0.6f, vec[0].val2);
    EXPECT_EQ(8, vec[1].val1);
    EXPECT_EQ(0.8f, vec[1].val2);
    EXPECT_EQ(8, foo3.val1);
    EXPECT_EQ(0.8f, foo3.val2);
}

TEST(BoundedVectorTests, Reset_EqualSize)
{
    BoundedVector<int> vec(3);

    vec.emplace_back(67);
    vec.emplace_back(68);
    vec.emplace_back(69);

    EXPECT_EQ(3u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(3);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, Reset_Smaller)
{
    BoundedVector<int> vec(3);

    vec.emplace_back(67);
    vec.emplace_back(68);
    vec.emplace_back(69);

    EXPECT_EQ(3u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, Reset_Larger)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(3);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetToZero)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset(0);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetOnZeroSize)
{
    BoundedVector<int> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(0u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.reset(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetOnEmpty)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.reset(3);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_TRUE(vec.empty());
}

TEST(BoundedVectorTests, ResetFull)
{
    BoundedVector<int> vec(2);

    EXPECT_EQ(0u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_TRUE(vec.empty());

    vec.reset_full(1);
    EXPECT_EQ(1u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.reset_full(3);
    EXPECT_EQ(3u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_FALSE(vec.empty());
}

TEST(BoundedVectorTests, EnsureSize_Smaller)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());

    vec.ensure_size(1);

    EXPECT_EQ(1u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_EQ(67, vec[0]);
}

TEST(BoundedVectorTests, EnsureSize_Larger)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());

    vec.ensure_size(3);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_EQ(67, vec[0]);
    EXPECT_EQ(68, vec[1]);
}

TEST(BoundedVectorTests, EnsureSizeFull_Smaller)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());

    vec.ensure_size_full(1);

    EXPECT_EQ(1u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_EQ(67, vec[0]);
}

TEST(BoundedVectorTests, EnsureSizeFull_Larger)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());

    vec.ensure_size_full(3);

    EXPECT_EQ(3u, vec.size());
    EXPECT_EQ(3u, vec.max_size());
    EXPECT_EQ(67, vec[0]);
    EXPECT_EQ(68, vec[1]);
}

TEST(BoundedVectorTests, GrowBy_Larger)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.grow_by(3);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(67, vec[0]);
    EXPECT_EQ(68, vec[1]);
    EXPECT_EQ(5u, vec.max_size());
    EXPECT_FALSE(vec.empty());
}

TEST(BoundedVectorTests, GrowBy_Zero)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(67);
    vec.emplace_back(68);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());

    vec.grow_by(0);

    EXPECT_EQ(2u, vec.size());
    EXPECT_EQ(67, vec[0]);
    EXPECT_EQ(68, vec[1]);
    EXPECT_EQ(2u, vec.max_size());
    EXPECT_FALSE(vec.empty());
}

TEST(BoundedVectorTests, Back)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(5);
    vec.emplace_back(6);

    ASSERT_EQ(2u, vec.size());

    EXPECT_EQ(6, vec.back());
}

TEST(BoundedVectorTests, Indexer)
{
    BoundedVector<int> vec(2);

    vec.emplace_back(5);
    vec.emplace_back(6);

    ASSERT_EQ(2u, vec.size());

    EXPECT_EQ(5, vec[0]);
    EXPECT_EQ(6, vec[1]);
}

TEST(BoundedVectorTests, Sort)
{
    BoundedVector<std::tuple<ElementIndex, float>> vec(6);
    vec.emplace_back(4, 5.0f);
    vec.emplace_back(15, 2.0f);
    vec.emplace_back(13, 3.0f);
    vec.emplace_back(0, 1.0f);

    vec.sort(
        [](auto const & t1, auto const & t2)
        {
            return std::get<1>(t1) < std::get<1>(t2);
        });

    EXPECT_EQ(4u, vec.size());

    EXPECT_EQ(0u, std::get<0>(vec[0]));
    EXPECT_EQ(15u, std::get<0>(vec[1]));
    EXPECT_EQ(13u, std::get<0>(vec[2]));
    EXPECT_EQ(4u, std::get<0>(vec[3]));
}