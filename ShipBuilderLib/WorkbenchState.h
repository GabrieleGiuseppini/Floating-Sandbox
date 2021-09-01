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

    StructuralMaterial const * GetForegroundStructuralMaterial() const
    {
        return mForegroundStructuralMaterial;
    }

    void SetForegroundStructuralMaterial(StructuralMaterial const * material)
    {
        mForegroundStructuralMaterial = material;
    }

    StructuralMaterial const * GetBackgroundStructuralMaterial() const
    {
        return mBackgroundStructuralMaterial;
    }

    void SetBackgroundStructuralMaterial(StructuralMaterial const * material)
    {
        mBackgroundStructuralMaterial = material;
    }

    ElectricalMaterial const * GetForegroundElectricalMaterial() const
    {
        return mForegroundElectricalMaterial;
    }

    void SetForegroundElectricalMaterial(ElectricalMaterial const * material)
    {
        mForegroundElectricalMaterial = material;
    }

    ElectricalMaterial const * GetBackgroundElectricalMaterial() const
    {
        return mBackgroundElectricalMaterial;
    }

    void SetBackgroundElectricalMaterial(ElectricalMaterial const * material)
    {
        mBackgroundElectricalMaterial = material;
    }

private:

    StructuralMaterial const * mForegroundStructuralMaterial;
    StructuralMaterial const * mBackgroundStructuralMaterial;
    ElectricalMaterial const * mForegroundElectricalMaterial;
    ElectricalMaterial const * mBackgroundElectricalMaterial;

};

}