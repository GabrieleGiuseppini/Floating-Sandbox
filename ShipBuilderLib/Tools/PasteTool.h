/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GenericUndoPayload.h"
#include "Tool.h"

#include <UILib/WxHelpers.h>

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

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

    void Commit();
    void Abort();

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

    ShipSpaceCoordinates CalculateInitialMouseOrigin() const;

    ShipSpaceCoordinates MouseCoordsToPasteOrigin(ShipSpaceCoordinates const & mouseCoords)
    {
        // We want paste' to b's top-left corner to be at the top-left corner of the ship "square" 
        // whose bottom-left corner is the specified mouse coords
        return ShipSpaceCoordinates(
            mouseCoords.x,
            mouseCoords.y)
            - ShipSpaceSize(0, mPasteRegion.Size.height - 1);
    }

    void DrawEphemeralVisualization();
    void UndoEphemeralVisualization();

private:

    ShipLayers mPasteRegion;    
    bool mIsTransparent;

    ShipSpaceCoordinates mMousePasteCoords;

    // Only set while we're dragging - remembers the previous mouse pos
    std::optional<ShipSpaceCoordinates> mMouseAnchor;

    std::optional<GenericUndoPayload> mEphemeralVisualization;
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