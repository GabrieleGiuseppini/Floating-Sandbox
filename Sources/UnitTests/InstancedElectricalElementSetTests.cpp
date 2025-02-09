#include <ShipBuilderLib/InstancedElectricalElementSet.h>

#include "TestingUtils.h"

#include "gtest/gtest.h"

namespace ShipBuilder {

TEST(InstancedElectricalElementSetTests, FromEmpty)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);
}

TEST(InstancedElectricalElementSetTests, IsRegistered)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);

    EXPECT_TRUE(elementSet.IsRegistered(instanceId));
    EXPECT_FALSE(elementSet.IsRegistered(instanceId + 1));
}

TEST(InstancedElectricalElementSetTests, Remove_First)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(0);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSetTests, Remove_Mid_One)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(1);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSetTests, Remove_Multiple)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(0);
    elementSet.Remove(1);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSetTests, Remove_Last)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(2);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSetTests, Remove_Last_Backwards)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(2);
    elementSet.Remove(1);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSetTests, Remove_All)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(0);
    elementSet.Remove(1);
    elementSet.Remove(2);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSet, RegisterIndex)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    elementSet.Register(1, &material);
    elementSet.Register(2, &material);

    auto instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(4, instanceId);
}

TEST(InstancedElectricalElementSet, RegisterIndex_Zero)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    elementSet.Register(0, &material);

    auto instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);
}

TEST(InstancedElectricalElementSet, RegisterIndex_FirstFree)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(1, instanceId);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Remove(1);

    elementSet.Register(1, &material);

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(3, instanceId);
}

TEST(InstancedElectricalElementSet, Reset)
{
    InstancedElectricalElementSet elementSet;

    auto const material = MakeTestElectricalMaterial("mat1", rgbColor(1, 2, 3), true);

    auto instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    instanceId = elementSet.Add(&material);
    EXPECT_EQ(2, instanceId);

    elementSet.Reset();

    instanceId = elementSet.Add(&material);
    EXPECT_EQ(0, instanceId);
}

}