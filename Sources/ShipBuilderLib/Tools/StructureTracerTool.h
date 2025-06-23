/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-06-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include "../SelectionManager.h"

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <wx/image.h>

#include <optional>

namespace ShipBuilder {

class StructureTracerTool final : public Tool
{
public:

    StructureTracerTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~StructureTracerTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override {}
    void OnShiftKeyUp() override {}
    void OnMouseLeft() override {}

private:

    // TODOHERE

private:

    // TODOHERE
};

}
