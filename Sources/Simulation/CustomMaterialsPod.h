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

    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type const>> CustomStructuralLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type const>> CustomElectricalLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type const>> CustomRopesLayerMaterials;

    CustomMaterialsPod()
        : CustomStructuralLayerMaterials()
        , CustomElectricalLayerMaterials()
        , CustomRopesLayerMaterials()
    {
    }

    CustomMaterialsPod(
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type const>> && customStructuralLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type const>> && customElectricalLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type const>> && customRopesLayerMaterials)
        : CustomStructuralLayerMaterials(std::move(customStructuralLayerMaterials))
        , CustomElectricalLayerMaterials(std::move(customElectricalLayerMaterials))
        , CustomRopesLayerMaterials(std::move(customRopesLayerMaterials))
    { }

    bool IsEmpty() const
    {
        return CustomStructuralLayerMaterials.empty()
            && CustomElectricalLayerMaterials.empty()
            && CustomRopesLayerMaterials.empty();
    }
};