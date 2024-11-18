/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SamplerTool.h"

#include "../Controller.h"

#include <type_traits>

namespace ShipBuilder {

StructuralSamplerTool::StructuralSamplerTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::StructuralSampler,
        controller,
        resourceLocator)
{}

ElectricalSamplerTool::ElectricalSamplerTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::ElectricalSampler,
        controller,
        resourceLocator)
{}

RopeSamplerTool::RopeSamplerTool(
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::RopeSampler,
        controller,
        resourceLocator)
{}

template<LayerType TLayerType>
SamplerTool<TLayerType>::SamplerTool(
    ToolType toolType,
    Controller & controller,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
{
    SetCursor(WxHelpers::LoadCursorImage("sampler_cursor", 1, 30, resourceLocator));

    // See if should sample right away
    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInWorkCanvas();
    if (mouseShipCoordinates)
    {
        mController.BroadcastSampledInformationUpdatedAt(*mouseShipCoordinates, TLayerType);
    }
}

template<LayerType TLayerType>
SamplerTool<TLayerType>::~SamplerTool()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    mController.BroadcastSampledInformationUpdatedAt(ScreenToShipSpace(mouseCoordinates), TLayer);
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnLeftMouseDown()
{
    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoordinates)
    {
        DoSelectMaterial(
            *mouseShipCoordinates,
            MaterialPlaneType::Foreground);
    }
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnRightMouseDown()
{
    auto const mouseShipCoordinates = GetCurrentMouseShipCoordinatesIfInShip();
    if (mouseShipCoordinates)
    {
        DoSelectMaterial(
            *mouseShipCoordinates,
            MaterialPlaneType::Background);
    }
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnMouseLeft()
{
    mController.BroadcastSampledInformationUpdatedNone();
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void SamplerTool<TLayer>::DoSelectMaterial(
    ShipSpaceCoordinates const & mouseCoordinates,
    MaterialPlaneType plane)
{
    assert(mouseCoordinates.IsInSize(mController.GetModelController().GetShipSize()));

    // Get material
    auto const * material = SampleMaterial(mouseCoordinates);

    // Select material
    if constexpr (TLayer == LayerType::Structural)
    {
        mController.GetWorkbenchState().SetStructuralMaterial(material, plane);
        mController.GetUserInterface().OnStructuralMaterialChanged(material, plane);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        mController.GetWorkbenchState().SetElectricalMaterial(material, plane);
        mController.GetUserInterface().OnElectricalMaterialChanged(material, plane);
    }
    else
    {
        static_assert(TLayer == LayerType::Ropes);

        if (material != nullptr) // Ropes material is not allowed to be NULL
        {
            mController.GetWorkbenchState().SetRopesMaterial(material, plane);
            mController.GetUserInterface().OnRopesMaterialChanged(material, plane);
        }
    }
}

template<LayerType TLayer>
typename SamplerTool<TLayer>::LayerMaterialType const * SamplerTool<TLayer>::SampleMaterial(ShipSpaceCoordinates const & mouseCoordinates)
{
    assert(mouseCoordinates.IsInSize(mController.GetModelController().GetShipSize()));

    if constexpr (TLayer == LayerType::Structural)
    {
        return mController.GetModelController().SampleStructuralMaterialAt(mouseCoordinates);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        return mController.GetModelController().SampleElectricalMaterialAt(mouseCoordinates);
    }
    else
    {
        static_assert(TLayer == LayerType::Ropes);
        return mController.GetModelController().SampleRopesMaterialAt(mouseCoordinates);
    }
}

}