/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "EditAction.h"

#include "ModelController.h"
#include "UndoStack.h"

namespace ShipBuilder {

template<typename TMaterial>
std::unique_ptr<UndoEntry> MaterialRegionFillEditAction<TMaterial>::Apply(ModelController & modelController) const
{
    std::unique_ptr<EditAction> undoEditAction;
    if constexpr (TMaterial::Layer == MaterialLayerType::Structural)
    {
        undoEditAction = modelController.StructuralRegionFill(mMaterial, mOrigin, mSize);
    }
    else
    {
        static_assert(TMaterial::Layer == MaterialLayerType::Electrical);

        undoEditAction = modelController.ElectricalRegionFill(mMaterial, mOrigin, mSize);
    }

    return std::make_unique<UndoEntry>(
        std::move(undoEditAction),
        std::make_unique<MaterialRegionFillEditAction>(*this));
}

//
// Explicit specializations
//

template class MaterialRegionFillEditAction<StructuralMaterial>;
template class MaterialRegionFillEditAction<ElectricalMaterial>;

}