/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-31
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>
#include <GameCore/StrongTypeDef.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class FloodTool : public Tool
{
public:

    ~FloodTool() = default;

    void OnMouseMove(ShipSpaceCoordinates const & /*mouseCoordinates*/) override {};
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override {};
    void OnRightMouseDown() override;
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override {};
    void OnShiftKeyUp() override {};

protected:

    FloodTool(
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

    void DoEdit(
        ShipSpaceCoordinates const & mouseCoordinates,
        StrongTypedBool<struct IsRightMouseButton> isRightButton);

    inline LayerMaterialType const * GetFloodMaterial(MaterialPlaneType plane) const;

private:

    wxImage mCursorImage;
};

class StructuralFloodTool : public FloodTool<LayerType::Structural>
{
public:

    StructuralFloodTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class ElectricalFloodTool : public FloodTool<LayerType::Electrical>
{
public:

    ElectricalFloodTool(
        ModelController & modelController,
        UndoStack & undoStack,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

}