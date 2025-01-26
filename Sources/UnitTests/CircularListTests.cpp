#include <Core/CircularList.h>

#include "gtest/gtest.h"

#include <optional>

TEST(CircularListTests, Emplace_LessThanMax)
{
    std::vector<int> removed;

    CircularList<int, 6> cl;

    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(0u, cl.size());

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        1);

    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(1u, cl.size());
    EXPECT_EQ(0u, removed.size());

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        1);

    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(2u, cl.size());
    EXPECT_EQ(0u, removed.size());
}

TEST(CircularListTests, Emplace_MoreThanMax_RemovesOld)
{
    std::vector<int> removed;

    CircularList<int, 4> cl;

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        10);

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        20);

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        30);

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        40);

    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(0u, removed.size());

    cl.emplace(
        [&removed](int value)
        {
            removed.push_back(value);
        },
        50);

    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(4u, cl.size());
    ASSERT_EQ(1u, removed.size());
    EXPECT_EQ(10, removed.front());
}

TEST(CircularListTests, Emplace_MoreThanMax_RemovesOld_ManyTimes)
{
    std::vector<int> removed;

    CircularList<int, 4> cl;

    for (int i = 10; i <= 40; i += 10)
    {
        cl.emplace(
            [&removed](int value)
            {
                removed.push_back(value);
            },
            i);
    }

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(0u, removed.size());

    for (int i = 50; i <= 120; i += 10)
    {
        cl.emplace(
            [&removed](int value)
            {
                removed.push_back(value);
            },
            i);
    }

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(8u, removed.size());
    EXPECT_EQ(10, removed[0]);
    EXPECT_EQ(20, removed[1]);
    EXPECT_EQ(30, removed[2]);
    EXPECT_EQ(40, removed[3]);
    EXPECT_EQ(50, removed[4]);
    EXPECT_EQ(60, removed[5]);
    EXPECT_EQ(70, removed[6]);
    EXPECT_EQ(80, removed[7]);
}

TEST(CircularListTests, Clear)
{
    CircularList<int, 6> cl;

    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(0u, cl.size());

    cl.emplace(
        [](int)
        {
        },
        1);

    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(1u, cl.size());

    cl.clear();

    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(0u, cl.size());
}

TEST(CircularListTests, Iterator_Explicit_Empty)
{
    CircularList<int, 6> cl;

    std::vector<int> vals;
    for (auto it = cl.begin(); it != cl.end(); ++it)
    {
        vals.push_back(*it);
    }

    EXPECT_EQ(0u, vals.size());
}

TEST(CircularListTests, Iterator_Range_Empty)
{
    CircularList<int, 6> cl;

    std::vector<int> vals;
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    EXPECT_EQ(0u, vals.size());
}

TEST(CircularListTests, Iterator_Explicit_LessThaxMax)
{
    CircularList<int, 6> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());

    std::vector<int> vals;
    for (auto it = cl.begin(); it != cl.end(); ++it)
    {
        vals.push_back(*it);
    }

    ASSERT_EQ(4u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);
    EXPECT_EQ(20, vals[2]);
    EXPECT_EQ(10, vals[3]);
}

TEST(CircularListTests, Iterator_Const_Explicit_LessThaxMax)
{
    CircularList<int, 6> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());

    CircularList<int, 6> const & cl2 = cl;

    std::vector<int> vals;
    for (auto it = cl2.begin(); it != cl2.end(); ++it)
    {
        vals.push_back(*it);
    }

    ASSERT_EQ(4u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);
    EXPECT_EQ(20, vals[2]);
    EXPECT_EQ(10, vals[3]);
}

TEST(CircularListTests, Iterator_Range_LessThaxMax)
{
    CircularList<int, 6> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());

    std::vector<int> vals;
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(4u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);
    EXPECT_EQ(20, vals[2]);
    EXPECT_EQ(10, vals[3]);
}

TEST(CircularListTests, Iterator_MoreThaxMax)
{
    CircularList<int, 4> cl;

    for (int i = 10; i <= 100; i += 10)
    {
        cl.emplace(
            [](int){}, i);
    }

    EXPECT_EQ(4u, cl.size());

    std::vector<int> vals;
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(4u, vals.size());
    EXPECT_EQ(100, vals[0]);
    EXPECT_EQ(90, vals[1]);
    EXPECT_EQ(80, vals[2]);
    EXPECT_EQ(70, vals[3]);
}

TEST(CircularListTests, Size_MoreThanMax_ManyTimes)
{
    CircularList<int, 4> cl;

    for (size_t i = 0; i < 11; ++i)
    {
        if (i < 4)
            EXPECT_EQ(i, cl.size());
        else
            EXPECT_EQ(4u, cl.size());

        cl.emplace(
            [](int)
            {
            },
            static_cast<int>(i));

        if (i < 4)
            EXPECT_EQ(i + 1, cl.size());
        else
            EXPECT_EQ(4u, cl.size());
    }
}

TEST(CircularListTests, Erase_TailHead_Head)
{
    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(40, *(cl.begin()));

    auto it = cl.erase(cl.begin());

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *(cl.begin()));
    EXPECT_EQ(30, *(it));

    it = cl.erase(cl.begin());

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(20, *(cl.begin()));
    EXPECT_EQ(20, *(it));

    it = cl.erase(cl.begin());

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(10, *(cl.begin()));
    EXPECT_EQ(10, *(it));

    it = cl.erase(cl.begin());

    EXPECT_EQ(0u, cl.size());
    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(it, cl.end());
}

TEST(CircularListTests, Erase_TailHead_HeadMinusOne)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(40, *(cl.begin()));

    auto it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(20, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(20, vals[1]);
    EXPECT_EQ(10, vals[2]);

    it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(10, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(10, vals[1]);

    it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(cl.end(), it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(40, vals[0]);
}

TEST(CircularListTests, Erase_TailHead_Tail)
{
    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(40, *(cl.begin()));

    int i = 0;
    auto itRes = cl.begin();
    for (auto it = cl.begin(); it != cl.end(); ++it, ++i)
    {
        if (i == 3)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    i = 40;
    for (auto it = cl.begin(); it != cl.end(); ++it, i -= 10)
    {
        EXPECT_EQ(i, *it);
    }

    i = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++i)
    {
        if (i == 2)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    i = 40;
    for (auto it = cl.begin(); it != cl.end(); ++it, i -= 10)
    {
        EXPECT_EQ(i, *it);
    }

    i = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++i)
    {
        if (i == 1)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    i = 40;
    for (auto it = cl.begin(); it != cl.end(); ++it, i -= 10)
    {
        EXPECT_EQ(i, *it);
    }

    i = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++i)
    {
        if (i == 0)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(0u, cl.size());
    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(itRes, cl.end());
}

TEST(CircularListTests, Erase_TailHead_TailPlusOne)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(40, *(cl.begin()));

    int iCheck = 0;
    auto itRes = cl.begin();
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 2)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(10, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);
    EXPECT_EQ(10, vals[2]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 1)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(10, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(10, vals[1]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 0)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(10, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(10, vals[0]);
}

TEST(CircularListTests, Erase_HeadTail_Head)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);
    cl.emplace([](int) {}, 50);
    cl.emplace([](int) {}, 60);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(60, *(cl.begin()));

    auto it = cl.erase(cl.begin());

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(50, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(50, vals[0]);
    EXPECT_EQ(40, vals[1]);
    EXPECT_EQ(30, vals[2]);

    it = cl.erase(cl.begin());

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(40, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);

    it = cl.erase(cl.begin());

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(30, vals[0]);

    it = cl.erase(cl.begin());

    EXPECT_EQ(0u, cl.size());
    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(cl.end(), it);
}

TEST(CircularListTests, Erase_HeadTail_HeadMinusOne)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);
    cl.emplace([](int) {}, 50);
    cl.emplace([](int) {}, 60);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(60, *(cl.begin()));

    auto it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(40, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(40, vals[1]);
    EXPECT_EQ(30, vals[2]);

    it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(30, vals[1]);

    it = cl.erase(std::next(cl.begin()));

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(cl.end(), it);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(60, vals[0]);
}

TEST(CircularListTests, Erase_HeadTail_Tail)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);
    cl.emplace([](int) {}, 50);
    cl.emplace([](int) {}, 60);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(60, *(cl.begin()));

    int iCheck = 0;
    auto itRes = cl.begin();
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 3)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(50, vals[1]);
    EXPECT_EQ(40, vals[2]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 2)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(50, vals[1]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 1)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(itRes, cl.end());

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(60, vals[0]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 0)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(0u, cl.size());
    EXPECT_TRUE(cl.empty());
    EXPECT_EQ(itRes, cl.end());
}

TEST(CircularListTests, Erase_HeadTail_TailPlusOne)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);
    cl.emplace([](int) {}, 50);
    cl.emplace([](int) {}, 60);

    EXPECT_EQ(4u, cl.size());
    EXPECT_EQ(60, *(cl.begin()));

    int iCheck = 0;
    auto itRes = cl.begin();
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 2)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(50, vals[1]);
    EXPECT_EQ(30, vals[2]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 1)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(2u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(2u, vals.size());
    EXPECT_EQ(60, vals[0]);
    EXPECT_EQ(30, vals[1]);

    iCheck = 0;
    for (auto it = cl.begin(); it != cl.end(); ++it, ++iCheck)
    {
        if (iCheck == 0)
        {
            itRes = cl.erase(it);
            break;
        }
    }

    EXPECT_EQ(1u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(30, *itRes);

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(1u, vals.size());
    EXPECT_EQ(30, vals[0]);
}

TEST(CircularListTests, Erase_ByElement)
{
    std::vector<int> vals;

    CircularList<int, 4> cl;

    cl.emplace([](int) {}, 10);
    cl.emplace([](int) {}, 20);
    cl.emplace([](int) {}, 30);
    cl.emplace([](int) {}, 40);

    EXPECT_EQ(4u, cl.size());
    EXPECT_FALSE(cl.empty());
    EXPECT_EQ(40, *(cl.begin()));

    cl.erase(20);

    EXPECT_EQ(3u, cl.size());
    EXPECT_FALSE(cl.empty());

    vals.clear();
    for (auto i : cl)
    {
        vals.push_back(i);
    }

    ASSERT_EQ(3u, vals.size());
    EXPECT_EQ(40, vals[0]);
    EXPECT_EQ(30, vals[1]);
    EXPECT_EQ(10, vals[2]);
}