/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SelectionManager.h"
#include "Tool.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <optional>

namespace ShipBuilder {

class SelectionTool : public Tool
{
public:

    ~SelectionTool();
    
    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override {}

protected:

    SelectionTool(
        ToolType toolType,
        Controller & controller,
        SelectionManager & selectionManager,
        ResourceLocator const & resourceLocator);

private:

    ShipSpaceCoordinates GetCornerCoordinatesEngaged() const;

    ShipSpaceCoordinates GetCornerCoordinatesEngaged(DisplayLogicalCoordinates const & input) const;

    std::optional<ShipSpaceCoordinates> GetCornerCoordinatesFree() const;

    void UpdateEphemeralSelection(ShipSpaceCoordinates const & cornerCoordinates);

private:

    SelectionManager & mSelectionManager;

    // Carries the same selection currently in the SelectionManager
    std::optional<ShipSpaceRect> mCurrentSelection;

    struct EngagementData
    {
        ShipSpaceCoordinates SelectionStartCorner;

        EngagementData(ShipSpaceCoordinates const & selectionStartCorner)
            : SelectionStartCorner(selectionStartCorner)
        {}
    };

    // When set, we're engaged (dragging)
    std::optional<EngagementData> mEngagementData;

    bool mIsShiftDown;
};

class StructuralSelectionTool final : public SelectionTool
{
public:

    StructuralSelectionTool(
        Controller & controller,
        SelectionManager & selectionManager,
        ResourceLocator const & resourceLocator);
};

class ElectricalSelectionTool final : public SelectionTool
{
public:

    ElectricalSelectionTool(
        Controller & controller,
        SelectionManager & selectionManager,
        ResourceLocator const & resourceLocator);
};

class RopeSelectionTool final : public SelectionTool
{
public:

    RopeSelectionTool(
        Controller & controller,
        SelectionManager & selectionManager,
        ResourceLocator const & resourceLocator);
};

class TextureSelectionTool final : public SelectionTool
{
public:

    TextureSelectionTool(
        Controller & controller,
        SelectionManager & selectionManager,
        ResourceLocator const & resourceLocator);
};

}
