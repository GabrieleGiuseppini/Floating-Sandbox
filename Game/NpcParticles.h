/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-07-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "Materials.h"

#include <GameCore/Buffer.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <optional>

namespace Physics
{

/*
 * Fixed-size container of NPC particles.
 *
 * Not all particles will be in use at all moments; particles are created and removed as NPCs are added and removed.
 */
class NpcParticles final : public ElementContainer
{
public:

    NpcParticles(
        ElementCount maxParticleCount)
        : ElementContainer(make_aligned_float_element_count(maxParticleCount))
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsInUseBuffer(maxParticleCount, false)
        // Material
        , mMaterialBuffer(maxParticleCount, nullptr)
        // Mechanical dynamics
        , mPositionBuffer(maxParticleCount, vec2f::zero())
        , mVelocityBuffer(maxParticleCount, vec2f::zero())
        , mExternalForcesBuffer(maxParticleCount, vec2f::zero())
        , mMassBuffer(maxParticleCount, 0.0f)
        , mMaterialBuoyancyVolumeFillBuffer(maxParticleCount, 0.0f)
        , mIntegrationFactorBuffer(maxParticleCount, vec2f::zero())
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParticleInUseCount(0)
        , mFreeParticleSearchStartIndex(0)
        , mAreElementsDirtyForRendering(false)
    {
    }

    NpcParticles(NpcParticles && other) = default;

    void Add(
        vec2f const & position,
        StructuralMaterial const & structuralMaterial);

    void Remove(
        ElementIndex particleIndex);

    //
    // Render
    //

public:

    //
    // Material
    //

    StructuralMaterial const & GetMaterial(ElementIndex particleIndex) const
    {
        return *(mMaterialBuffer[particleIndex]);
    }

    //
    // Dynamics
    //

    vec2f const & GetPosition(ElementIndex particleIndex) const noexcept
    {
        return mPositionBuffer[particleIndex];
    }

    vec2f * GetPositionBufferAsVec2()
    {
        return mPositionBuffer.data();
    }

    float * GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(mPositionBuffer.data());
    }

    void SetPosition(
        ElementIndex particleIndex,
        vec2f const & position) noexcept
    {
        mPositionBuffer[particleIndex] = position;
    }

    vec2f const & GetVelocity(ElementIndex particleIndex) const noexcept
    {
        return mVelocityBuffer[particleIndex];
    }

    vec2f * GetVelocityBufferAsVec2()
    {
        return mVelocityBuffer.data();
    }

    float * GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(mVelocityBuffer.data());
    }

    void SetVelocity(
        ElementIndex particleIndex,
        vec2f const & velocity) noexcept
    {
        mVelocityBuffer[particleIndex] = velocity;
    }

private:

    ElementIndex FindFreeParticleIndex();

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // In use: true when the particle is occupied
    Buffer<bool> mIsInUseBuffer;

    //
    // Material
    //

    Buffer<StructuralMaterial const *> mMaterialBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mExternalForcesBuffer; // Forces applied from outside
    Buffer<float> mMassBuffer;
    Buffer<float> mMaterialBuoyancyVolumeFillBuffer;
    Buffer<vec2f> mIntegrationFactorBuffer; // TODO: see if needed

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Convenience counter
    ElementCount mParticleInUseCount;
    
    // The index at which to start searching for free particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeParticleSearchStartIndex;

    // Set when the "series" of particles has changed (e.g. particle addition or removal)
    bool mAreElementsDirtyForRendering;
};

}
