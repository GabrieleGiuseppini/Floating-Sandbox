#include <ShipBuilderLib/RegionCombiner.h>

#include "gtest/gtest.h"

TEST(RegionCombinerTests, One)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));

	ASSERT_EQ(combiner.GetRegionCount(), 1u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));
}

TEST(RegionCombinerTests, Two_Disjoint)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(100, 100),
			ShipSpaceSize(30, 20)));

	ASSERT_EQ(combiner.GetRegionCount(), 2u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));

	EXPECT_EQ(combiner.GetRegionAt(1),
		ShipSpaceRect(
			ShipSpaceCoordinates(100, 100),
			ShipSpaceSize(30, 20)));
}

TEST(RegionCombinerTests, Two_OneIncludedInOther)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(10, 20)));

	ASSERT_EQ(combiner.GetRegionCount(), 1u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 30)));
}

TEST(RegionCombinerTests, Two_Overlapping)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(20, 20),
			ShipSpaceSize(20, 20)));

	ASSERT_EQ(combiner.GetRegionCount(), 1u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(30, 30)));
}

TEST(RegionCombinerTests, Three_ThirdOverlappingWithSecond)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(40, 40),
			ShipSpaceSize(20, 20)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(50, 50),
			ShipSpaceSize(20, 20)));

	ASSERT_EQ(combiner.GetRegionCount(), 2u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));

	EXPECT_EQ(combiner.GetRegionAt(1),
		ShipSpaceRect(
			ShipSpaceCoordinates(40, 40),
			ShipSpaceSize(30, 30)));
}

TEST(RegionCombinerTests, Three_ThirdOverlappingWithBoth)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(40, 40),
			ShipSpaceSize(20, 20)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(20, 20),
			ShipSpaceSize(30, 30)));

	ASSERT_EQ(combiner.GetRegionCount(), 1u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(50, 50)));
}

// Optimizations

TEST(RegionCombinerTests, Two_TouchingCorners)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(10, 10)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(20, 20),
			ShipSpaceSize(10, 10)));

	// Expected combined, as wasted area == useful area

	ASSERT_EQ(combiner.GetRegionCount(), 1u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));
}

TEST(RegionCombinerTests, Three_TouchingCorners)
{
	ShipBuilder::RegionCombiner<struct ShipSpaceTag> combiner;

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(10, 10)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(20, 20),
			ShipSpaceSize(10, 10)));

	combiner.AddRegion(
		ShipSpaceRect(
			ShipSpaceCoordinates(30, 30),
			ShipSpaceSize(10, 10)));

	// Expected only first two combined, as with last wasted area > useful area

	ASSERT_EQ(combiner.GetRegionCount(), 2u);

	EXPECT_EQ(combiner.GetRegionAt(0),
		ShipSpaceRect(
			ShipSpaceCoordinates(10, 10),
			ShipSpaceSize(20, 20)));

	EXPECT_EQ(combiner.GetRegionAt(1),
		ShipSpaceRect(
			ShipSpaceCoordinates(30, 30),
			ShipSpaceSize(10, 10)));
}
