/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <UILib/WxHelpers.h>

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class PasteTool : public Tool
{
public:

    virtual ~PasteTool();

    void OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {};
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override;

protected:

    PasteTool(
        ShipLayers && pasteRegion,
        ToolType toolType,
        Controller & controller,
        ResourceLocator const & resourceLocator);

private:

    ShipLayers mPasteRegion;

    wxImage mCursorImage;
};

class StructuralPasteTool : public PasteTool<LayerType::Structural>
{
public:

    StructuralPasteTool(
        ShipLayers && pasteRegion,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class ElectricalPasteTool : public PasteTool<LayerType::Electrical>
{
public:

    ElectricalPasteTool(
        ShipLayers && pasteRegion,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class RopePasteTool : public PasteTool<LayerType::Ropes>
{
public:

    RopePasteTool(
        ShipLayers && pasteRegion,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class TexturePasteTool : public PasteTool<LayerType::Texture>
{
public:

    TexturePasteTool(
        ShipLayers && pasteRegion,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

}