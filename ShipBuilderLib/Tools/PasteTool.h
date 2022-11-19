/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GenericEphemeralVisualizationRestorePayload.h"
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
    void OnMouseLeft() override {};

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

    ShipSpaceCoordinates MousePasteCoordsToActualPasteOrigin(
        ShipSpaceCoordinates const & mousePasteCoords,
        ShipSpaceSize const & pasteRegionSize) const;

    ShipSpaceCoordinates ClampMousePasteCoords(
        ShipSpaceCoordinates const & mousePasteCoords,
        ShipSpaceSize const & pasteRegionSize) const;

    void UpdateEphemeralVisualization(ShipSpaceCoordinates const & mouseCoordinates);

    void DrawEphemeralVisualization();

    void UndoEphemeralVisualization();

    template<typename TModifier>
    void ModifyPasteRegion(TModifier && modifier);

private:

    bool mIsShiftDown;

    struct PendingSessionData
    {
        ShipLayers PasteRegion;
        bool IsTransparent;

        ShipSpaceCoordinates MousePasteCoords;

        // When set we have an ephemeral visualization
        std::optional<GenericEphemeralVisualizationRestorePayload> EphemeralVisualization;

        PendingSessionData(
            ShipLayers && pasteRegion,
            bool isTransparent,
            ShipSpaceCoordinates const & mousePasteCoords)
            : PasteRegion(std::move(pasteRegion))
            , IsTransparent(isTransparent)
            , MousePasteCoords(mousePasteCoords)
            , EphemeralVisualization()
        {}
    };

    // Only set when the current paste has not been committed nor aborted yet
    std::optional<PendingSessionData> mPendingSessionData;

    struct DragSessionData
    {
        ShipSpaceCoordinates LastMousePosition;
        std::optional<ShipSpaceCoordinates> LockedOrigin;

        DragSessionData(
            ShipSpaceCoordinates const & currentMousePosition,
            bool isLocked)
            : LastMousePosition(currentMousePosition)
            , LockedOrigin(isLocked ? currentMousePosition : std::optional<ShipSpaceCoordinates>())
        {}
    };

    // Only set while we're dragging
    std::optional<DragSessionData> mDragSessionData;
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