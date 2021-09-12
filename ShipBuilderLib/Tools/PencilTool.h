/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "BaseTool.h"

#include <Game/Materials.h>

namespace ShipBuilder {

template<LayerType Layer>
class PencilTool : public BaseTool
{
public:

    PencilTool(
        ToolType toolType,
        ModelController & modelController,
        SelectionManager & selectionManager,
        IUserInterface & userInterface,
        View & view)
        : BaseTool(
            toolType,
            modelController,
            selectionManager,
            userInterface,
            view)
    {}
};

class StructuralPencilTool : public PencilTool<LayerType::Structural>
{
public:

    StructuralPencilTool(
        ModelController & modelController,
        SelectionManager & selectionManager,
        IUserInterface & userInterface,
        View & view);
};

class ElectricalPencilTool : public PencilTool<LayerType::Electrical>
{
public:

    ElectricalPencilTool(
        ModelController & modelController,
        SelectionManager & selectionManager,
        IUserInterface & userInterface,
        View & view);
};

}