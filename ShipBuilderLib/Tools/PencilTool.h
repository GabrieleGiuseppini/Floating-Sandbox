/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>
#include <GameCore/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer, bool IsEraser>
class PencilTool : public Tool
{
public:

    ~PencilTool();

    void OnMouseMove(ShipSpaceCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override {}
    void OnShiftKeyUp() override {}

protected:

    PencilTool(
        ToolType toolType,
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);

private:

    using LayerMaterialType = typename LayerTypeTraits<TLayer>::material_type;

private:

    void StartEngagement(StrongTypedBool<struct IsRightMouseButton> isRightButton);

    void DoEdit(ShipSpaceCoordinates const & mouseCoordinates);

    void EndEngagement();

    void DoTempVisualization(ShipSpaceRect const & affectedRect);

    void MendTempVisualization();

    std::optional<ShipSpaceRect> CalculateApplicableRect(ShipSpaceCoordinates const & coords) const;

    int GetPencilSize() const;

    inline LayerMaterialType const * GetFillMaterial(MaterialPlaneType plane) const;

private:

    // Original layer
    typename LayerTypeTraits<TLayer>::layer_data_type mOriginalLayerClone;

    // Ship region dirtied so far with temporary visualization
    std::optional<ShipSpaceRect> mTempVisualizationDirtyShipRegion;

    struct EngagementData
    {
        // Plane of the engagement
        MaterialPlaneType const Plane;

        // Rectangle of edit operation
        std::optional<ShipSpaceRect> EditRegion;

        // Dirty state
        Model::DirtyState OriginalDirtyState;

        // Position of previous engagement (when this is second, third, etc.)
        std::optional<ShipSpaceCoordinates> PreviousEngagementPosition;

        EngagementData(
            MaterialPlaneType plane,
            Model::DirtyState const & dirtyState)
            : Plane(plane)
            , EditRegion()
            , OriginalDirtyState(dirtyState)
            , PreviousEngagementPosition()
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;
};

class StructuralPencilTool : public PencilTool<LayerType::Structural, false>
{
public:

    StructuralPencilTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class ElectricalPencilTool : public PencilTool<LayerType::Electrical, false>
{
public:

    ElectricalPencilTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class StructuralEraserTool : public PencilTool<LayerType::Structural, true>
{
public:

    StructuralEraserTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class ElectricalEraserTool : public PencilTool<LayerType::Electrical, true>
{
public:

    ElectricalEraserTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

}