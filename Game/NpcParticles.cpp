/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NpcParticles.h"

#include <stdexcept>

namespace Physics
{

ElementIndex NpcParticles::Add(
    vec2f const & position,
    StructuralMaterial const & structuralMaterial)
{
    // Find first free particle
    ElementIndex const p = FindFreeParticleIndex();

    mIsInUseBuffer[p] = true;

    mMaterialBuffer[p] = &structuralMaterial;
    mPositionBuffer[p] = position;
    mVelocityBuffer[p] = vec2f::zero();
    mExternalForcesBuffer[p] = vec2f::zero();
    mMassBuffer[p] = structuralMaterial.GetMass();
    mMaterialBuoyancyVolumeFillBuffer[p] = structuralMaterial.BuoyancyVolumeFill;
    float const integrationFactor = GameParameters::SimulationStepTimeDuration<float> *GameParameters::SimulationStepTimeDuration<float> / structuralMaterial.GetMass();;
    mIntegrationFactorBuffer[p] = vec2f(integrationFactor, integrationFactor);

    ++mParticleInUseCount;
    
    // Remember we're dirty
    mAreElementsDirtyForRendering = true;

    return p;
}

void NpcParticles::Remove(
    ElementIndex particleIndex)
{
    assert(mIsInUseBuffer[particleIndex]);

    mIsInUseBuffer[particleIndex] = false;

    --mParticleInUseCount;

    // Remember we're dirty
    mAreElementsDirtyForRendering = true;
}

ElementIndex NpcParticles::FindFreeParticleIndex()
{
    for (ElementIndex p = mFreeParticleSearchStartIndex; ; /*incremented in loop*/)
    {
        if (!mIsInUseBuffer[p])
        {
            // Found!

            // Remember to start after this one next time
            mFreeParticleSearchStartIndex = p + 1;
            if (mFreeParticleSearchStartIndex >= mElementCount)
                mFreeParticleSearchStartIndex = 0;

            return p;
        }

        // Advance
        ++p;
        if (p >= mElementCount)
            p = 0;

        if (p == mFreeParticleSearchStartIndex)
        {
            // Went around
            break;
        }
    }

    // If we're here, no luck
    throw std::runtime_error("Cannot find free NPC particle");
}

}