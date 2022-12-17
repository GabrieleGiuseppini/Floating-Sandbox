#include <Game/ElectricalPanel.h>

#include "gtest/gtest.h"

TEST(ElectricalPanelTests, Merge_FreePosition)
{
    ElectricalPanel panel;
    panel.Merge(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_EQ(1u, panel.GetSize());

    auto const it = panel.begin();

    EXPECT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(IntegralCoordinates(12, 42), it->second.PanelCoordinates);

    EXPECT_TRUE(it->second.Label.has_value());
    EXPECT_EQ("foo", *it->second.Label);

    EXPECT_EQ(false, it->second.IsHidden);
}

TEST(ElectricalPanelTests, Merge_OccupiedPosition)
{
    ElectricalPanel panel;
    panel.Merge(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    panel.Merge(
        ElectricalElementInstanceIndex(43),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "bar", true));

    ASSERT_EQ(2u, panel.GetSize());

    auto it = panel.begin();
    if (it->first != 43)
    {
        ++it;
    }

    ASSERT_EQ(it->first, 43);

    EXPECT_FALSE(it->second.PanelCoordinates.has_value());

    EXPECT_TRUE(it->second.Label.has_value());
    EXPECT_EQ("bar", *it->second.Label);

    EXPECT_EQ(true, it->second.IsHidden);
}

TEST(ElectricalPanelTests, TryAdd_and_Find)
{
    ElectricalPanel panel;
    auto const [_, isInserted] = panel.TryAdd(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_TRUE(isInserted);
    ASSERT_FALSE(panel.IsEmpty());
    ASSERT_EQ(1u, panel.GetSize());

    auto const it1 = panel.Find(4);
    EXPECT_NE(it1, panel.end());

    auto const it2 = panel.Find(5);
    EXPECT_EQ(it2, panel.end());
}

TEST(ElectricalPanelTests, Contains)
{
    ElectricalPanel panel;
    panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_TRUE(panel.Contains(4));
    ASSERT_FALSE(panel.Contains(7));
}

TEST(ElectricalPanelTests, IndexOperator)
{
    ElectricalPanel panel;
    panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_FALSE(panel.IsEmpty());
    ASSERT_EQ(1u, panel.GetSize());

    auto const & elem = panel[4];

    EXPECT_TRUE(elem.PanelCoordinates.has_value());
    EXPECT_EQ(IntegralCoordinates(12, 42), elem.PanelCoordinates);

    EXPECT_TRUE(elem.Label.has_value());
    EXPECT_EQ("foo", *elem.Label);

    EXPECT_EQ(false, elem.IsHidden);
}