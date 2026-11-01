/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-11-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Simulation/Layers.h>

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <optional>

#include <wx/image.h>

namespace ShipBuilder {

template<LayerType TLayer, typename TCoordsTag>
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

    void UpdateEphemeralSelection(TRect const & rect);

    void StopEphemeralSelection();

    std::optional<TCoords> GetMouseSelectionCoordsIfInSpace() const;
    TCoords GetMouseSelectionCoordsClamped() const;

    TSize GetSpaceSize() const;

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

        // For Selecting
        TCoords SelectionStartCorner;

        // When Selected and onwards; presence of this indicates we are drawing overlays
        std::optional<TRect> SelectionRect;

        // When moving
        std::optional<TCoords> MoveStartCoords;

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

class StructuralSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::Structural, struct ShipSpaceTag>
{
public:

    StructuralSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::Electrical, struct ShipSpaceTag>
{
public:

    ElectricalSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class RopeSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::Ropes, struct ShipSpaceTag>
{
public:

    RopeSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ExteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::ExteriorTexture, struct ImageTag>
{
public:

    ExteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class InteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::InteriorTexture, struct ImageTag>
{
public:

    InteriorSelectAndTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}
