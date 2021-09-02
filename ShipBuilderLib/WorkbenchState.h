/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>

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

private:

    StructuralMaterial const * mStructuralForegroundMaterial;
    StructuralMaterial const * mStructuralBackgroundMaterial;
    ElectricalMaterial const * mElectricalForegroundMaterial;
    ElectricalMaterial const * mElectricalBackgroundMaterial;

};

}