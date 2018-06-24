#include <GameLib/Vectors.h>

#include "gtest/gtest.h"

TEST(VectorsTests, Sum_2f)
{
	vec2f a(1.0f, 5.0f);
	vec2f b(2.0f, 4.0f);
	vec2f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
	EXPECT_EQ(c.y, 9.0f);
}
