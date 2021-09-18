/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/Materials.h>

#include <memory>

namespace ShipBuilder {

// Forward declarations
class ModelController;
class UndoEntry;

/*
 * Base class of hierarchy of edit actions.
 */
class EditAction
{
public:

    virtual ~EditAction() = default;

    virtual std::unique_ptr<UndoEntry> Apply(ModelController & modelController) const = 0;
};

template<typename TMaterial>
class MaterialRegionFillEditAction final : public EditAction
{
public:

    MaterialRegionFillEditAction(
        TMaterial const * material,
        WorkSpaceCoordinates const & origin,
        WorkSpaceSize const & size)
        : mMaterial(material)
        , mOrigin(origin)
        , mSize(size)
    {}

    MaterialRegionFillEditAction(MaterialRegionFillEditAction const & other) = default;

    std::unique_ptr<UndoEntry> Apply(ModelController & modelController) const override;

private:

    TMaterial const * mMaterial;
    WorkSpaceCoordinates const mOrigin;
    WorkSpaceSize const mSize;
};

}