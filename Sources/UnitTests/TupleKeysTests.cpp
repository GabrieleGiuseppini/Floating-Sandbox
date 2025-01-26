#include <Core/TupleKeys.h>

#include <unordered_map>

#include "gtest/gtest.h"

TEST(TupleKeysTests, Hasher_OneElementTuple)
{
    std::tuple<int> t1(4);
    std::tuple<int> t2(5);
    std::tuple<int> t3(4);

    TupleHasher<std::tuple<int>> hasher;
    EXPECT_NE(hasher(t1), hasher(t2));
    EXPECT_NE(hasher(t2), hasher(t3));
    EXPECT_EQ(hasher(t1), hasher(t3));
}

TEST(TupleKeysTests, Hasher_TwoElementTuple_DifferentTypes)
{
    std::tuple<int, std::string> t1(4, "foo");
    std::tuple<int, std::string> t2(5, "foo");
    std::tuple<int, std::string> t3(4, "bar");
    std::tuple<int, std::string> t4(4, "foo");

    TupleHasher<std::tuple<int, std::string>> hasher;
    EXPECT_NE(hasher(t1), hasher(t2));
    EXPECT_NE(hasher(t1), hasher(t3));
    EXPECT_NE(hasher(t2), hasher(t4));
    EXPECT_NE(hasher(t3), hasher(t4));
    EXPECT_EQ(hasher(t1), hasher(t4));
}

TEST(TupleKeysTests, Hasher_TwoElementTuple_SameTypes)
{
    std::tuple<int, int> t1(4, 100);
    std::tuple<int, int> t2(5, 100);
    std::tuple<int, int> t3(4, 101);
    std::tuple<int, int> t4(4, 100);

    TupleHasher<std::tuple<int, int>> hasher;
    EXPECT_NE(hasher(t1), hasher(t2));
    EXPECT_NE(hasher(t1), hasher(t3));
    EXPECT_NE(hasher(t2), hasher(t4));
    EXPECT_NE(hasher(t3), hasher(t4));
    EXPECT_EQ(hasher(t1), hasher(t4));
}

TEST(TupleKeysTests, Hasher_TwoElementTuple_WithBoolean)
{
    std::tuple<int, bool> t1(4, false);
    std::tuple<int, bool> t2(5, false);
    std::tuple<int, bool> t3(4, true);
    std::tuple<int, bool> t4(4, false);

    TupleHasher<std::tuple<int, bool>> hasher;
    EXPECT_NE(hasher(t1), hasher(t2));
    EXPECT_NE(hasher(t1), hasher(t3));
    EXPECT_NE(hasher(t2), hasher(t4));
    EXPECT_NE(hasher(t3), hasher(t4));
    EXPECT_EQ(hasher(t1), hasher(t4));
}

TEST(TupleKeysTests, UnorderedMap)
{
    std::tuple<int, std::string> t1(4, "foo");
    std::tuple<int, std::string> t2(5, "foo");
    std::tuple<int, std::string> t3(4, "bar");
    std::tuple<int, std::string> t4(4, "foo");

    unordered_tuple_map<std::tuple<int, std::string>, int> map;
    map[t1] = 1;
    map[t2] = 2;
    map[t3] = 3;
    map[t4] = 4;

    EXPECT_EQ(4, map[t1]);
    EXPECT_EQ(2, map[t2]);
    EXPECT_EQ(3, map[t3]);
    EXPECT_EQ(4, map[t4]);
}

TEST(TupleKeysTests, UnorderedSet)
{
    std::tuple<int, std::string> t1(4, "foo");
    std::tuple<int, std::string> t2(5, "foo");
    std::tuple<int, std::string> t3(4, "bar");
    std::tuple<int, std::string> t4(4, "foo");

    unordered_tuple_set<std::tuple<int, std::string>> set;
    set.insert(t1);
    set.insert(t2);
    set.insert(t3);
    set.insert(t4);

    EXPECT_EQ(3u, set.size());
}