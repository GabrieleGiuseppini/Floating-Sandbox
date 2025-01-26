#include <Core/EnumFlags.h>

#include "gtest/gtest.h"

enum class TestEnum : uint8_t
{
    None = 0,
    Option1a = 1,
    Option1b = 0,
    Option2a = 2,
    Option2b = 0,
    Option3a = 4,
};

template <> struct is_flag<TestEnum> : std::true_type { };

TEST(EnumFlagsTests, BitOperators)
{
    TestEnum f1 = TestEnum::Option1a | TestEnum::Option2a;
    EXPECT_EQ(TestEnum::Option1a, f1 & TestEnum::Option1a);
    EXPECT_EQ(TestEnum::Option2a, f1 & TestEnum::Option2a);
    EXPECT_NE(TestEnum::Option3a, f1 & TestEnum::Option3a);
}

TEST(EnumFlagsTests, BooleanCast)
{
    TestEnum f1 = TestEnum::Option1a | TestEnum::Option2a;
    EXPECT_TRUE(!!f1);
    EXPECT_FALSE(!f1);
    EXPECT_TRUE(!!(f1 & TestEnum::Option1a));
    EXPECT_FALSE(!(f1 & TestEnum::Option1a));
    EXPECT_FALSE(!!(f1 & TestEnum::Option1b));
    EXPECT_TRUE(!!(f1 & TestEnum::Option2a));
    EXPECT_FALSE(!!(f1 & TestEnum::Option2b));
    EXPECT_FALSE(!!(f1 & TestEnum::Option3a));
}

class TestClass
{
public:
    enum class TestNestedEnum
    {
        Option1 = 1,
        Option2 = 2,
        Option3 = 4
    };
};

template <> struct is_flag<TestClass::TestNestedEnum> : std::true_type {};

TEST(EnumFlagsTests, BooleanCast_Nested)
{
    TestClass::TestNestedEnum f1 = TestClass::TestNestedEnum::Option1 | TestClass::TestNestedEnum::Option2;
    EXPECT_TRUE(!!f1);
    EXPECT_FALSE(!f1);
    EXPECT_TRUE(!!(f1 & TestClass::TestNestedEnum::Option1));
    EXPECT_FALSE(!(f1 & TestClass::TestNestedEnum::Option1));
    EXPECT_TRUE(!!(f1 & TestClass::TestNestedEnum::Option2));
    EXPECT_FALSE(!!(f1 & TestClass::TestNestedEnum::Option3));
}