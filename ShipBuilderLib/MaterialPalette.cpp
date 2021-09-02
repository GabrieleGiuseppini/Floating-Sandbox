/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MaterialPalette.h"

namespace ShipBuilder {

template<MaterialLayerType TMaterialLayerType>
MaterialPalette<TMaterialLayerType>::MaterialPalette(
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer)
    : mCurrentPlaneType()
{
    // TODO
}

template<MaterialLayerType TMaterialLayerType>
void MaterialPalette<TMaterialLayerType>::Open(
    wxPoint const & position,
    wxRect const & referenceArea,
    MaterialPlaneType planeType,
    Material const * initialMaterial)
{
    // tODOHERE
}

//
// Explicit specializations for all material layers
//

template class MaterialPalette<MaterialLayerType::Structural>;
template class MaterialPalette<MaterialLayerType::Electrical>;

}