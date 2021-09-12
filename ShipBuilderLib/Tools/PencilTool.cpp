/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PencilTool.h"

namespace ShipBuilder {

StructuralPencilTool::StructuralPencilTool(
    ModelController & modelController,
    SelectionManager & selectionManager,
    IUserInterface & userInterface,
    View & view)
    : PencilTool(
        ToolType::StructuralPencil,
        modelController,
        selectionManager,
        userInterface,
        view)
{}

ElectricalPencilTool::ElectricalPencilTool(
    ModelController & modelController,
    SelectionManager & selectionManager,
    IUserInterface & userInterface,
    View & view)
    : PencilTool(
        ToolType::ElectricalPencil,
        modelController,
        selectionManager,
        userInterface,
        view)
{}

}