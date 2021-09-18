/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EditAction.h"
#include "ShipBuilderTypes.h"

#include <memory>

namespace ShipBuilder {

/*
 * This object wraps the information to Undo and Redo individual edit actions.
 */
class UndoEntry
{
public:

    UndoEntry(
        std::unique_ptr<EditAction> undoEditAction,
        std::unique_ptr<EditAction> redoEditAction)
        : mUndoEditAction(std::move(undoEditAction))
        , mRedoEditAction(std::move(redoEditAction))
    {}

private:

    std::unique_ptr<EditAction> mUndoEditAction;
    std::unique_ptr<EditAction> mRedoEditAction;
};

class UndoStack
{
    // TODO
};

}