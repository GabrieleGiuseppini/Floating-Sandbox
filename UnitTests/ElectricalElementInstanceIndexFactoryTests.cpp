#include <ShipBuilderLib/ElectricalElementInstanceIndexFactory.h>

#include "gtest/gtest.h"

namespace ShipBuilder {

TEST(ElectricalElementInstanceIndexFactoryTests, FromEmpty)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_First)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(0);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_Mid_One)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(1);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_Multiple)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(0);
    factory.DisposeIndex(1);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_Last)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(2);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_Last_Backwards)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(2);
    factory.DisposeIndex(1);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Dispose_All)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(0);
    factory.DisposeIndex(1);
    factory.DisposeIndex(2);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, RegisterIndex)
{
    ElectricalElementInstanceIndexFactory factory;

    factory.RegisterIndex(1);
    factory.RegisterIndex(2);

    auto instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(4, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, RegisterIndex_Zero)
{
    ElectricalElementInstanceIndexFactory factory;

    factory.RegisterIndex(0);

    auto instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, RegisterIndex_FirstFree)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(1, instanceId);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.DisposeIndex(1);

    factory.RegisterIndex(1);

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(3, instanceId);
}

TEST(ElectricalElementInstanceIndexFactoryTests, Reset)
{
    ElectricalElementInstanceIndexFactory factory;

    auto instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(2, instanceId);

    factory.Reset();

    instanceId = factory.MakeNewIndex();
    EXPECT_EQ(0, instanceId);
}

}