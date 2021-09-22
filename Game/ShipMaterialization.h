/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "LayerBuffers.h"

#include <memory>

struct ShipMaterialization
{
    std::unique_ptr<StructuralLayerBuffer> StructuralLayer;
    std::unique_ptr<ElectricalLayerBuffer> ElectricalLayer;
    std::unique_ptr<RopesLayerBuffer> RopesLayer;
    std::unique_ptr<TextureLayerBuffer> TextureLayer;
};
