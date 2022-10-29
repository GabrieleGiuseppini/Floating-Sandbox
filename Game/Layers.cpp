/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-03-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Layers.h"

StructuralLayerData StructuralLayerData::MakeReframed(
    ShipSpaceSize const & newSize, // Final size
    ShipSpaceCoordinates const & originOffset, // Position in final buffer of original {0, 0}
    StructuralElement const & fillerValue) const
{
    return StructuralLayerData(
        Buffer.MakeReframed(
            newSize,
            originOffset,
            fillerValue));
}

ElectricalLayerData ElectricalLayerData::MakeReframed(
    ShipSpaceSize const & newSize, // Final size
    ShipSpaceCoordinates const & originOffset, // Position in final buffer of original {0, 0}
    ElectricalElement const & fillerValue) const
{
    // Trim panel
    ElectricalPanelMetadata newPanel = MakedTrimmedPanel(
        Panel,
        ShipSpaceRect(
            -originOffset,
            newSize));

    // Trim buffer
    return ElectricalLayerData(
        Buffer.MakeReframed(
            newSize,
            originOffset,
            fillerValue),
        std::move(newPanel));
}

ElectricalPanelMetadata ElectricalLayerData::MakedTrimmedPanel(
    ElectricalPanelMetadata const & panel,
    ShipSpaceRect const & rect) const
{
    ElectricalPanelMetadata newPanel;

    // Visit all instanced elements
    for (int y = 0; y < Buffer.Size.height; ++y)
    {
        for (int x = 0; x < Buffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates({ x, y });

            auto const instanceIndex = Buffer[coords].InstanceIndex;
            if (instanceIndex != NoneElectricalElementInstanceIndex
                && coords.IsInRect(rect))
            {
                // This instanced element remains...
                // ...see if it has an entry in the panel
                auto searchIt = panel.find(instanceIndex);
                if (searchIt != panel.end())
                {
                    // Copy to new panel
                    auto const [_, isInserted] = newPanel.emplace(instanceIndex, searchIt->second);
                    assert(isInserted);
                    (void)isInserted;
                }
            }
        }
    }

    return newPanel;
}

RopesLayerData RopesLayerData::MakeReframed(
    ShipSpaceSize const & newSize, // Final size
    ShipSpaceCoordinates const & originOffset) const // Position in final buffer of original {0, 0}
{
    RopeBuffer newBuffer = Buffer;
    newBuffer.Reframe(
        newSize,
        originOffset);

    return RopesLayerData(std::move(newBuffer));
}

TextureLayerData TextureLayerData::MakeReframed(
    ImageSize const & newSize, // Final size
    ImageCoordinates const & originOffset, // Position in final buffer of original {0, 0}
    rgbaColor const & fillerValue) const
{
    return TextureLayerData(
        Buffer.MakeReframed(
            newSize,
            originOffset,
            fillerValue));
}

void ShipLayers::Flip(DirectionType direction)
{
    if (StructuralLayer)
    {
        StructuralLayer->Buffer.Flip(direction);
    }

    if (ElectricalLayer)
    {
        ElectricalLayer->Buffer.Flip(direction);
    }

    if (RopesLayer)
    {
        RopesLayer->Buffer.Flip(direction, Size);
    }

    if (TextureLayer)
    {
        TextureLayer->Buffer.Flip(direction);
    }
}

void ShipLayers::Rotate90(RotationDirectionType direction)
{
    if (StructuralLayer)
    {
        StructuralLayer->Buffer.Rotate90(direction);
    }

    if (ElectricalLayer)
    {
        ElectricalLayer->Buffer.Rotate90(direction);
    }

    if (RopesLayer)
    {
        RopesLayer->Buffer.Rotate90(direction, Size);
    }

    if (TextureLayer)
    {
        TextureLayer->Buffer.Rotate90(direction);
    }
}