/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-11-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <optional>

namespace ShipBuilder {

class StructuralRectangleTool : public Tool
{
public:

    StructuralRectangleTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);

    ~StructuralRectangleTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override {}

protected:


private:

    ShipSpaceCoordinates GetCornerCoordinatesEngaged() const;

    ShipSpaceCoordinates GetCornerCoordinatesEngaged(DisplayLogicalCoordinates const & input) const;

    std::optional<ShipSpaceCoordinates> GetCornerCoordinatesFree() const;

    void UpdateEphemeralRectangle(ShipSpaceCoordinates const & cornerCoordinates);

private:

    std::optional<ShipSpaceRect> mCurrentRectangle;

    struct EngagementData
    {
        ShipSpaceCoordinates StartCorner;

        EngagementData(ShipSpaceCoordinates const & startCorner)
            : StartCorner(startCorner)
        {}
    };

    // When set, we're engaged (dragging)
    std::optional<EngagementData> mEngagementData;

    bool mIsShiftDown;
};

}
