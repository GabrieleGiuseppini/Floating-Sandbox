#include <GameCore/IndexRemap.h>

#include "gtest/gtest.h"

TEST(IndexRemapTestsTests, Basic)
{
    IndexRemap remap(10);

    remap.AddOld(5);
    remap.AddOld(0);

    EXPECT_EQ(ElementIndex(0), remap.OldToNew(5));
    EXPECT_EQ(ElementIndex(5), remap.NewToOld(0));

    EXPECT_EQ(ElementIndex(1), remap.OldToNew(0));
    EXPECT_EQ(ElementIndex(0), remap.NewToOld(1));
}

TEST(IndexRemapTestsTests, Idempotent)
{
    IndexRemap remap = IndexRemap::MakeIdempotent(10);

    EXPECT_EQ(ElementIndex(0), remap.OldToNew(0));
    EXPECT_EQ(ElementIndex(0), remap.NewToOld(0));

    EXPECT_EQ(ElementIndex(9), remap.OldToNew(9));
    EXPECT_EQ(ElementIndex(9), remap.NewToOld(9));
}