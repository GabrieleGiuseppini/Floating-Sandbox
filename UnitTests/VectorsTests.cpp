#include <GameCore/Vectors.h>

#include "gtest/gtest.h"

TEST(VectorsTests, Sum_2f)
{
	vec2f a(1.0f, 5.0f);
	vec2f b(2.0f, 4.0f);
	vec2f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
	EXPECT_EQ(c.y, 9.0f);
}

TEST(VectorsTests, Sum_4f)
{
    vec4f a(1.0f, 5.0f, 20.0f, 100.4f);
    vec4f b(2.0f, 4.0f, 40.0f, 200.0f);
    vec4f c = a + b;

    EXPECT_EQ(c.x, 3.0f);
    EXPECT_EQ(c.y, 9.0f);
    EXPECT_EQ(c.z, 60.0f);
    EXPECT_EQ(c.w, 300.4f);
}