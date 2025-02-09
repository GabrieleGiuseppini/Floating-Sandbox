/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "Controller.h"

#include <cassert>

namespace ShipBuilder {

void UndoStack::PopAndApply(Controller & controller)
{
    assert(!mStack.empty());

    // Extract undo action
    auto undoAction = std::move(mStack.back());
    mStack.pop_back();

    // Update total cost
    assert(mTotalCost >= undoAction->Cost);
    mTotalCost -= undoAction->Cost;

    // Execute action
    // Note: will make model dirty, temporarily
    undoAction->ApplyAndConsume(controller);

    // Restore dirty state
    // Note: undoes previous model dirti-zation
    controller.RestoreDirtyState(std::move(undoAction->OriginalDirtyState));
}

void UndoStack::RewindAndApply(
    size_t startIndex,
    Controller & controller)
{
    while (startIndex < mStack.size() - 1)
    {
        PopAndApply(controller);
    }
}

}