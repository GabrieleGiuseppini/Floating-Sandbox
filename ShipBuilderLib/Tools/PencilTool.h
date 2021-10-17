/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "Tool.h"

#include <Game/LayerBuffers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer, bool IsEraser>
class PencilTool : public Tool
{
public:

    void Reset() override;

    void OnMouseMove(InputState const & inputState) override;
    void OnLeftMouseDown(InputState const & inputState) override;
    void OnLeftMouseUp(InputState const & inputState) override;
    void OnRightMouseDown(InputState const & inputState) override;
    void OnRightMouseUp(InputState const & inputState) override;
    void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    void OnShiftKeyUp(InputState const & /*inputState*/) override {}

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

    using LayerElementType = typename LayerTypeTraits<TLayer>::buffer_type::element_type;

private:

    void TakeOriginalLayerBufferClone();

    void StartEngagement(InputState const & inputState);

    void DoEdit(InputState const & inputState);

    void EndEngagement();

    // TODOOLD
    void CheckEdit(InputState const & inputState);

    void MendTempVisualization();

    std::optional<ShipSpaceRect> CalculateApplicableRect(ShipSpaceCoordinates const & coords) const;

    int GetPencilSize() const;

    LayerElementType GetFillElement(MaterialPlaneType plane) const;

private:

    // Original layer buffer
    std::unique_ptr<typename LayerTypeTraits<TLayer>::buffer_type> mOriginalLayerBufferClone;

    // Ship region dirtied so far with temporary visualization
    std::optional<ShipSpaceRect> mTempVisualizationDirtyShipRegion;

    struct EngagementData
    {
        // Plane of the engagement
        MaterialPlaneType const Plane;

        // Rectangle of edit operation
        ShipSpaceRect EditRegion;

        // Dirty state
        Model::DirtyState OriginalDirtyState;

        // Position of previous engagement (when this is second, third, etc.)
        std::optional<ShipSpaceCoordinates> PreviousEngagementPosition;

        EngagementData(
            MaterialPlaneType plane,
            ShipSpaceCoordinates const & initialPosition,
            Model::DirtyState const & dirtyState)
            : Plane(plane)
            , EditRegion(initialPosition)
            , OriginalDirtyState(dirtyState)
            , PreviousEngagementPosition()
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;

    wxImage mCursorImage;
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