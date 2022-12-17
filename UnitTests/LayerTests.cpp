#include <Game/Layers.h>

#include "Utils.h"

#include "gtest/gtest.h"

TEST(LayerTests, StructuralLayer_Trim)
{
    //
    // Create source layer
    //

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceBuffer(8, 6);

    std::vector<std::unique_ptr<StructuralMaterial>> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2))));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(materials.back().get());
        }
    }

    StructuralLayerData sourceLayer(std::move(sourceBuffer));

    //
    // Trim layer
    //

    StructuralLayerData targetLayer = sourceLayer.Clone();

    targetLayer.Trim({
        ShipSpaceCoordinates(2, 1),
        ShipSpaceSize(4, 3) });

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

    std::vector<std::unique_ptr<StructuralMaterial>> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2))));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(materials.back().get());
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

    std::vector<std::unique_ptr<StructuralMaterial>> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2))));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(materials.back().get());
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

    std::vector<std::unique_ptr<StructuralMaterial>> materials;

    uint8_t iVal = 2;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            materials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2))));
            sourceBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(materials.back().get());
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

TEST(LayerTests, ElectricalLayer_CloneRegion_Smaller)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 6);
    ElectricalPanel sourcePanel;

    std::vector<std::unique_ptr<ElectricalMaterial>> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 0;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx = curIdx++;

            sourcePanel.Add(
                idx,
                ElectricalPanel::ElementMetadata(
                    IntegralCoordinates(idx + 5, idx + 7),
                    "Foo",
                    false));

            materials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex)));

            sourceBuffer[coords] = ElectricalElement(materials.back().get(), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.GetSize(), static_cast<size_t>(sourceBuffer.Size.height * sourceBuffer.Size.width));

    //
    // Clone layer
    //

    ElectricalLayerData targetLayer = sourceLayer.CloneRegion({
        ShipSpaceCoordinates(2, 1),
        ShipSpaceSize(4, 3) });

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

    EXPECT_EQ(targetLayer.Panel.GetSize(), static_cast<size_t>(targetLayer.Buffer.Size.height * targetLayer.Buffer.Size.width));

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex const actualTargetIdx = targetLayer.Buffer[coords].InstanceIndex;
            ElectricalElementInstanceIndex const expectedTargetIdx = static_cast<ElectricalElementInstanceIndex>(1 * sourceBuffer.Size.width + (y * sourceBuffer.Size.width) + 2 + x);
            EXPECT_EQ(actualTargetIdx, expectedTargetIdx);

            auto const it = targetLayer.Panel.Find(expectedTargetIdx);
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
    ElectricalPanel sourcePanel;

    std::vector<std::unique_ptr<ElectricalMaterial>> materials;

    uint8_t iVal = 2;
    ElectricalElementInstanceIndex curIdx = 0;
    for (int y = 0; y < sourceBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceBuffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex idx = curIdx++;

            sourcePanel.Add(
                idx,
                ElectricalPanel::ElementMetadata(
                    IntegralCoordinates(idx + 5, idx + 7),
                    "Foo",
                    false));

            materials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex)));

            sourceBuffer[coords] = ElectricalElement(materials.back().get(), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.GetSize(), static_cast<size_t>(sourceBuffer.Size.height * sourceBuffer.Size.width));

    //
    // Trim layer
    //

    ElectricalLayerData targetLayer = sourceLayer.Clone();
    targetLayer.Trim({
        ShipSpaceCoordinates(2, 1),
        ShipSpaceSize(4, 3) });

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

    EXPECT_EQ(targetLayer.Panel.GetSize(), static_cast<size_t>(targetLayer.Buffer.Size.height * targetLayer.Buffer.Size.width));

    for (int y = 0; y < 3; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);

            ElectricalElementInstanceIndex const actualTargetIdx = targetLayer.Buffer[coords].InstanceIndex;
            ElectricalElementInstanceIndex const expectedTargetIdx = static_cast<ElectricalElementInstanceIndex>(1 * sourceBuffer.Size.width + (y * sourceBuffer.Size.width) + 2 + x);
            EXPECT_EQ(actualTargetIdx, expectedTargetIdx);

            auto const it = targetLayer.Panel.Find(expectedTargetIdx);
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
    ElectricalPanel sourcePanel;

    std::vector<std::unique_ptr<ElectricalMaterial>> materials;

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

                sourcePanel.Add(
                    idx,
                    ElectricalPanel::ElementMetadata(
                        IntegralCoordinates(idx + 5, idx + 7),
                        "Foo",
                        false));
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex)));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(materials.back().get(), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.GetSize(), 4u);

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

    EXPECT_EQ(targetLayer.Panel.GetSize(), 2u);

    auto it = targetLayer.Panel.Find(1);
    EXPECT_EQ(it, targetLayer.Panel.end());

    it = targetLayer.Panel.Find(2);
    ASSERT_NE(it, targetLayer.Panel.end());
    ASSERT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(*(it->second.PanelCoordinates), IntegralCoordinates(7, 9));

    it = targetLayer.Panel.Find(3);
    ASSERT_NE(it, targetLayer.Panel.end());
    ASSERT_TRUE(it->second.PanelCoordinates.has_value());
    EXPECT_EQ(*(it->second.PanelCoordinates), IntegralCoordinates(8, 10));

    it = targetLayer.Panel.Find(4);
    EXPECT_EQ(it, targetLayer.Panel.end());
}

TEST(LayerTests, ElectricalLayer_Reframe_Larger)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(4, 4);
    ElectricalPanel sourcePanel;

    std::vector<std::unique_ptr<ElectricalMaterial>> materials;

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

                sourcePanel.Add(
                    idx,
                    ElectricalPanel::ElementMetadata(
                        IntegralCoordinates(idx + 5, idx + 7),
                        "Foo",
                        false));
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex)));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(materials.back().get(), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.GetSize(), 3u);

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

    EXPECT_EQ(targetLayer.Panel.GetSize(), 3u);
}

TEST(LayerTests, ElectricalLayer_Reframe_Same)
{
    //
    // Create source layer
    //

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceBuffer(8, 8);
    ElectricalPanel sourcePanel;

    std::vector<std::unique_ptr<ElectricalMaterial>> materials;

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

                sourcePanel.Add(
                    idx,
                    ElectricalPanel::ElementMetadata(
                        IntegralCoordinates(idx + 5, idx + 7),
                        "Foo",
                        false));
            }
            else
            {
                idx = NoneElectricalElementInstanceIndex;
            }

            materials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal + 1, iVal + 2), idx != NoneElectricalElementInstanceIndex)));

            sourceBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(materials.back().get(), idx);
        }
    }

    ElectricalLayerData sourceLayer(std::move(sourceBuffer), std::move(sourcePanel));

    ASSERT_EQ(sourceLayer.Panel.GetSize(), 4u);

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

    EXPECT_EQ(targetLayer.Panel.GetSize(), 4u);
}

TEST(LayerTests, RopesLayer_Trim)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer({400, 200});

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

    targetLayer.Trim({
        ShipSpaceCoordinates(3, 3),
        ShipSpaceSize(8, 9) });

    //
    // Verify
    //

    EXPECT_EQ(ShipSpaceSize(8, 9), targetLayer.Buffer.GetSize());
    ASSERT_EQ(1u, targetLayer.Buffer.GetElementCount());

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(1, 2));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(7, 7));
}

TEST(LayerTests, RopesLayer_CloneRegion_Smaller)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer({ 400, 200 });

    buffer.EmplaceBack(
        ShipSpaceCoordinates(4, 5), // In
        ShipSpaceCoordinates(5, 6),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(0, 1), // Out
        ShipSpaceCoordinates(5, 6),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    buffer.EmplaceBack(
        ShipSpaceCoordinates(2, 4), // Out
        ShipSpaceCoordinates(10, 10),
        nullptr,
        rgbaColor(1, 2, 3, 4));

    RopesLayerData sourceLayer(std::move(buffer));

    //
    // Clone layer
    //

    RopesLayerData targetLayer = sourceLayer.CloneRegion(
        ShipSpaceRect(
            ShipSpaceCoordinates(1, 1),
            ShipSpaceSize(8, 9)));

    //
    // Verify
    //

    EXPECT_EQ(ShipSpaceSize(8, 9), targetLayer.Buffer.GetSize());
    ASSERT_EQ(1u, targetLayer.Buffer.GetElementCount());

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(3, 4));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(4, 5));
}

TEST(LayerTests, RopesLayer_Reframe_Smaller)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer({ 400, 200 });

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

    EXPECT_EQ(ShipSpaceSize(8, 9), targetLayer.Buffer.GetSize());
    ASSERT_EQ(1u, targetLayer.Buffer.GetElementCount());

    EXPECT_EQ(targetLayer.Buffer[0].StartCoords, ShipSpaceCoordinates(1, 2));
    EXPECT_EQ(targetLayer.Buffer[0].EndCoords, ShipSpaceCoordinates(7, 7));
}

TEST(LayerTests, RopesLayer_Reframe_Larger)
{
    //
    // Create source layer
    //
    // Size= (12, 12)

    RopeBuffer buffer({ 4, 2 });

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

    EXPECT_EQ(ShipSpaceSize(20, 20), targetLayer.Buffer.GetSize());
    ASSERT_EQ(3u, targetLayer.Buffer.GetElementCount());

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

    RopeBuffer buffer({ 12, 12 });

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

    EXPECT_EQ(ShipSpaceSize(12, 12), targetLayer.Buffer.GetSize());
    ASSERT_EQ(3u, targetLayer.Buffer.GetElementCount());

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

TEST(LayerTests, ShipLayers_FlipH)
{
    std::vector<std::unique_ptr<StructuralMaterial>> structuralMaterials;
    std::vector<std::unique_ptr<ElectricalMaterial>> electricalMaterials;

    //
    // Create source
    //

    ShipSpaceSize const shipSize(8, 6);

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceStructuralLayerBuffer(shipSize);
    uint8_t iVal = 0;
    for (int y = 0; y < sourceStructuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceStructuralLayerBuffer.Size.width; ++x)
        {
            structuralMaterials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal, iVal))));
            sourceStructuralLayerBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(structuralMaterials.back().get());

            ++iVal;
        }
    }

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceElectricalLayerBuffer(shipSize);
    iVal = 0;
    for (int y = 0; y < sourceElectricalLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceElectricalLayerBuffer.Size.width; ++x)
        {
            electricalMaterials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal, iVal), false)));
            sourceElectricalLayerBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(electricalMaterials.back().get(), NoneElectricalElementInstanceIndex);

            ++iVal;
        }
    }

    ElectricalPanel sourcePanel;

    RopeBuffer sourceRopesLayerBuffer(shipSize);

    {
        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(5, 5),
            ShipSpaceCoordinates(2, 3),
            nullptr,
            rgbaColor(1, 2, 3, 4));

        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(1, 1),
            ShipSpaceCoordinates(2, 2),
            nullptr,
            rgbaColor(1, 2, 3, 4));
    }

    Buffer2D<rgbaColor, struct ImageTag> sourceTextureLayerBuffer(80, 60);

    iVal = 0;
    for (int y = 0; y < sourceTextureLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceTextureLayerBuffer.Size.width; ++x)
        {
            sourceTextureLayerBuffer[ImageCoordinates(x, y)] = rgbaColor(iVal, iVal, iVal, iVal);

            ++iVal;
        }
    }

    ShipLayers layers(
        shipSize,
        std::make_unique<StructuralLayerData>(std::move(sourceStructuralLayerBuffer)),
        std::make_unique<ElectricalLayerData>(std::move(sourceElectricalLayerBuffer), std::move(sourcePanel)),
        std::make_unique<RopesLayerData>(std::move(sourceRopesLayerBuffer)),
        std::make_unique<TextureLayerData>(std::move(sourceTextureLayerBuffer)));

    //
    // Flip
    //

    layers.Flip(DirectionType::Horizontal);

    //
    // Verify
    //

    ASSERT_EQ(layers.Size, shipSize);

    ASSERT_EQ(layers.StructuralLayer->Buffer.Size, shipSize);

    iVal = 0;
    for (int y = 0; y < layers.StructuralLayer->Buffer.Size.height; ++y)
    {
        for (int x = layers.StructuralLayer->Buffer.Size.width - 1; x >= 0; --x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(layers.StructuralLayer->Buffer[coords].Material->ColorKey, rgbColor(iVal, iVal, iVal));

            ++iVal;
        }
    }

    ASSERT_TRUE(layers.ElectricalLayer);
    ASSERT_EQ(layers.ElectricalLayer->Buffer.Size, shipSize);

    iVal = 0;
    for (int y = 0; y < layers.ElectricalLayer->Buffer.Size.height; ++y)
    {
        for (int x = layers.ElectricalLayer->Buffer.Size.width - 1; x >= 0; --x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(layers.ElectricalLayer->Buffer[coords].Material->ColorKey, rgbColor(iVal, iVal, iVal));

            ++iVal;
        }
    }

    ASSERT_TRUE(layers.RopesLayer);
    ASSERT_EQ(layers.RopesLayer->Buffer.GetSize(), shipSize);
    ASSERT_EQ(layers.RopesLayer->Buffer.GetElementCount(), 2u);

    EXPECT_EQ(layers.RopesLayer->Buffer[0].StartCoords, ShipSpaceCoordinates(2, 5));
    EXPECT_EQ(layers.RopesLayer->Buffer[0].EndCoords, ShipSpaceCoordinates(5, 3));

    EXPECT_EQ(layers.RopesLayer->Buffer[1].StartCoords, ShipSpaceCoordinates(6, 1));
    EXPECT_EQ(layers.RopesLayer->Buffer[1].EndCoords, ShipSpaceCoordinates(5, 2));


    ASSERT_TRUE(layers.TextureLayer);
    ASSERT_EQ(layers.TextureLayer->Buffer.Size, ImageSize(80, 60));

    iVal = 0;
    for (int y = 0; y < layers.TextureLayer->Buffer.Size.height; ++y)
    {
        for (int x = layers.TextureLayer->Buffer.Size.width - 1; x >= 0; --x)
        {
            auto const coords = ImageCoordinates(x, y);
            EXPECT_EQ(layers.TextureLayer->Buffer[coords], rgbaColor(iVal, iVal, iVal, iVal));

            ++iVal;
        }
    }
}

TEST(LayerTests, ShipLayers_Rotate)
{
    std::vector<std::unique_ptr<StructuralMaterial>> structuralMaterials;
    std::vector<std::unique_ptr<ElectricalMaterial>> electricalMaterials;

    //
    // Create source
    //

    ShipSpaceSize const shipSize(8, 6);

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceStructuralLayerBuffer(shipSize);
    uint8_t iVal = 0;
    for (int y = 0; y < sourceStructuralLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceStructuralLayerBuffer.Size.width; ++x)
        {
            structuralMaterials.emplace_back(new StructuralMaterial(MakeTestStructuralMaterial("Foo", rgbColor(iVal, iVal, iVal))));
            sourceStructuralLayerBuffer[ShipSpaceCoordinates(x, y)] = StructuralElement(structuralMaterials.back().get());

            ++iVal;
        }
    }

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceElectricalLayerBuffer(shipSize);
    iVal = 0;
    for (int y = 0; y < sourceElectricalLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceElectricalLayerBuffer.Size.width; ++x)
        {
            electricalMaterials.emplace_back(new ElectricalMaterial(MakeTestElectricalMaterial("Foo", rgbColor(iVal, iVal, iVal), false)));
            sourceElectricalLayerBuffer[ShipSpaceCoordinates(x, y)] = ElectricalElement(electricalMaterials.back().get(), NoneElectricalElementInstanceIndex);

            ++iVal;
        }
    }

    ElectricalPanel sourcePanel;

    RopeBuffer sourceRopesLayerBuffer(shipSize);

    {
        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(5, 5),
            ShipSpaceCoordinates(2, 3),
            nullptr,
            rgbaColor(1, 2, 3, 4));

        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(1, 1),
            ShipSpaceCoordinates(2, 2),
            nullptr,
            rgbaColor(1, 2, 3, 4));
    }

    Buffer2D<rgbaColor, struct ImageTag> sourceTextureLayerBuffer(80, 60);

    iVal = 0;
    for (int y = 0; y < sourceTextureLayerBuffer.Size.height; ++y)
    {
        for (int x = 0; x < sourceTextureLayerBuffer.Size.width; ++x)
        {
            sourceTextureLayerBuffer[ImageCoordinates(x, y)] = rgbaColor(iVal, iVal, iVal, iVal);

            ++iVal;
        }
    }

    ShipLayers layers(
        shipSize,
        std::make_unique<StructuralLayerData>(std::move(sourceStructuralLayerBuffer)),
        std::make_unique<ElectricalLayerData>(std::move(sourceElectricalLayerBuffer), std::move(sourcePanel)),
        std::make_unique<RopesLayerData>(std::move(sourceRopesLayerBuffer)),
        std::make_unique<TextureLayerData>(std::move(sourceTextureLayerBuffer)));

    //
    // Flip
    //

    layers.Rotate90(RotationDirectionType::Clockwise);

    //
    // Verify
    //

    ASSERT_EQ(layers.Size, ShipSpaceSize(6, 8));

    ASSERT_EQ(layers.StructuralLayer->Buffer.Size, ShipSpaceSize(6, 8));

    iVal = 0;
    for (int x = 0; x < layers.StructuralLayer->Buffer.Size.width; ++x)
    {
        for (int y = layers.StructuralLayer->Buffer.Size.height - 1; y >= 0; --y)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(layers.StructuralLayer->Buffer[coords].Material->ColorKey, rgbColor(iVal, iVal, iVal));

            ++iVal;
        }
    }

    ASSERT_TRUE(layers.ElectricalLayer);
    ASSERT_EQ(layers.ElectricalLayer->Buffer.Size, ShipSpaceSize(6, 8));

    iVal = 0;
    for (int x = 0; x < layers.ElectricalLayer->Buffer.Size.width; ++x)
    {
        for (int y = layers.ElectricalLayer->Buffer.Size.height - 1; y >= 0; --y)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            EXPECT_EQ(layers.ElectricalLayer->Buffer[coords].Material->ColorKey, rgbColor(iVal, iVal, iVal));

            ++iVal;
        }
    }

    ASSERT_TRUE(layers.RopesLayer);
    ASSERT_EQ(layers.RopesLayer->Buffer.GetSize(), ShipSpaceSize(6, 8));
    ASSERT_EQ(layers.RopesLayer->Buffer.GetElementCount(), 2u);

    EXPECT_EQ(layers.RopesLayer->Buffer[0].StartCoords, ShipSpaceCoordinates(5, 2));
    EXPECT_EQ(layers.RopesLayer->Buffer[0].EndCoords, ShipSpaceCoordinates(3, 5));

    EXPECT_EQ(layers.RopesLayer->Buffer[1].StartCoords, ShipSpaceCoordinates(1, 6));
    EXPECT_EQ(layers.RopesLayer->Buffer[1].EndCoords, ShipSpaceCoordinates(2, 5));


    ASSERT_TRUE(layers.TextureLayer);
    ASSERT_EQ(layers.TextureLayer->Buffer.Size, ImageSize(60, 80));

    iVal = 0;
    for (int x = 0; x < layers.TextureLayer->Buffer.Size.width; ++x)
    {
        for (int y = layers.TextureLayer->Buffer.Size.height - 1; y >= 0; --y)
        {
            auto const coords = ImageCoordinates(x, y);
            EXPECT_EQ(layers.TextureLayer->Buffer[coords], rgbaColor(iVal, iVal, iVal, iVal));

            ++iVal;
        }
    }
}

TEST(LayerTests, ShipLayers_Clone_Full)
{
    //
    // Create source
    //

    ShipSpaceSize const shipSize(8, 6);

    Buffer2D<StructuralElement, struct ShipSpaceTag> sourceStructuralLayerBuffer(shipSize);

    Buffer2D<ElectricalElement, struct ShipSpaceTag> sourceElectricalLayerBuffer(shipSize);

    ElectricalPanel sourcePanel;
    sourcePanel.Add(
        ElectricalElementInstanceIndex(1),
        ElectricalPanel::ElementMetadata(std::nullopt, std::nullopt, true));

    RopeBuffer sourceRopesLayerBuffer(shipSize);

    {
        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(5, 5),
            ShipSpaceCoordinates(2, 3),
            nullptr,
            rgbaColor(1, 2, 3, 4));

        sourceRopesLayerBuffer.EmplaceBack(
            ShipSpaceCoordinates(1, 1),
            ShipSpaceCoordinates(2, 2),
            nullptr,
            rgbaColor(1, 2, 3, 4));
    }

    Buffer2D<rgbaColor, struct ImageTag> sourceTextureLayerBuffer(80, 60);

    ShipLayers layers(
        shipSize,
        std::make_unique<StructuralLayerData>(std::move(sourceStructuralLayerBuffer)),
        std::make_unique<ElectricalLayerData>(std::move(sourceElectricalLayerBuffer), std::move(sourcePanel)),
        std::make_unique<RopesLayerData>(std::move(sourceRopesLayerBuffer)),
        std::make_unique<TextureLayerData>(std::move(sourceTextureLayerBuffer)));

    //
    // Clone
    //

    auto layerClone = layers.Clone();

    //
    // Verify
    //

    ASSERT_EQ(layerClone.Size, shipSize);

    ASSERT_TRUE(layerClone.StructuralLayer);
    ASSERT_EQ(layerClone.StructuralLayer->Buffer.Size, shipSize);

    ASSERT_TRUE(layerClone.ElectricalLayer);
    ASSERT_EQ(layerClone.ElectricalLayer->Buffer.Size, shipSize);
    ASSERT_EQ(layerClone.ElectricalLayer->Panel.GetSize(), 1u);

    ASSERT_TRUE(layerClone.RopesLayer);
    ASSERT_EQ(layerClone.RopesLayer->Buffer.GetSize(), shipSize);
    ASSERT_EQ(layerClone.RopesLayer->Buffer.GetElementCount(), 2u);

    ASSERT_TRUE(layerClone.TextureLayer);
    ASSERT_EQ(layerClone.TextureLayer->Buffer.Size, ImageSize(80, 60));
}

TEST(LayerTests, ShipLayers_Clone_Empty)
{
    //
    // Create source
    //

    ShipSpaceSize const shipSize(8, 6);

    ShipLayers layers(
        shipSize,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    //
    // Clone
    //

    auto layerClone = layers.Clone();

    //
    // Verify
    //

    ASSERT_EQ(layerClone.Size, shipSize);

    ASSERT_FALSE(layerClone.StructuralLayer);

    ASSERT_FALSE(layerClone.ElectricalLayer);

    ASSERT_FALSE(layerClone.RopesLayer);

    ASSERT_FALSE(layerClone.TextureLayer);
}
