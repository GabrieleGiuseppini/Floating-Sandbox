/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-06-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StructureTracerTool.h"

#include "../Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

StructureTracerTool::StructureTracerTool(
    Controller & controller,
    GameAssetManager const & gameAssetManager)
    : Tool(
        ToolType::StructureTracer,
        controller)
    // TODOHERE
    //, mEngagementData()
    //, mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("crosshair_cursor", 15, 15, gameAssetManager));
}

StructureTracerTool::~StructureTracerTool()
{
}

void StructureTracerTool::OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/)
{
    // TODOHERE
}

void StructureTracerTool::OnLeftMouseDown()
{
    // TODOHERE
}

void StructureTracerTool::OnLeftMouseUp()
{
    // TODOHERE
}

}