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
    //
    // Trim panel
    //

    // Calculate "static" (remaining) rect - wrt old coordinates 

    auto const originalShipRect = ShipSpaceRect(
        ShipSpaceCoordinates(0, 0),
        Buffer.Size);

    std::optional<ShipSpaceRect> staticShipRect = originalShipRect.MakeIntersectionWith(
        ShipSpaceRect(
            -originOffset, 
            newSize));

    // Trim panel elements

    ElectricalPanelMetadata newPanel = Panel;

    if (staticShipRect.has_value())
    {
        for (int y = 0; y < Buffer.Size.height; ++y)
        {
            for (int x = 0; x < Buffer.Size.width; ++x)
            {
                auto const coords = ShipSpaceCoordinates({ x, y });

                auto const instanceIndex = Buffer[coords].InstanceIndex;
                if (instanceIndex != NoneElectricalElementInstanceIndex
                    && !coords.IsInRect(*staticShipRect))
                {
                    // This instanced element will be gone
                    auto searchIt = newPanel.find(instanceIndex);
                    if (searchIt != newPanel.end())
                    {
                        newPanel.erase(searchIt);
                    }
                }
            }
        }
    }
    else
    {
        // None of the current elements will survive
        newPanel.clear();
    }

    //
    // Trim buffer
    //

    return ElectricalLayerData(
        Buffer.MakeReframed(
            newSize,
            originOffset,
            fillerValue),
        std::move(newPanel));
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