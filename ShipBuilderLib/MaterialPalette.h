/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <GameCore/GameTypes.h>
#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>
#include <Game/ShipTexturizer.h>

#include <wx/wx.h>

#include <optional>

namespace ShipBuilder {

template<MaterialLayerType TMaterialLayerType>
class MaterialPalette
{
public:

    MaterialPalette(
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer);

    void Open(
        wxPoint const & position,
        wxRect const & referenceArea,
        MaterialPlaneType planeType,
        // TODOHERE: should be templated on this - but then do we need the "layer type" template?
        Material const * initialMaterial);

private:

    std::optional<MaterialPlaneType> mCurrentPlaneType;
};

}