/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "Controller.h"

namespace ShipBuilder {

template<typename TLayerBuffer>
void LayerBufferRegionUndoAction<TLayerBuffer>::Apply(Controller & controller) const
{
    controller.RestoreLayerBufferRegion(mLayerBufferRegion, mOrigin);
}

//
// Explicit specializations
//

template class LayerBufferRegionUndoAction<StructuralLayerBuffer>;
template class LayerBufferRegionUndoAction<ElectricalLayerBuffer>;

}