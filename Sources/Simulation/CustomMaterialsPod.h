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
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type const>> && customStructuralLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type const>> && customElectricalLayerMaterials,
        std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type const>> && customRopesLayerMaterials)
        : mCustomStructuralLayerMaterials(std::move(customStructuralLayerMaterials))
        , mCustomElectricalLayerMaterials(std::move(customElectricalLayerMaterials))
        , mCustomRopesLayerMaterials(std::move(customRopesLayerMaterials))
    { }

    bool IsEmpty() const
    {
        return mCustomStructuralLayerMaterials.empty()
            && mCustomElectricalLayerMaterials.empty()
            && mCustomRopesLayerMaterials.empty();
    }

private:
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Structural>::material_type const>> mCustomStructuralLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Electrical>::material_type const>> mCustomElectricalLayerMaterials;
    std::vector<std::unique_ptr<LayerTypeTraits<LayerType::Ropes>::material_type const>> mCustomRopesLayerMaterials;
};