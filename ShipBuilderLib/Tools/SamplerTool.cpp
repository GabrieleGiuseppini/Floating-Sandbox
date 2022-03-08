/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-02-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SamplerTool.h"

#include <Controller.h>

#include <type_traits>

namespace ShipBuilder {

StructuralSamplerTool::StructuralSamplerTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::StructuralSampler,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

ElectricalSamplerTool::ElectricalSamplerTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::ElectricalSampler,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

RopeSamplerTool::RopeSamplerTool(
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : SamplerTool(
        ToolType::RopeSampler,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view,
        resourceLocator)
{}

template<LayerType TLayerType>
SamplerTool<TLayerType>::SamplerTool(
    ToolType toolType,
    ModelController & modelController,
    UndoStack & undoStack,
    WorkbenchState & workbenchState,
    IUserInterface & userInterface,
    View & view,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        modelController,
        undoStack,
        workbenchState,
        userInterface,
        view)
    , mCursorImage(WxHelpers::LoadCursorImage("sampler_cursor", 1, 30, resourceLocator))
{
    SetCursor(mCursorImage);
}

template<LayerType TLayerType>
SamplerTool<TLayerType>::~SamplerTool()
{
    // Notify about material
    mUserInterface.OnSampledMaterialChanged(std::nullopt);
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    auto const coords = ScreenToShipSpace(mouseCoordinates);
    if (coords.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        // Get material
        auto const * material = SampleMaterial(coords);

        // Notify about material
        mUserInterface.OnSampledMaterialChanged(material ? material->Name : std::optional<std::string>());
    }
    else
    {
        mUserInterface.OnSampledMaterialChanged(std::nullopt);
    }
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnLeftMouseDown()
{
    auto const coords = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (coords)
    {
        DoSelectMaterial(
            ScreenToShipSpace(*coords),
            MaterialPlaneType::Foreground);
    }
}

template<LayerType TLayer>
void SamplerTool<TLayer>::OnRightMouseDown()
{
    auto const coords = mUserInterface.GetMouseCoordinatesIfInWorkCanvas();
    if (coords)
    {
        DoSelectMaterial(
            ScreenToShipSpace(*coords),
            MaterialPlaneType::Background);
    }
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
void SamplerTool<TLayer>::DoSelectMaterial(
    ShipSpaceCoordinates const & mouseCoordinates,
    MaterialPlaneType plane)
{
    if (mouseCoordinates.IsInSize(mModelController.GetModel().GetShipSize()))
    {
        // Get material
        auto const * material = SampleMaterial(mouseCoordinates);

        // Select material
        if constexpr (TLayer == LayerType::Structural)
        {
            mWorkbenchState.SetStructuralMaterial(material, plane);
            mUserInterface.OnStructuralMaterialChanged(material, plane);
        }
        else if constexpr (TLayer == LayerType::Electrical)
        {
            mWorkbenchState.SetElectricalMaterial(material, plane);
            mUserInterface.OnElectricalMaterialChanged(material, plane);
        }
        else
        {
            static_assert(TLayer == LayerType::Ropes);
            mWorkbenchState.SetRopesMaterial(material, plane);
            mUserInterface.OnRopesMaterialChanged(material, plane);
        }
    }
}

template<LayerType TLayer>
typename SamplerTool<TLayer>::LayerMaterialType const * SamplerTool<TLayer>::SampleMaterial(ShipSpaceCoordinates const & mouseCoordinates)
{
    assert(mouseCoordinates.IsInSize(mModelController.GetModel().GetShipSize()));

    if constexpr (TLayer == LayerType::Structural)
    {
        return mModelController.SampleStructuralMaterialAt(mouseCoordinates);
    }
    else if constexpr (TLayer == LayerType::Electrical)
    {
        return mModelController.SampleElectricalMaterialAt(mouseCoordinates);
    }
    else
    {
        return mModelController.SampleRopesMaterialAt(mouseCoordinates);
    }
}

}