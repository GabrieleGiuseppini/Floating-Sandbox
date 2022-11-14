#include <Game/ElectricalPanel.h>

#include "gtest/gtest.h"

TEST(ElectricalPanelTests, SafeAdd_FreePosition)
{
    ElectricalPanel panel;
    panel.SafeAdd(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_EQ(1, panel.GetSize());

    auto const it = panel.cbegin();

    EXPECT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(IntegralCoordinates(12, 42), it->second.PanelCoordinates);

    EXPECT_TRUE(it->second.Label.has_value());
    EXPECT_EQ("foo", *it->second.Label);

    EXPECT_EQ(false, it->second.IsHidden);
}

TEST(ElectricalPanelTests, SafeAdd_OccupiedPosition)
{
    ElectricalPanel panel;
    panel.SafeAdd(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    panel.SafeAdd(
        ElectricalElementInstanceIndex(43),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "bar", true));

    ASSERT_EQ(2, panel.GetSize());

    auto it = panel.cbegin();
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

TEST(ElectricalPanelTests, Add_and_Find)
{
    ElectricalPanel panel;
    bool const isInserted = panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_TRUE(isInserted);
    ASSERT_FALSE(panel.IsEmpty());
    ASSERT_EQ(1, panel.GetSize());

    auto const it1 = panel.find(4);
    EXPECT_NE(it1, panel.cend());

    auto const it2 = panel.find(5);
    EXPECT_EQ(it2, panel.cend());
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
    bool const isInserted = panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanel::ElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_TRUE(isInserted);
    ASSERT_FALSE(panel.IsEmpty());
    ASSERT_EQ(1, panel.GetSize());

    auto const & elem = panel[4];

    EXPECT_TRUE(elem.PanelCoordinates.has_value());
    EXPECT_EQ(IntegralCoordinates(12, 42), elem.PanelCoordinates);

    EXPECT_TRUE(elem.Label.has_value());
    EXPECT_EQ("foo", *elem.Label);

    EXPECT_EQ(false, elem.IsHidden);
}