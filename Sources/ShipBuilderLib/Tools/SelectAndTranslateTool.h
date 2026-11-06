/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-11-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include "../GenericEphemeralVisualizationRestorePayload.h"

#include <Simulation/Layers.h>

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <optional>

#include <wx/image.h>

namespace ShipBuilder {

template<LayerType TLayer, typename TEphemeralVisualizationRestorePayload, typename TCoordsTag>
class SelectAndTranslateTool : public Tool
{
public:

    ~SelectAndTranslateTool();

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

private:

    void InternalInitSelectionMoves();

    void InternalDoMoveForEphemeralVisualization(TSize totalMoveOffset);

    void InternalCommit();

    void DrawEphemeralVisualization();
    void UndoEphemeralVisualization();

    void UpdateSelectionViz(TRect const & rect);
    void StopSelectionViz();

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

        // For Selecting: origin of selection
        TCoords SelectionStartCorner;

        // When Selected and onwards; model region for current selection
        std::optional<TRect> OriginalSelectionRect;

        // When Selected and onwards; pasted position for current selection, when set we have overlay;
        // updated at each move
        std::optional<TRect> CurrentPastedSelectionRect;

        std::optional<TCoords> CurrentMoveStartCoords; // Upon click in selection and transitioning to Moving

        struct SelectionMovesType
        {
            TCoords CurrentSelectionPasteCornerCoords; // Current coords of pasted selection; updated at each move end with final coords, from that move's final CurrentSelectionPasteCornerCoords

            struct MovingStateType
            {
                TCoords CurrentSelectionPasteCornerCoords; // Current coords of pasted selection; updated at each move
                std::optional<TSize> MoveShiftConstraints; // Empty until detected

                MovingStateType(TCoords const & currentSelectionPasteCornerCoords)
                    : CurrentSelectionPasteCornerCoords(currentSelectionPasteCornerCoords)
                { }
            };

            // Current move - only set when move has begun
            std::optional<MovingStateType> MovingState;

            // Frozen tool settings - these affect Undo, thus we freeze them when we start undo
            bool const IsAllLayers;
            bool const IsTransparent;

            // When set we have an ephemeral visualization
            std::optional<TEphemeralVisualizationRestorePayload> EphemeralVisualization;

            SelectionMovesType(
                TCoords const & startSelectionPasteCornerCoords,
                bool isAllLayers,
                bool isTransparent)
                : CurrentSelectionPasteCornerCoords(startSelectionPasteCornerCoords)
                , IsAllLayers(isAllLayers)
                , IsTransparent(isTransparent)
            { }
        };

        // When set, we have started moving for this selection - which also means having done Undo;
        // also, means we still need to commit move - we are in eph viz
        std::optional<SelectionMovesType> SelectionMoves;

        EngagementData(TCoords selectionStartCorner)
            : State(StateType::Selecting)
            , SelectionStartCorner(selectionStartCorner)
        { }
    };

    std::optional<EngagementData> mEngagementData;

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
class SelectAndTranslateTool_SER : public SelectAndTranslateTool<TLayer, GenericEphemeralVisualizationRestorePayload, struct ShipSpaceTag>
{
protected:

    SelectAndTranslateTool_SER(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

    ShipSpaceSize GetSpaceSize() const override;
};

class StructuralSelectAndTranslateTool final : public SelectAndTranslateTool_SER<LayerType::Structural>
{
public:

    StructuralSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalSelectAndTranslateTool final : public SelectAndTranslateTool_SER<LayerType::Electrical>
{
public:

    ElectricalSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class RopeSelectAndTranslateTool final : public SelectAndTranslateTool_SER<LayerType::Ropes>
{
public:

    RopeSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ExteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::ExteriorTexture, GenericTextureEphemeralVisualizationRestorePayload<LayerType::ExteriorTexture>, struct ImageTag>
{
public:

    ExteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

protected:

    ImageSize GetSpaceSize() const override;
};

class InteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::InteriorTexture, GenericTextureEphemeralVisualizationRestorePayload<LayerType::InteriorTexture>, struct ImageTag>
{
public:

    InteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);

protected:

    ImageSize GetSpaceSize() const override;
};

}
