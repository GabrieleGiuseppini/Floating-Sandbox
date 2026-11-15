/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-11-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include "../GenericEphemeralVisualizationRestorePayload.h"
#include "../GenericUndoPayload.h"
#include "../LayerSelector.h"

#include <Simulation/Layers.h>

#include <Game/GameAssetManager.h>

#include <Core/Finalizer.h>
#include <Core/GameTypes.h>

#include <wx/image.h>

#include <optional>

namespace ShipBuilder {

template<LayerType TLayer, typename TUndoPayload, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
class SelectAndTranslateTool : public Tool
{
public:

    virtual ~SelectAndTranslateTool() = default;

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override {}

protected:

    using TCoords = typename _IntegralCoordinates<TCoordsTag>;
    using TSize = typename _IntegralSize<TCoordsTag>;
    using TRect = typename _IntegralRect<TCoordsTag>;

    SelectAndTranslateTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    void OnDestroy();

private:

    void InternalDoMoveForEphemeralVisualization(TCoords const & newPasteCoords);

    void Commit();
    virtual TUndoPayload InternalCommit(
        TRect const & sourceRegion,
        TCoords const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) = 0;

    void DrawEphemeralVisualization();
    virtual TEphemeralVisualizationRestorePayload InternalDrawEphemeralVisualization(
        TRect const & sourceRegion,
        TCoords const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) = 0;

    void UpdateSelectionViz();
    void StopSelectionViz();

    // Uses current WorkbenchState
    virtual LayerSelector MakeLayerSelection() const = 0;
    virtual bool MakeIsTransparent() const = 0;

    TCoords GetMouseSelectionCoords() const; // Also outside space
    std::optional<TCoords> GetMouseSelectionCoordsIfInSpace() const;
    TCoords GetMouseSelectionCoordsClamped() const;
    TCoords ClampSelectionCoords(TCoords const & coords) const;

    virtual TSize GetSpaceSize() const = 0;

    void InternalSetCursor(wxImage const & cursorImage);

private:

    //
    // State
    //

    struct EngagementData
    {
        enum class StateType
        {
            Selecting,
            Selected,
            Moving,
        };

        StateType State;

        TCoords const SelectionStartCorner;
        TRect SourceRect; // Changes during Selecting; properly origined

        struct MovingStateType
        {
            TCoords const MoveStartCoords;
            std::optional<TSize> MoveShiftConstraints; // Empty until detected

            TCoords const SelectionRectStartOrigin; // The origin of the selection coords at move start

            MovingStateType(
                TCoords const & moveStartCoords,
                TCoords const & selectionRectStartOrigin)
                : MoveStartCoords(moveStartCoords)
                , SelectionRectStartOrigin(selectionRectStartOrigin)
            {
            }
        };

        // Current move
        std::optional<MovingStateType> CurrentMoveState;

        TRect PasteRect; // Changes during Selecting and Moving; not clamped to space; properly origined

        EngagementData(TCoords selectionStartCorner)
            : State(StateType::Selecting)
            , SelectionStartCorner(selectionStartCorner)
            , SourceRect(selectionStartCorner, TSize(0, 0))
            , PasteRect(selectionStartCorner, TSize(0, 0))
        { }
    };

    std::optional<EngagementData> mEngagementData;

    // When set we have an ephemeral visualization
    std::optional<Finalizer> mEphemeralVisualization;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;

    //
    // UI
    //

    // Cursors
    wxImage const mPointerCursor;
    wxImage const mMoveCursor;

    // Current cursor, for perf
    wxImage const *mCurrentCursor;
};

// Specialization for Structural/Electrical/Ropes layers, which work on ship space coords
template<LayerType TLayer>
class SelectAndTranslateTool_ShipSpace : public SelectAndTranslateTool<TLayer, GenericShipSpaceUndoPayload, GenericShipSpaceEphemeralVisualizationRestorePayload, ShipSpaceTag>
{
protected:

    SelectAndTranslateTool_ShipSpace(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    GenericShipSpaceUndoPayload InternalCommit(
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    GenericShipSpaceEphemeralVisualizationRestorePayload InternalDrawEphemeralVisualization(
        ShipSpaceRect const & sourceRegion,
        ShipSpaceCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    LayerSelector MakeLayerSelection() const override;
    bool MakeIsTransparent() const override;

    ShipSpaceSize GetSpaceSize() const override;
};

class StructuralSelectAndTranslateTool final : public SelectAndTranslateTool_ShipSpace<LayerType::Structural>
{
public:

    StructuralSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~StructuralSelectAndTranslateTool()
    {
        OnDestroy();
    }
};

class ElectricalSelectAndTranslateTool final : public SelectAndTranslateTool_ShipSpace<LayerType::Electrical>
{
public:

    ElectricalSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~ElectricalSelectAndTranslateTool()
    {
        OnDestroy();
    }
};

class RopeSelectAndTranslateTool final : public SelectAndTranslateTool_ShipSpace<LayerType::Ropes>
{
public:

    RopeSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~RopeSelectAndTranslateTool()
    {
        OnDestroy();
    }
};

class ExteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::ExteriorTexture, GenericExteriorTextureSpaceUndoPayload, GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload, ImageSpaceTag>
{
public:

    ExteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~ExteriorSelectAndTranslateTool()
    {
        OnDestroy();
    }

protected:

    GenericExteriorTextureSpaceUndoPayload InternalCommit(
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    GenericExteriorTextureSpaceEphemeralVisualizationRestorePayload InternalDrawEphemeralVisualization(
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    LayerSelector MakeLayerSelection() const override;
    bool MakeIsTransparent() const override;

    ImageSize GetSpaceSize() const override;
};

class InteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::InteriorTexture, GenericInteriorTextureSpaceUndoPayload, GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload, ImageSpaceTag>
{
public:

    InteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ~InteriorSelectAndTranslateTool()
    {
        OnDestroy();
    }

protected:

    GenericInteriorTextureSpaceUndoPayload InternalCommit(
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    GenericInteriorTextureSpaceEphemeralVisualizationRestorePayload InternalDrawEphemeralVisualization(
        ImageRect const & sourceRegion,
        ImageCoordinates const & targetPos,
        LayerSelector const & layerSelection,
        bool isTransparent) override;

    LayerSelector MakeLayerSelection() const override;
    bool MakeIsTransparent() const override;

    ImageSize GetSpaceSize() const override;
};

}
