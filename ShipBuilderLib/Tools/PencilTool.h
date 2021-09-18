/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

template<typename TMaterial>
class PencilTool : public Tool
{
protected:

    PencilTool(
        ToolType toolType,
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);

    void Reset() override {}

    void OnMouseMove(InputState const & inputState) override;
    void OnLeftMouseDown(InputState const & inputState) override;
    void OnLeftMouseUp(InputState const & /*inputState*/) override {};
    void OnRightMouseDown(InputState const & inputState) override;
    void OnRightMouseUp(InputState const & /*inputState*/) override {};
    void OnShiftKeyDown(InputState const & /*inputState*/) override {}
    void OnShiftKeyUp(InputState const & /*inputState*/) override {}
    void OnMouseOut(InputState const & /*inputState*/) override {}

private:

    void ApplyEditAt(
        WorkSpaceCoordinates const & position,
        MaterialPlaneType plane);

private:

    wxImage mCursorImage;
};

class StructuralPencilTool : public PencilTool<StructuralMaterial>
{
public:

    StructuralPencilTool(
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

class ElectricalPencilTool : public PencilTool<ElectricalMaterial>
{
public:

    ElectricalPencilTool(
        ModelController & modelController,
        WorkbenchState const & workbenchState,
        IUserInterface & userInterface,
        View & view,
        ResourceLocator const & resourceLocator);
};

}