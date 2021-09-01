/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <GameCore/GameTypes.h>
#include <Game/MaterialDatabase.h>
#include <Game/ShipTexturizer.h>

namespace ShipBuilder {

template<MaterialLayerType TMaterialLayerType>
class MaterialPalette
{
public:

    MaterialPalette(
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer);

private:
};

}