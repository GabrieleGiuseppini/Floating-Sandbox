/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/Materials.h>

#include <GameCore/Buffer2D.h>

#include <memory>

namespace ShipBuilder {

// Forward declarations
class ModelController;

/*
 * Base class of the hierarchy of undo actions.
 *
 * Some examples of specializations include:
 * - Material region replace
 * - Resize
 */
class UndoEditAction
{
public:

    virtual ~UndoEditAction() = default;

    virtual void Apply(ModelController & modelController) const = 0;
};

template<typename TMaterial>
class MaterialRegionUndoEditAction final : public UndoEditAction
{
public:

    MaterialRegionUndoEditAction(
        std::unique_ptr<MaterialBuffer<TMaterial>> && region,
        WorkSpaceCoordinates const & origin)
        : mRegion(std::move(region))
        , mOrigin(origin)
    {}

    void Apply(ModelController & modelController) const override;

private:

    std::unique_ptr<MaterialBuffer<TMaterial>> mRegion;
    WorkSpaceCoordinates mOrigin;
};

/*
 * This object wraps the information to Undo and Redo individual edit actions.
 */
class UndoEntry final
{
public:

    UndoEntry(
        std::unique_ptr<UndoEditAction> undoEditAction,
        std::unique_ptr<UndoEditAction> redoEditAction)
        : mUndoEditAction(std::move(undoEditAction))
        , mRedoEditAction(std::move(redoEditAction))
    {}

private:

    std::unique_ptr<UndoEditAction> mUndoEditAction;
    std::unique_ptr<UndoEditAction> mRedoEditAction;
};

class UndoStack
{
    // TODO
};

}