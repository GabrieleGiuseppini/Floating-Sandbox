/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>

#include <cstdint>

namespace ShipBuilder {

/*
 * This class aggregates the current state of the shipbuilder work tools.
 * It is completely independent from the model - is holds exclusively
 * tools-related settings, so ther is no need to reset/change/update
 * at creation of new models.
 */
class WorkbenchState
{
public:

    WorkbenchState(MaterialDatabase const & materialDatabase);

    StructuralMaterial const * GetStructuralForegroundMaterial() const
    {
        return mStructuralForegroundMaterial;
    }

    void SetStructuralForegroundMaterial(StructuralMaterial const * material)
    {
        mStructuralForegroundMaterial = material;
    }

    StructuralMaterial const * GetStructuralBackgroundMaterial() const
    {
        return mStructuralBackgroundMaterial;
    }

    void SetStructuralBackgroundMaterial(StructuralMaterial const * material)
    {
        mStructuralBackgroundMaterial = material;
    }

    ElectricalMaterial const * GetElectricalForegroundMaterial() const
    {
        return mElectricalForegroundMaterial;
    }

    void SetElectricalForegroundMaterial(ElectricalMaterial const * material)
    {
        mElectricalForegroundMaterial = material;
    }

    ElectricalMaterial const * GetElectricalBackgroundMaterial() const
    {
        return mElectricalBackgroundMaterial;
    }

    void SetElectricalBackgroundMaterial(ElectricalMaterial const * material)
    {
        mElectricalBackgroundMaterial = material;
    }

    std::uint32_t GetStructuralPencilToolSize() const
    {
        return mStructuralPencilToolSize;
    }

    void SetStructuralPencilToolSize(std::uint32_t value)
    {
        mStructuralPencilToolSize = value;
    }

    std::uint32_t GetStructuralEraserToolSize() const
    {
        return mStructuralEraserToolSize;
    }

    void SetStructuralEraserToolSize(std::uint32_t value)
    {
        mStructuralEraserToolSize = value;
    }

    bool GetStructuralFloodToolIsContiguous() const
    {
        return mStructuralFloodToolIsContiguous;
    }

    void SetStructuralFloodToolIsContiguous(bool value)
    {
        mStructuralFloodToolIsContiguous = value;
    }

private:

    StructuralMaterial const * mStructuralForegroundMaterial;
    StructuralMaterial const * mStructuralBackgroundMaterial;
    ElectricalMaterial const * mElectricalForegroundMaterial;
    ElectricalMaterial const * mElectricalBackgroundMaterial;

    std::uint32_t mStructuralPencilToolSize;
    std::uint32_t mStructuralEraserToolSize;
    bool mStructuralFloodToolIsContiguous;
};

}