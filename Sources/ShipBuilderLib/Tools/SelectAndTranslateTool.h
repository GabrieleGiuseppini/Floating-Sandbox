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

class ISelectAndTranslateTool
{
public:
    virtual void SetIsAllLayers(bool value) = 0;
    virtual void SetIsTransparent(bool value) = 0;
};

template<LayerType TLayer, typename TCoordsTag>
class SelectAndTranslateTool : public Tool, public ISelectAndTranslateTool
{
public:

    ~SelectAndTranslateTool();

    ToolClass GetClass() const override
    {
        return ToolClass::SelectAndTranslate;
    }

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override {}

    void SetIsAllLayers(bool value) override;
    void SetIsTransparent(bool value) override;

protected:

    using TCoords = typename _IntegralCoordinates<TCoordsTag>;
    using TSize = typename _IntegralSize<TCoordsTag>;
    using TRect = typename _IntegralRect<TCoordsTag>;

    SelectAndTranslateTool(
        ToolType toolType,
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    void InternalStartMove();

    void InternalDoMove(TSize totalMoveOffset);

    void UpdateEphemeralSelection(TRect const & rect);

    void StopEphemeralSelection();

    TCoords GetMouseSelectionCoords() const; // Also outside space
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

        // For Selecting and onwards: origin of selection; finalized with achieved translation
        // at end of each move session
        TCoords SelectionStartCorner;

        // When Selected and onwards; presence of this indicates we are drawing overlays
        std::optional<TRect> SelectionRect;

        struct MovingStateType
        {
            TCoords const MoveStartCoords;
            bool HasStartedMove; // To detect first change, at which moment we create Undo action
            std::optional<TSize> MoveShiftConstraints; // Empty until detected

            MovingStateType(TCoords const & startCoords)
                : MoveStartCoords(startCoords)
                , HasStartedMove(false)
            { }
        };

        // When moving
        std::optional<MovingStateType> MovingState;

        EngagementData(TCoords selectionStartCorner)
            : State(StateType::Selecting)
            , SelectionStartCorner(selectionStartCorner)
        { }
    };

    std::optional<EngagementData> mEngagementData;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;

    // From workbench
    bool mIsAllLayers;
    bool mIsTransparent;

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
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ElectricalSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::Electrical, struct ShipSpaceTag>
{
public:

    ElectricalSelectAndTranslateTool(
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class RopeSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::Ropes, struct ShipSpaceTag>
{
public:

    RopeSelectAndTranslateTool(
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class ExteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::ExteriorTexture, struct ImageTag>
{
public:

    ExteriorSelectAndTranslateTool(
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class InteriorSelectAndTranslateTool final : public SelectAndTranslateTool<LayerType::InteriorTexture, struct ImageTag>
{
public:

    InteriorSelectAndTranslateTool(
        bool isAllLayers,
        bool isTransparent,
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}
