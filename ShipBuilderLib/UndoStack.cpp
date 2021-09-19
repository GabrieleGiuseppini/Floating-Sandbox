/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "ModelController.h"

#include <type_traits>

namespace ShipBuilder {

template<typename TMaterial>
void MaterialRegionUndoEditAction<TMaterial>::Apply(ModelController & modelController) const
{
    if constexpr (std::is_same<StructuralMaterial, TMaterial>())
    {
        modelController.StructuralRegionReplace(
            *mRegion,
            mOrigin);
    }
    else
    {
        static_assert(std::is_same<ElectricalMaterial, TMaterial>());

        modelController.ElectricalRegionReplace(
            *mRegion,
            mOrigin);
    }
}

// All template specializations
template class MaterialRegionUndoEditAction<StructuralMaterial>;
template class MaterialRegionUndoEditAction<ElectricalMaterial>;

}