#include <Game/Layers.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(LayerTests, StructuralLayer_Trim)
{
    //
    // Create source layer
    //

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceBuffer(8, 6);

    std::vector<StructuralMaterial> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2)));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(&(materials.back()));
        }
    }

    StructuralLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Trim layer
    //

    StructuralLayerData targetLayer = sourceLayer.Clone();

    targetLayer.Trim({
        { 2, 1},
        { 4, 3 } });

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(4, 3));

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            if (x >= 4 || y >= 3)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], StructuralElement(nullptr));
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ShipSpaceSize(2, 1)]);
            }
        }
    }
}

TEST(LayerTests, StructuralLayer_Reframe_Smaller)
{
    //
    // Create source layer
    //

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceBuffer(8, 6);

    std::vector<StructuralMaterial> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2)));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(&(materials.back()));
        }
    }

    StructuralLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    StructuralLayerData targetLayer = sourceLayer.MakeReframed(
        { 4, 3 },
        { -2, -1 },
        StructuralElement(nullptr));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(4, 3));

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            if (x >= 4 || y >= 3)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], StructuralElement(nullptr));
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ShipSpaceSize(2, 1)]);
            }
        }
    }
}

TEST(LayerTests, StructuralLayer_Reframe_Larger)
{
    //
    // Create source layer
    //

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceBuffer(4, 4);

    std::vector<StructuralMaterial> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2)));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(&(materials.back()));
        }
    }

    StructuralLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    StructuralLayerData targetLayer = sourceLayer.MakeReframed(
        { 8, 6 },
        { 1, 2 },
        StructuralElement(nullptr));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(8, 6));

    for (int y = 0; y < 6; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            if (x >= 1 && x < 1 + 4 && y >= 2 && y < 2 + 4)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords - ShipSpaceSize(1, 2)]);
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], StructuralElement(nullptr));
            }
        }
    }
}

TEST(LayerTests, StructuralLayer_Reframe_Same)
{
    //
    // Create source layer
    //

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceBuffer(8, 8);

    std::vector<StructuralMaterial> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2)));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(&(materials.back()));
        }
    }

    StructuralLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    StructuralLayerData targetLayer = sourceLayer.MakeReframed(
        sourceLayer.Buffer.Size,
        { 0, 0 },
        StructuralElement(nullptr));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, sourceLayer.Buffer.Size);

    for (int y = 0; y < sourceLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceLayer.Buffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords]);
        }
    }
}

TEST(LayerTests, ElectricalLayer_Clone_Smaller)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 6);
    ElectricalPanelMetadata sourcePanel;

    std::vector<ElectricalMaterial> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 0;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx = curIdx++;

            sourcePanel.try_emplace(
                idx,
                IntegralCoordinates(idx + 5, idx + 7),
                "Foo",
                false);

            materials.emplace_back(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(&(materials.back()), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.size(), sourceBuffer.Size.height * sourceBuffer.Size.width);

    //
    // Clone layer
    //

    ElectricalLayerData targetLayer = sourceLayer.Clone({ {2, 1}, {4, 3} });

    //
    // Verify
    //

    // Buffer

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(4, 3));
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ShipSpaceSize(2, 1)]);
        }
    }

    // Panel

    EXPECT_EQ(targetLayer.Panel.size(), targetLayer.Buffer.Size.height * targetLayer.Buffer.Size.width);

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex const actualTargetIdx = targetLayer.Buffer[coords].InstanceIndex;
            ElectricalElementInstanceIndex const expectedTargetIdx = static_cast<ElectricalElementInstanceIndex>(1 * sourceBuffer.Size.width + (y * sourceBuffer.Size.width) + 2 + x);
            EXPECT_EQ(actualTargetIdx, expectedTargetIdx);

            auto const it = targetLayer.Panel.find(expectedTargetIdx);
            ASSERT_NE(it, targetLayer.Panel.end());
            EXPECT_EQ(it->second.PanelCoordinates, IntegralCoordinates(expectedTargetIdx + 5, expectedTargetIdx + 7));
        }
    }
}

TEST(LayerTests, ElectricalLayer_Trim)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 6);
    ElectricalPanelMetadata sourcePanel;

    std::vector<ElectricalMaterial> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 0;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx = curIdx++;

            sourcePanel.try_emplace(
                idx,
                IntegralCoordinates(idx + 5, idx + 7),
                "Foo",
                false);

            materials.emplace_back(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(&(materials.back()), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.size(), sourceBuffer.Size.height * sourceBuffer.Size.width);

    //
    // Trim layer
    //

    ElectricalLayerData targetLayer = sourceLayer.Clone();
    targetLayer.Trim({ {2, 1}, { 4, 3 } });

    //
    // Verify
    //

    // Buffer

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(4, 3));
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ShipSpaceSize(2, 1)]);
        }
    }

    // Panel

    EXPECT_EQ(targetLayer.Panel.size(), targetLayer.Buffer.Size.height * targetLayer.Buffer.Size.width);

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex const actualTargetIdx = targetLayer.Buffer[coords].InstanceIndex;
            ElectricalElementInstanceIndex const expectedTargetIdx = static_cast<ElectricalElementInstanceIndex>(1 * sourceBuffer.Size.width + (y * sourceBuffer.Size.width) + 2 + x);
            EXPECT_EQ(actualTargetIdx, expectedTargetIdx);

            auto const it = targetLayer.Panel.find(expectedTargetIdx);
            ASSERT_NE(it, targetLayer.Panel.end());
            EXPECT_EQ(it->second.PanelCoordinates, IntegralCoordinates(expectedTargetIdx + 5, expectedTargetIdx + 7));
        }
    }
}

TEST(LayerTests, ElectricalLayer_Reframe_Smaller)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 6);
    ElectricalPanelMetadata sourcePanel;

    std::vector<ElectricalMaterial> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 1;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx;
            if (coords == ShipSpaceCoordinates{ 1, 1 } // Out, Idx=1
                || coords == ShipSpaceCoordinates{ 2, 1 } // In, Idx=2
                || coords == ShipSpaceCoordinates{ 4, 2 } // In, Idx=3
                || coords == ShipSpaceCoordinates{ 7, 4 }) // Out, Idx=4
            {
                idx = curIdx++;

                sourcePanel.try_emplace(
                    idx,
                    IntegralCoordinates(idx + 5, idx + 7),
                    "Foo",
                    false);
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(&(materials.back()), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.size(), 4u);

    //
    // Reframe layer
    //

    ElectricalLayerData targetLayer = sourceLayer.MakeReframed(
        { 4, 3 },
        { -2, -1 },
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));

    //
    // Verify
    //

    // Buffer

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(4, 3));
    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            // Buffer
            if (x >= 4 || y >= 3)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ShipSpaceSize(2, 1)]);
            }
        }
    }

    // Panel

    EXPECT_EQ(targetLayer.Panel.size(), 2u);

    auto it = targetLayer.Panel.find(1);
    EXPECT_EQ(it, targetLayer.Panel.end());

    it = targetLayer.Panel.find(2);
    ASSERT_NE(it, targetLayer.Panel.end());
    ASSERT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(*(it->second.PanelCoordinates), IntegralCoordinates(7, 9));

    it = targetLayer.Panel.find(3);
    ASSERT_NE(it, targetLayer.Panel.end());
    ASSERT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(*(it->second.PanelCoordinates), IntegralCoordinates(8, 10));

    it = targetLayer.Panel.find(4);
    EXPECT_EQ(it, targetLayer.Panel.end());
}

TEST(LayerTests, ElectricalLayer_Reframe_Larger)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(4, 4);
    ElectricalPanelMetadata sourcePanel;

    std::vector<ElectricalMaterial> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 1;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx;
            if (coords == ShipSpaceCoordinates{ 1, 1 } // In
                || coords == ShipSpaceCoordinates{ 2, 1 } // In
                || coords == ShipSpaceCoordinates{ 3, 3 }) // In
            {
                idx = curIdx++;

                sourcePanel.try_emplace(
                    idx,
                    IntegralCoordinates(idx + 5, idx + 7),
                    "Foo",
                    false);
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(&(materials.back()), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.size(), 3u);

    //
    // Reframe layer
    //

    ElectricalLayerData targetLayer = sourceLayer.MakeReframed(
        { 8, 6 },
        { 1, 2 },
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));

    //
    // Verify
    //

    // Buffer

    ASSERT_EQ(targetLayer.Buffer.Size, ShipSpaceSize(8, 6));
    for (int y = 0; y < 6; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            // Buffer
            if (x >= 1 && x < 1 + 4 && y >= 2 && y < 2 + 4)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords - ShipSpaceSize(1, 2)]);
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));
            }
        }
    }

    // Panel

    EXPECT_EQ(targetLayer.Panel.size(), 3u);
}

TEST(LayerTests, ElectricalLayer_Reframe_Same)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 8);
    ElectricalPanelMetadata sourcePanel;

    std::vector<ElectricalMaterial> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 1;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx;
            if (coords == ShipSpaceCoordinates{ 1, 1 } // Out
                || coords == ShipSpaceCoordinates{ 2, 1 } // In
                || coords == ShipSpaceCoordinates{ 4, 2 } // In
                || coords == ShipSpaceCoordinates{ 6, 4 }) // Out
            {
                idx = curIdx++;

                sourcePanel.try_emplace(
                    idx,
                    IntegralCoordinates(idx + 5, idx + 7),
                    "Foo",
                    false);
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(&(materials.back()), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.size(), 4u);

    //
    // Reframe layer
    //

    ElectricalLayerData targetLayer = sourceLayer.MakeReframed(
        sourceLayer.Buffer.Size,
        { 0, 0 },
        ElectricalElement(nullptr, NoneElectricalElementInstanceIndex));

    //
    // Verify
    //

    // Buffer

    ASSERT_EQ(targetLayer.Buffer.Size, sourceLayer.Buffer.Size);

    for (int y = 0; y < sourceLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceLayer.Buffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords]);
        }
    }

    // Panel

    EXPECT_EQ(targetLayer.Panel.size(), 4u);
}

TEST(LayerTests, RopesLayer_Trim)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // Out
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(2, 4), // Out
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopesLayerData sourceLayer(std::move(buffer));

    //
    // Trim layer
    //

    RopesLayerData targetLayer = sourceLayer.Clone();

    targetLayer.Trim({ {3, 3}, {8, 9} });

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.GetSize(), 1);

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(1, 2));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(7, 7));
}

TEST(LayerTests, RopesLayer_Reframe_Smaller)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // Out
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(2, 4), // Out
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopesLayerData sourceLayer(std::move(buffer));

    //
    // Reframe layer
    //

    RopesLayerData targetLayer = sourceLayer.MakeReframed(
        { 8, 9 },
        { -3, -3 });

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.GetSize(), 1);

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(1, 2));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(7, 7));
}

TEST(LayerTests, RopesLayer_Reframe_Larger)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(2, 4), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopesLayerData sourceLayer(std::move(buffer));

    //
    // Reframe layer
    //

    RopesLayerData targetLayer = sourceLayer.MakeReframed(
        { 20, 20 },
        { 4, 4 });

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.GetSize(), 3);

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(8, 9));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(14, 14));

    EXPECT_EQ(targetLayer.Buffer[1].StartCoords, ShipSpaceCoordinates(8, 9));
    EXPECT_EQ(targetLayer.Buffer[1].EndCoords, ShipSpaceCoordinates(15, 15));

    EXPECT_EQ(targetLayer.Buffer[2].StartCoords, ShipSpaceCoordinates(6, 8));
    EXPECT_EQ(targetLayer.Buffer[2].EndCoords, ShipSpaceCoordinates(14, 14));
}

TEST(LayerTests, RopesLayer_Reframe_Same)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer;

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(11, 11),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(2, 4), // In
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopesLayerData sourceLayer(std::move(buffer));

    //
    // Reframe layer
    //

    RopesLayerData targetLayer = sourceLayer.MakeReframed(
        { 12, 12 },
        { 0, 0 });

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.GetSize(), 3);

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(4, 5));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(10, 10));

    EXPECT_EQ(targetLayer.Buffer[1].StartCoords, ShipSpaceCoordinates(4, 5));
    EXPECT_EQ(targetLayer.Buffer[1].EndCoords, ShipSpaceCoordinates(11, 11));

    EXPECT_EQ(targetLayer.Buffer[2].StartCoords, ShipSpaceCoordinates(2, 4));
    EXPECT_EQ(targetLayer.Buffer[2].EndCoords, ShipSpaceCoordinates(10, 10));
}

TEST(LayerTests, TextureLayer_Reframe_Smaller)
{
    //
    // Create source layer
    //

    Buffer2D<rgbaColor, struct ImageTag> sourceBuffer(8, 6);

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[ImageCoordinates(x, y)] = rgbaColor(iVal, iVal, iVal, iVal);
        }
    }

    TextureLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    TextureLayerData targetLayer = sourceLayer.MakeReframed(
        { 4, 3 },
        { -2, -1 },
        rgbaColor(0, 0, 0, 255));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, ImageSize(4, 3));

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ImageCoordinates(x, y);
            if (x >= 4 || y >= 3)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], rgbaColor(0, 0, 0, 255));
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords + ImageSize(2, 1)]);
            }
        }
    }
}

TEST(LayerTests, TextureLayer_Reframe_Larger)
{
    //
    // Create source layer
    //

    Buffer2D<rgbaColor, struct ImageTag> sourceBuffer(4, 4);

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[ImageCoordinates(x, y)] = rgbaColor(iVal, iVal, iVal, iVal);
        }
    }

    TextureLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    TextureLayerData targetLayer = sourceLayer.MakeReframed(
        { 8, 6 },
        { 1, 2 },
        rgbaColor(0, 0, 0, 255));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, ImageSize(8, 6));

    for (int y = 0; y < 6; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            auto const coords = ImageCoordinates(x, y);
            if (x >= 1 && x < 1 + 4 && y >= 2 && y < 2 + 4)
            {
                EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords - ImageSize(1, 2)]);
            }
            else
            {
                EXPECT_EQ(targetLayer.Buffer[coords], rgbaColor(0, 0, 0, 255));
            }
        }
    }
}

TEST(LayerTests, TextureLayer_Reframe_Same)
{
    //
    // Create source layer
    //

    Buffer2D<rgbaColor, struct ImageTag> sourceBuffer(8, 8);

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            sourceBuffer[ImageCoordinates(x, y)] = rgbaColor(iVal, iVal, iVal, iVal);
        }
    }

    TextureLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Reframe layer
    //

    TextureLayerData targetLayer = sourceLayer.MakeReframed(
        sourceLayer.Buffer.Size,
        { 0, 0 },
        rgbaColor(0, 0, 0, 255));

    //
    // Verify
    //

    ASSERT_EQ(targetLayer.Buffer.Size, sourceLayer.Buffer.Size);

    for (int y = 0; y < sourceLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceLayer.Buffer.Size.width; ++x)
        {
            auto const coords = ImageCoordinates(x, y);
            EXPECT_EQ(targetLayer.Buffer[coords], sourceLayer.Buffer[coords]);
        }
    }
}