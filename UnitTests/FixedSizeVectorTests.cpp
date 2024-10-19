#include <GameCore/FixedSizeVector.h>

#include <GameCore/GameTypes.h>

#include "gtest/gtest.h"

TEST(FixedSizeVectorTests, Empty)
{
    FixedSizeVector<int, 6> vec;

    EXPECT_EQ(0u, vec.size());
    EXPECT_TRUE(vec.empty());
}

TEST(FixedSizeVectorTests, PushBack)
{
	FixedSizeVector<int, 6> vec;

    vec.push_back(4);

    EXPECT_EQ(1u, vec.size());
    EXPECT_FALSE(vec.empty());

    vec.push_back(6);

    EXPECT_EQ(2u, vec.size());
    EXPECT_FALSE(vec.empty());
}

TEST(FixedSizeVectorTests, PushFront)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(4);
    vec.push_back(5);

    EXPECT_EQ(2u, vec.size());
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(4, vec[0]);
    EXPECT_EQ(5, vec[1]);

    vec.push_front(6);

    EXPECT_EQ(3u, vec.size());
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(6, vec[0]);
    EXPECT_EQ(4, vec[1]);
    EXPECT_EQ(5, vec[2]);
}

TEST(FixedSizeVectorTests, PushFront_OnEmpty)
{
    FixedSizeVector<int, 6> vec;

    vec.push_front(6);

    EXPECT_EQ(1u, vec.size());
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(6, vec[0]);
}

TEST(FixedSizeVectorTests, EmplaceBack)
{
    struct Elem
    {
        int val1;
        float val2;

        Elem()
            : val1(0)
            , val2(0)
        {}

        Elem(
            int _val1,
            float _val2)
            : val1(_val1)
            , val2(_val2)
        {}
    };

    FixedSizeVector<Elem, 6> vec;

    auto const & newElem1 = vec.emplace_back(4, 8.0f);

    EXPECT_EQ(1u, vec.size());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(4, newElem1.val1);

    auto & newElem2 = vec.emplace_back(6, 12.0f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(6, newElem2.val1);

    EXPECT_EQ(4, vec[0].val1);
    EXPECT_EQ(8.0f, vec[0].val2);

    EXPECT_EQ(6, vec[1].val1);
    EXPECT_EQ(12.0f, vec[1].val2);
}

TEST(FixedSizeVectorTests, EmplaceFront)
{
    struct Elem
    {
        int val1;
        float val2;

        Elem()
            : val1(0)
            , val2(0)
        {}

        Elem(
            int _val1,
            float _val2)
            : val1(_val1)
            , val2(_val2)
        {}
    };

    FixedSizeVector<Elem, 6> vec;

    vec.emplace_back(4, 8.0f);
    vec.emplace_back(6, 12.0f);

    EXPECT_EQ(2u, vec.size());
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(4, vec[0].val1);
    EXPECT_EQ(8.0f, vec[0].val2);

    EXPECT_EQ(6, vec[1].val1);
    EXPECT_EQ(12.0f, vec[1].val2);

    vec.emplace_front(8, 16.0f);

    EXPECT_EQ(3u, vec.size());
    EXPECT_FALSE(vec.empty());

    EXPECT_EQ(8, vec[0].val1);
    EXPECT_EQ(16.0f, vec[0].val2);

    EXPECT_EQ(4, vec[1].val1);
    EXPECT_EQ(8.0f, vec[1].val2);

    EXPECT_EQ(6, vec[2].val1);
    EXPECT_EQ(12.0f, vec[2].val2);
}

TEST(FixedSizeVectorTests, IteratesElements)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    auto it = vec.begin();

    EXPECT_NE(it, vec.end());
    EXPECT_EQ(3, *it);

    ++it;

    EXPECT_NE(it, vec.end());
    EXPECT_EQ(2, *it);

    ++it;

    EXPECT_NE(it, vec.end());
    EXPECT_EQ(1, *it);

    ++it;

    EXPECT_EQ(it, vec.end());
}

TEST(FixedSizeVectorTests, IteratesElements_Const)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    auto const & vecRef = vec;

    auto it = vecRef.begin();

    EXPECT_NE(it, vecRef.end());
    EXPECT_EQ(3, *it);

    ++it;

    EXPECT_NE(it, vecRef.end());
    EXPECT_EQ(2, *it);

    ++it;

    EXPECT_NE(it, vecRef.end());
    EXPECT_EQ(1, *it);

    ++it;

    EXPECT_EQ(it, vecRef.end());
}

TEST(FixedSizeVectorTests, IteratesElements_ForLoop)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    size_t sum = 0;
    for (auto i : vec)
    {
        sum += i;
    }

    EXPECT_EQ(6u, sum);
}

TEST(FixedSizeVectorTests, IteratesElements_ForLoop_Const)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    auto const & vecRef = vec;

    size_t sum = 0;
    for (auto i : vecRef)
    {
        sum += i;
    }

    EXPECT_EQ(6u, sum);
}

TEST(FixedSizeVectorTests, IteratesElements_Index)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    size_t sum = 0;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        sum += vec[i];
    }

    EXPECT_EQ(6u, sum);
}

TEST(FixedSizeVectorTests, IteratesElements_Index_Const)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);

    auto const & vecRef = vec;

    size_t sum = 0;
    for (size_t i = 0; i < vecRef.size(); ++i)
    {
        sum += vecRef[i];
    }

    EXPECT_EQ(6u, sum);
}

TEST(FixedSizeVectorTests, Erase_BecomesEmpty)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);

    vec.erase(0);

    EXPECT_EQ(0u, vec.size());
}

TEST(FixedSizeVectorTests, Erase_Copies_First)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.erase(0);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(2, vec[0]);
    EXPECT_EQ(3, vec[1]);
}

TEST(FixedSizeVectorTests, Erase_Copies_Middle)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.erase(1);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(3, vec[1]);
}

TEST(FixedSizeVectorTests, Erase_Copies_Last)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.erase(2);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(2, vec[1]);
}

TEST(FixedSizeVectorTests, EraseFirst_Empty)
{
    FixedSizeVector<int, 6> vec;

    bool result = vec.erase_first(3);

    EXPECT_FALSE(result);
    EXPECT_EQ(0u, vec.size());
}

TEST(FixedSizeVectorTests, EraseFirst_BecomesEmpty)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);

    bool result = vec.erase_first(3);

    EXPECT_TRUE(result);
    EXPECT_EQ(0u, vec.size());
}

TEST(FixedSizeVectorTests, EraseFirst_NotFound)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(3);

    bool result = vec.erase_first(4);

    EXPECT_FALSE(result);
    EXPECT_EQ(1u, vec.size());
}

TEST(FixedSizeVectorTests, EraseFirst_Copies_First)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    bool result = vec.erase_first(1);

    EXPECT_TRUE(result);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(2, vec[0]);
    EXPECT_EQ(3, vec[1]);
}

TEST(FixedSizeVectorTests, EraseFirst_Copies_Middle)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    bool result = vec.erase_first(2);

    EXPECT_TRUE(result);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(3, vec[1]);
}

TEST(FixedSizeVectorTests, EraseFirst_Copies_Last)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    bool result = vec.erase_first(3);

    EXPECT_TRUE(result);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(2, vec[1]);
}

TEST(FixedSizeVectorTests, EraseFirst_Lambda_Copies_Middle)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    bool result = vec.erase_first(
        [](int const & elem)
        {
            return elem * 4 == 8;
        });

    EXPECT_TRUE(result);

    ASSERT_EQ(2u, vec.size());
    EXPECT_EQ(1, vec[0]);
    EXPECT_EQ(3, vec[1]);
}

TEST(FixedSizeVectorTests, Back)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(4);

    EXPECT_EQ(4, vec.back());

    vec.push_back(6);

    EXPECT_EQ(6, vec.back());
}

TEST(FixedSizeVectorTests, Clear)
{
    FixedSizeVector<int, 6> vec;

    vec.push_back(4);

    ASSERT_FALSE(vec.empty());
    ASSERT_EQ(1u, vec.size());

    vec.clear();

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0u, vec.size());
}

TEST(FixedSizeVectorTests, Fill)
{
    FixedSizeVector<int, 6> vec;

    ASSERT_TRUE(vec.empty());

    vec.fill(242);

    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(6u, vec.size());

    EXPECT_EQ(242, vec[0]);
    EXPECT_EQ(242, vec[5]);
}

TEST(FixedSizeVectorTests, Sort)
{
    FixedSizeVector<std::tuple<ElementIndex, float>, 6> vec;
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