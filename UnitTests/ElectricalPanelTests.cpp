#include <Game/ElectricalPanel.h>

#include "gtest/gtest.h"

TEST(ElectricalPanelTests, Add_FreePosition)
{
    ElectricalPanel panel;
    panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanelElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    ASSERT_EQ(1, panel.GetSize());

    auto const it = panel.cbegin();

    EXPECT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(IntegralCoordinates(12, 42), it->second.PanelCoordinates);

    EXPECT_TRUE(it->second.Label.has_value());
    EXPECT_EQ("foo", *it->second.Label);

    EXPECT_EQ(false, it->second.IsHidden);
}

TEST(ElectricalPanelTests, Add_OccupiedPosition)
{
    ElectricalPanel panel;
    panel.Add(
        ElectricalElementInstanceIndex(4),
        ElectricalPanelElementMetadata(IntegralCoordinates(12, 42), "foo", false));

    panel.Add(
        ElectricalElementInstanceIndex(43),
        ElectricalPanelElementMetadata(IntegralCoordinates(12, 42), "bar", true));

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
