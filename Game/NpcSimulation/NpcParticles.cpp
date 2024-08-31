/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>

namespace Physics {

ElementIndex NpcParticles::Add(
    float mass,
    float buoyancyVolumeFill,
    float buoyancyFactor,
    StructuralMaterial const * material,
    vec2f const & position,
    rgbaColor const & color)
{
    // Find first free particle
    ElementIndex const p = FindFreeParticleIndex();

    mIsInUseBuffer[p] = true;

    mMaterialBuffer[p] = material;
    mBuoyancyVolumeFillBuffer[p] = buoyancyVolumeFill;
    mMassBuffer[p] = mass;
    mBuoyancyFactorBuffer[p] = buoyancyFactor;
    mPositionBuffer[p] = position;
    mVelocityBuffer[p] = vec2f::zero();
    mPreliminaryForcesBuffer[p] = vec2f::zero();
    mExternalForcesBuffer[p] = vec2f::zero();

    mMeshWaternessBuffer[p] = 0.0f;
    mMeshWaterVelocityBuffer[p] = vec2f::zero();
    mAnyWaternessBuffer[p] = 0.0f;

    mRenderColorBuffer[p] = color;

    ++mParticleInUseCount;

    return p;
}

void NpcParticles::Remove(
    ElementIndex particleIndex)
{
    assert(mIsInUseBuffer[particleIndex]);

    mIsInUseBuffer[particleIndex] = false;
    mFreeParticleSearchStartIndex = std::min(mFreeParticleSearchStartIndex, particleIndex);

    --mParticleInUseCount;
}

void NpcParticles::Query(ElementIndex particleElementIndex) const
{
    LogMessage("ParticleIndex: ", particleElementIndex);
    LogMessage("P=", mPositionBuffer[particleElementIndex].toString(), " V=", mVelocityBuffer[particleElementIndex].toString());
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
    throw GameException("Cannot find free NPC particle");
}

}