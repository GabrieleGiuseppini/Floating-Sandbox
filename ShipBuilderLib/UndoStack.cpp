/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "Controller.h"

#include <GameCore/Log.h>

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
    undoAction->ApplyAndConsume(controller);
    controller.RestoreDirtyState(std::move(undoAction->OriginalDirtyState));
}

}