/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/ResourceLocator.h>

namespace ShipBuilder {

class TextureMagicWandTool : public Tool
{
public:

    TextureMagicWandTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);

    void OnMouseMove(DisplayLogicalCoordinates const &) override {}
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override {};
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override {};
    void OnShiftKeyUp() override {};
    void OnMouseLeft() override {};
};

}