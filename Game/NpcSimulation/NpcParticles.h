/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../GameParameters.h"
#include "../Materials.h"

#include <GameCore/Buffer.h>
#include <GameCore/Colors.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/ElementIndexRangeIterator.h>
#include <GameCore/GameTypes.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <optional>

namespace Physics {

class NpcParticles final : public ElementContainer
{
public:

    NpcParticles(ElementCount maxParticleCount)
        : ElementContainer(make_aligned_float_element_count(maxParticleCount))
        , mMaxParticleCount(maxParticleCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsInUseBuffer(maxParticleCount, false)
        // Physics
        , mMaterialBuffer(maxParticleCount, nullptr)
        , mBuoyancyVolumeFillBuffer(maxParticleCount, 0.0f)
        , mMassBuffer(maxParticleCount, 0.0f)
        , mBuoyancyFactorBuffer(maxParticleCount, 0.0f)
        , mStaticFrictionTotalAdjustmentBuffer(maxParticleCount, 1.0f)
        , mKineticFrictionTotalAdjustmentBuffer(maxParticleCount, 1.0f)
        , mPositionBuffer(maxParticleCount, vec2f::zero())
        , mVelocityBuffer(maxParticleCount, vec2f::zero())
        , mPreliminaryForcesBuffer(maxParticleCount, vec2f::zero())
        , mExternalForcesBuffer(maxParticleCount, vec2f::zero())
        , mTemperatureBuffer(maxParticleCount, 0.0f)
        , mMeshWaternessBuffer(maxParticleCount, 0.0f)
        , mMeshWaterVelocityBuffer(maxParticleCount, vec2f::zero())
        , mAnyWaternessBuffer(maxParticleCount, 0.0f)
        , mRandomNormalizedUniformFloatBuffer(maxParticleCount, 0.0f)
        , mLightBuffer(maxParticleCount, 0.0f)
        // Render
        , mRenderColorBuffer(maxParticleCount, rgbaColor::zero())
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParticlesInUseCount(0)
        , mFreeParticleSearchStartIndex(0)
    {
    }

    NpcParticles(NpcParticles && other) = default;

    ElementCount GetInUseParticlesCount() const
    {
        return mParticlesInUseCount;
    }

    ElementCount GetRemainingParticlesCount() const
    {
        assert(mParticlesInUseCount <= mMaxParticleCount);
        return mMaxParticleCount - mParticlesInUseCount;
    }

    ElementIndex Add(
        float mass,
        float buoyancyVolumeFill,
        float buoyancyFactor,
        StructuralMaterial const * material,
        float staticFrictionTotalAdjustment,
        float kineticFrictionTotalAdjustment,
        vec2f const & position,
        rgbaColor const & color);

    void Remove(
        ElementIndex particleIndex);

    void Query(ElementIndex particleElementIndex) const;

public:

    //
    // Physics
    //

    StructuralMaterial const & GetMaterial(ElementIndex particleElementIndex) const noexcept
    {
        assert(mMaterialBuffer[particleElementIndex] != nullptr);
        return *(mMaterialBuffer[particleElementIndex]);
    }

    float GetBuoyancyVolumeFill(ElementIndex particleElementIndex) const noexcept
    {
        return mBuoyancyVolumeFillBuffer[particleElementIndex];
    }

    float const GetMass(ElementIndex particleElementIndex) const noexcept
    {
        return mMassBuffer[particleElementIndex];
    }

    void SetMass(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mMassBuffer[particleElementIndex] = value;
    }

    float const GetBuoyancyFactor(ElementIndex particleElementIndex) const noexcept
    {
        return mBuoyancyFactorBuffer[particleElementIndex];
    }

    void SetBuoyancyFactor(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mBuoyancyFactorBuffer[particleElementIndex] = value;
    }

    float const GetStaticFrictionTotalAdjustment(ElementIndex particleElementIndex) const noexcept
    {
        return mStaticFrictionTotalAdjustmentBuffer[particleElementIndex];
    }

    void SetStaticFrictionTotalAdjustment(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mStaticFrictionTotalAdjustmentBuffer[particleElementIndex] = value;
    }

    float const GetKineticFrictionTotalAdjustment(ElementIndex particleElementIndex) const noexcept
    {
        return mKineticFrictionTotalAdjustmentBuffer[particleElementIndex];
    }

    void SetKineticFrictionTotalAdjustment(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mKineticFrictionTotalAdjustmentBuffer[particleElementIndex] = value;
    }

    vec2f const & GetPosition(ElementIndex particleElementIndex) const noexcept
    {
        return mPositionBuffer[particleElementIndex];
    }

    vec2f & GetPosition(ElementIndex particleElementIndex) noexcept
    {
        return mPositionBuffer[particleElementIndex];
    }

    vec2f const * GetPositionBuffer() const noexcept
    {
        return mPositionBuffer.data();
    }

    vec2f * GetPositionBuffer() noexcept
    {
        return mPositionBuffer.data();
    }

    void SetPosition(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mPositionBuffer[particleElementIndex] = value;
    }

    vec2f const & GetVelocity(ElementIndex particleElementIndex) const noexcept
    {
        return mVelocityBuffer[particleElementIndex];
    }

    vec2f & GetVelocity(ElementIndex particleElementIndex) noexcept
    {
        return mVelocityBuffer[particleElementIndex];
    }

    vec2f const * GetVelocityBuffer() const noexcept
    {
        return mVelocityBuffer.data();
    }

    vec2f * GetVelocityBuffer() noexcept
    {
        return mVelocityBuffer.data();
    }

    void SetVelocity(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mVelocityBuffer[particleElementIndex] = value;
    }

    vec2f const & GetPreliminaryForces(ElementIndex particleElementIndex) const noexcept
    {
        return mPreliminaryForcesBuffer[particleElementIndex];
    }

    vec2f * GetPreliminaryForces() noexcept
    {
        return mPreliminaryForcesBuffer.data();
    }

    void SetPreliminaryForces(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mPreliminaryForcesBuffer[particleElementIndex] = value;
    }

    vec2f const & GetExternalForces(ElementIndex particleElementIndex) const noexcept
    {
        return mExternalForcesBuffer[particleElementIndex];
    }

    vec2f * GetExternalForces() noexcept
    {
        return mExternalForcesBuffer.data();
    }

    void SetExternalForces(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mExternalForcesBuffer[particleElementIndex] = value;
    }

    void AddExternalForce(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mExternalForcesBuffer[particleElementIndex] += value;
    }

    void ResetExternalForces()
    {
        mExternalForcesBuffer.fill(vec2f::zero());
    }

    float GetTemperature(ElementIndex particleElementIndex) const noexcept
    {
        return mTemperatureBuffer[particleElementIndex];
    }

    void SetTemperature(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mTemperatureBuffer[particleElementIndex] = value;
    }

    void AddHeat(
        ElementIndex particleElementIndex,
        float heat) // J
    {
        assert(GetMaterial(particleElementIndex).GetHeatCapacity() > 0.0f);

        mTemperatureBuffer[particleElementIndex] +=
            heat
            / GetMaterial(particleElementIndex).GetHeatCapacity();
    }

    // [0.0, ~1.0]
    float const GetMeshWaterness(ElementIndex particleElementIndex) const noexcept
    {
        return mMeshWaternessBuffer[particleElementIndex];
    }

    void SetMeshWaterness(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mMeshWaternessBuffer[particleElementIndex] = value;
    }

    vec2f const & GetMeshWaterVelocity(ElementIndex particleElementIndex) const noexcept
    {
        return mMeshWaterVelocityBuffer[particleElementIndex];
    }

    void SetMeshWaterVelocity(
        ElementIndex particleElementIndex,
        vec2f const & value) noexcept
    {
        mMeshWaterVelocityBuffer[particleElementIndex] = value;
    }

    float const GetAnyWaterness(ElementIndex particleElementIndex) const noexcept
    {
        return mAnyWaternessBuffer[particleElementIndex];
    }

    void SetAnyWaterness(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mAnyWaternessBuffer[particleElementIndex] = value;
    }

    float GetLight(ElementIndex pointElementIndex) const
    {
        return mLightBuffer[pointElementIndex];
    }

    void SetLight(
        ElementIndex particleElementIndex,
        float value) noexcept
    {
        mLightBuffer[particleElementIndex] = value;
    }

    float GetRandomNormalizedUniformPersonalitySeed(ElementIndex pointElementIndex) const
    {
        return mRandomNormalizedUniformFloatBuffer[pointElementIndex];
    }

    //
    // Render
    //

    rgbaColor const & GetRenderColor(ElementIndex particleElementIndex) const
    {
        return mRenderColorBuffer[particleElementIndex];
    }

    void SetRenderColor(
        ElementIndex particleElementIndex,
        rgbaColor const & color)
    {
        mRenderColorBuffer[particleElementIndex] = color;
    }

    rgbaColor const * GetRenderColorBuffer() const
    {
        return mRenderColorBuffer.data();
    }

private:

    ElementIndex FindFreeParticleIndex();

private:

    ElementCount const mMaxParticleCount;

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // In use: true when the particle is occupied
    Buffer<bool> mIsInUseBuffer;

    //
    // Physics
    //

    Buffer<StructuralMaterial const *> mMaterialBuffer;
    Buffer<float> mBuoyancyVolumeFillBuffer;
    Buffer<float> mMassBuffer; // Adjusted
    Buffer<float> mBuoyancyFactorBuffer; // Adjusted
    Buffer<float> mStaticFrictionTotalAdjustmentBuffer;
    Buffer<float> mKineticFrictionTotalAdjustmentBuffer;
    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mPreliminaryForcesBuffer;
    Buffer<vec2f> mExternalForcesBuffer;

    Buffer<float> mTemperatureBuffer;

    Buffer<float> mMeshWaternessBuffer; // Mesh water at triangle (when constrained); // [0.0, ~1.0]
    Buffer<vec2f> mMeshWaterVelocityBuffer; // (when constrained)
    Buffer<float> mAnyWaternessBuffer; // Mesh water at triangle (when constrained), depth (when free); [0.0, 1.0]

    Buffer<float> mLightBuffer;

    Buffer<float> mRandomNormalizedUniformFloatBuffer;

    //
    // Render
    //

    Buffer<rgbaColor> mRenderColorBuffer;

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Convenience counter
    ElementCount mParticlesInUseCount;

    // The index at which to start searching for free particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeParticleSearchStartIndex;
};

}