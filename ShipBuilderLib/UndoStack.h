/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ShipBuilderTypes.h"

#include <Game/Layers.h>

#include <wx/string.h>

#include <deque>
#include <memory>
#include <string>

namespace ShipBuilder {

// Forward declarations
class Controller;

/*
 * Base class of the hierarchy of undo actions.
 *
 * Some examples of specializations include:
 * - Material region replace
 * - Resize
 */
class UndoAction
{
public:

    virtual ~UndoAction() = default;

    wxString const & GetTitle() const
    {
        return mTitle;
    }

    size_t GetCost() const
    {
        return mCost;
    }

    Model::DirtyState const & GetOriginalDirtyState() const
    {
        return mOriginalDirtyState;
    }

    virtual void ApplyAndConsume(Controller & controller) = 0;

protected:

    UndoAction(
        wxString const & title,
        size_t cost,
        Model::DirtyState const & originalDirtyState)
        : mTitle(title)
        , mCost(cost)
        , mOriginalDirtyState(originalDirtyState)
    {}

private:

    wxString const mTitle;
    size_t const mCost;
    Model::DirtyState const mOriginalDirtyState; // The model's dirty state that was in effect when the edit action being undode was applied
};

template<typename TLayer>
class LayerRegionUndoAction final : public UndoAction
{
public:

    LayerRegionUndoAction(
        wxString const & title,
        Model::DirtyState const & originalDirtyState,
        TLayer && layerRegion,
        ShipSpaceCoordinates const & origin)
        : UndoAction(
            title,
            layerRegion.Buffer.GetByteSize(),
            originalDirtyState)
        , mLayerRegion(std::move(layerRegion))
        , mOrigin(origin)
    {}

    void ApplyAndConsume(Controller & controller) override;

private:

    TLayer mLayerRegion;
    ShipSpaceCoordinates mOrigin;
};

class UndoStack
{
public:

    UndoStack()
        : mStack()
        , mTotalCost(0)
    {}

    bool IsEmpty() const
    {
        return mStack.empty();
    }

    void Push(std::unique_ptr<UndoAction> && undoAction);

    std::unique_ptr<UndoAction> Pop();

private:

    static size_t constexpr MaxEntries = 20;
    static size_t constexpr MaxCost = (1000 * 1000) * 20;

    std::deque<std::unique_ptr<UndoAction>> mStack;
    size_t mTotalCost;
};

}