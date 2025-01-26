#include <Core/StrongTypeDef.h>

#include "gtest/gtest.h"

TEST(StrongTypeDefTests, Basic)
{
	StrongTypeDef<int, struct MyTag> val1(2);
	StrongTypeDef<int, struct MyTag> val2(3);
	StrongTypeDef<int, struct MyTag> val3(2);

	EXPECT_NE(val1, val2);
	EXPECT_EQ(val1, val3);
}

TEST(StrongTypeDefTests, Boolean_Constructor)
{
	StrongTypedBool<struct MyTag> val1(true);
	StrongTypedBool<struct MyTag> val2(false);

	EXPECT_TRUE(val1);
	EXPECT_FALSE(val2);
}

TEST(StrongTypeDefTests, Boolean_Constants)
{
	EXPECT_TRUE(StrongTypedTrue<struct MyTag>);
	EXPECT_FALSE(StrongTypedFalse<struct MyTag>);
}

namespace {

int MyBooleanTakingFunction(StrongTypedBool<struct IsBlocking> isBlocking, StrongTypedBool<struct UseMagic> useMagic)
{
	int res = 0;

	if (isBlocking)
	{
		// Something blocking
		res += 1;
	}

	if (useMagic)
	{
		// Eene meene Hexerei
		res += 2;
	}

	return res;
}

}

TEST(StrongTypeDefTests, Boolean_FunctionArgsWithConstants)
{
	auto const res = MyBooleanTakingFunction(
		StrongTypedTrue<IsBlocking>,
		StrongTypedFalse<UseMagic>);

	EXPECT_EQ(res, 1);
}