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

void StructuralLayerData::ReplaceMaterials(std::map<StructuralMaterial const *, StructuralMaterial const *> const & replacementMap)
{
    StructuralElement * bufferData = Buffer.Data.get();
    for (size_t i = 0; i < Buffer.GetLinearSize(); ++i)
    {
        auto const & it = replacementMap.find(bufferData[i].Material);
        if (it != replacementMap.end())
        {
            bufferData[i].Material = it->second;
        }
    }
}

ElectricalLayerData ElectricalLayerData::MakeReframed(
    ShipSpaceSize const & newSize, // Final size
    ShipSpaceCoordinates const & originOffset, // Position in final buffer of original {0, 0}
    ElectricalElement const & fillerValue) const
{
    // Trim panel
    ElectricalPanel newPanel = MakeTrimmedPanel(
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

ElectricalPanel ElectricalLayerData::MakeTrimmedPanel(
    ElectricalPanel const & panel,
    ShipSpaceRect const & rect) const
{
    ElectricalPanel newPanel;

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
                auto searchIt = panel.Find(instanceIndex);
                if (searchIt != panel.end())
                {
                    // Copy to new panel
                    newPanel.Add(instanceIndex, searchIt->second);
                }
            }
        }
    }

    return newPanel;
}

void ElectricalLayerData::ReplaceMaterials(std::map<ElectricalMaterial const *, ElectricalMaterial const *> const & replacementMap)
{
    ElectricalElement * bufferData = Buffer.Data.get();
    for (size_t i = 0; i < Buffer.GetLinearSize(); ++i)
    {
        auto const & it = replacementMap.find(bufferData[i].Material);
        if (it != replacementMap.end())
        {
            bufferData[i].Material = it->second;
        }
    }
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

void RopesLayerData::ReplaceMaterials(std::map<StructuralMaterial const *, StructuralMaterial const *> const & replacementMap)
{
    for (size_t i = 0; i < Buffer.GetElementCount(); ++i)
    {
        auto const & it = replacementMap.find(Buffer[i].Material);
        if (it != replacementMap.end())
        {
            Buffer[i].Material = it->second;
        }
    }
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
        RopesLayer->Buffer.Flip(direction);
    }

    if (ExteriorTextureLayer)
    {
        ExteriorTextureLayer->Buffer.Flip(direction);
    }

    if (InteriorTextureLayer)
    {
        InteriorTextureLayer->Buffer.Flip(direction);
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
        RopesLayer->Buffer.Rotate90(direction);
    }

    if (ExteriorTextureLayer)
    {
        ExteriorTextureLayer->Buffer.Rotate90(direction);
    }

    if (InteriorTextureLayer)
    {
        InteriorTextureLayer->Buffer.Rotate90(direction);
    }

    Size = ShipSpaceSize(Size.height, Size.width);
}