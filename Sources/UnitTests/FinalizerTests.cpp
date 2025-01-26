#include <Core/Finalizer.h>

#include <optional>

#include "gtest/gtest.h"

TEST(FinalizerTests, Basic)
{
    int a = 0;

    std::optional<Finalizer> obj;
    obj.emplace([&a]() { ++a; });

    EXPECT_EQ(0, a);

    obj.reset();

    EXPECT_EQ(1, a);
}
