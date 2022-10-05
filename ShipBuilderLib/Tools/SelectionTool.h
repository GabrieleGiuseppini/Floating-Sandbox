/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class SelectionTool : public Tool
{
public:

    ~SelectionTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override;
    void OnRightMouseUp() override;
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;

protected:

    SelectionTool(
        ToolType toolType,
        Controller & controller,
        ResourceLocator const & resourceLocator);

private:

    // When set - and not empty - we have a selection (*and* thus also
    // a selection overlay)
    std::optional<ShipSpaceRect> mCurrentRect;

    struct EngagementData
    {
        ShipSpaceCoordinates SelectionStartCorner;
    };

    // When set, we're engaged (dragging)
    std::optional<EngagementData> mEngagementData;
};

class StructuralSelectionTool final : public SelectionTool<LayerType::Structural>
{
public:

    StructuralSelectionTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class ElectricalSelectionTool final : public SelectionTool<LayerType::Electrical>
{
public:

    ElectricalSelectionTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class RopeSelectionTool final : public SelectionTool<LayerType::Ropes>
{
public:

    RopeSelectionTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class TextureSelectionTool final : public SelectionTool<LayerType::Texture>
{
public:

    TextureSelectionTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

}
