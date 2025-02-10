/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include "../GenericEphemeralVisualizationRestorePayload.h"
#include "../GenericUndoPayload.h"

#include <UILib/WxHelpers.h>

#include <Game/GameAssetManager.h>

#include <Simulation/Layers.h>

#include <Core/GameTypes.h>

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
        GameAssetManager const & gameAssetManager);

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
        GameAssetManager const & gameAssetManager);
};

class ElectricalPasteTool : public PasteTool
{
public:

    ElectricalPasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class RopePasteTool : public PasteTool
{
public:

    RopePasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ExteriorTexturePasteTool : public PasteTool
{
public:

    ExteriorTexturePasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class InteriorTexturePasteTool : public PasteTool
{
public:

    InteriorTexturePasteTool(
        ShipLayers && pasteRegion,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}