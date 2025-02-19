#include <Core/SysSpecifics.h>

#include "gtest/gtest.h"

TEST(SysSpecificsTests, CeilPowerOfTwo)
{
    EXPECT_EQ(ceil_power_of_two(0), 1);
    EXPECT_EQ(ceil_power_of_two(1), 1);
    EXPECT_EQ(ceil_power_of_two(2), 2);
    EXPECT_EQ(ceil_power_of_two(3), 4);
    EXPECT_EQ(ceil_power_of_two(4), 4);
    EXPECT_EQ(ceil_power_of_two(5), 8);
    EXPECT_EQ(ceil_power_of_two(6), 8);
    EXPECT_EQ(ceil_power_of_two(7), 8);
    EXPECT_EQ(ceil_power_of_two(8), 8);
    EXPECT_EQ(ceil_power_of_two(9), 16);
}

TEST(SysSpecificsTests, CeilSquarePowerOfTwo)
{
    EXPECT_EQ(ceil_square_power_of_two(0), 0);
    EXPECT_EQ(ceil_square_power_of_two(1), 1);
    EXPECT_EQ(ceil_square_power_of_two(2), 4);
    EXPECT_EQ(ceil_square_power_of_two(3), 4);
    EXPECT_EQ(ceil_square_power_of_two(4), 4);
    EXPECT_EQ(ceil_square_power_of_two(5), 16);
    EXPECT_EQ(ceil_square_power_of_two(6), 16);
    EXPECT_EQ(ceil_square_power_of_two(7), 16);
    EXPECT_EQ(ceil_square_power_of_two(8), 16);
    EXPECT_EQ(ceil_square_power_of_two(9), 16);
    EXPECT_EQ(ceil_square_power_of_two(16), 16);
    EXPECT_EQ(ceil_square_power_of_two(17), 64);
    EXPECT_EQ(ceil_square_power_of_two(18), 64);
    EXPECT_EQ(ceil_square_power_of_two(63), 64);
    EXPECT_EQ(ceil_square_power_of_two(64), 64);
    EXPECT_EQ(ceil_square_power_of_two(65), 256);
}

TEST(SysSpecificsTests, AlignedAllocation_FromStdLib)
{
    auto buf1 = make_unique_buffer_aligned_to_vectorization_word<float>(100);
    EXPECT_TRUE(is_aligned_to_vectorization_word(buf1.get()));
}

TEST(SysSpecificsTests, AlignedAllocation_PoorMans)
{
    std::vector<void *> ptrs;
    for (int t = 0; t < 20; ++t)
    {
        void * bufPtr1 = _poor_mans_alloc_aligned_to_vectorization_word((16000 + t) * sizeof(float) + t);
        EXPECT_TRUE(is_aligned_to_vectorization_word(bufPtr1));

        ptrs.push_back(bufPtr1);
    }

    for (void * ptr : ptrs)
    {
        _poor_mans_free_aligned(ptr);
    }
}