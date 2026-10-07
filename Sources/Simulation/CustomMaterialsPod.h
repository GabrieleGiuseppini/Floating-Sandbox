/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-10-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Layers.h"

#include <memory>
#include <vector>

/*
 * Container to maintain lifetime of custom materials.
 */

class CustomMaterialsPod
{
public:

    CustomMaterialsPod()
        : mCustomStructuralLayerMaterials()
        , mCustomElectricalLayerMaterials()
        , mCustomRopesLayerMaterials()
    {
    }

    CustomMaterialsPod(
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type>> && customStructuralLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type>> && customElectricalLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type>> && customRopesLayerMaterials)
        : mCustomStructuralLayerMaterials(std::move(customStructuralLayerMaterials))
        , mCustomElectricalLayerMaterials(std::move(customElectricalLayerMaterials))
        , mCustomRopesLayerMaterials(std::move(customRopesLayerMaterials))
    { }

private:
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type>> mCustomStructuralLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type>> mCustomElectricalLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type>> mCustomRopesLayerMaterials;
};