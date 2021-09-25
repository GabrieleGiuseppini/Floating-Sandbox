/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ShipBuilderTypes.h"

#include <Game/LayerBuffers.h>

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

    virtual void ApplyAction(Controller & controller) const = 0;

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

template<typename TLayerBuffer>
class LayerBufferRegionUndoAction final : public UndoAction
{
public:

    LayerBufferRegionUndoAction(
        wxString const & title,
        Model::DirtyState const & originalDirtyState,
        TLayerBuffer && layerBufferRegion,
        ShipSpaceCoordinates const & origin)
        : UndoAction(
            title,
            layerBufferRegion.GetByteSize(),
            originalDirtyState)
        , mLayerBufferRegion(std::move(layerBufferRegion))
        , mOrigin(origin)
    {}

    void ApplyAction(Controller & controller) const override;

private:

    TLayerBuffer mLayerBufferRegion;
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