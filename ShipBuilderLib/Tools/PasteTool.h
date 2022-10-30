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

class PasteTool : public Tool
{
public:

    virtual ~PasteTool();

    ToolClass GetClass() const override
    {
        return ToolClass::Paste;
    }

    void OnMouseMove(DisplayLogicalCoordinates const & /*mouseCoordinates*/) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {};
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override;

    void SetIsTransparent(bool isTransparent);
    void Rotate90CW();
    void Rotate90CCW();
    void FlipH();
    void FlipV();

protected:

    PasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        ToolType toolType,
        Controller & controller,
        ResourceLocator const & resourceLocator);

private:

    ShipLayers mPasteRegion;
    bool mIsTransparent;
};

class StructuralPasteTool : public PasteTool
{
public:

    StructuralPasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class ElectricalPasteTool : public PasteTool
{
public:

    ElectricalPasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class RopePasteTool : public PasteTool
{
public:

    RopePasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class TexturePasteTool : public PasteTool
{
public:

    TexturePasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

}