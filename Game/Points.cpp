/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>
#include <GameCore/Log.h>

#include <cmath>
#include <limits>

namespace Physics {

void Points::Add(
    vec2f const & position,
    StructuralMaterial const & structuralMaterial,
    ElectricalMaterial const * electricalMaterial,
    bool isRope,
    ElementIndex electricalElementIndex,
    bool isLeaking,
    vec4f const & color,
    vec2f const & textureCoordinates)
{
    ElementIndex const pointIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);

    mMaterialsBuffer.emplace_back(&structuralMaterial, electricalMaterial);
    mIsRopeBuffer.emplace_back(isRope);

    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());
    mForceBuffer.emplace_back(vec2f::zero());
    mMassBuffer.emplace_back(structuralMaterial.Mass);
    mIntegrationFactorTimeCoefficientBuffer.emplace_back(CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations));

    // These will be recalculated each time
    mTotalMassBuffer.emplace_back(0.0f);
    mIntegrationFactorBuffer.emplace_back(vec2f::zero());
    mForceRenderBuffer.emplace_back(vec2f::zero());

    mIsHullBuffer.emplace_back(structuralMaterial.IsHull);
    mWaterVolumeFillBuffer.emplace_back(structuralMaterial.WaterVolumeFill);
    mWaterIntakeBuffer.emplace_back(structuralMaterial.WaterIntake);
    mWaterRestitutionBuffer.emplace_back(1.0f - structuralMaterial.WaterRetention);
    mWaterDiffusionSpeedBuffer.emplace_back(structuralMaterial.WaterDiffusionSpeed);

    mWaterBuffer.emplace_back(0.0f);
    mWaterVelocityBuffer.emplace_back(vec2f::zero());
    mWaterMomentumBuffer.emplace_back(vec2f::zero());
    mCumulatedIntakenWater.emplace_back(0.0f);
    mIsLeakingBuffer.emplace_back(isLeaking);
    if (isLeaking)
        SetLeaking(pointIndex);

    // Electrical dynamics
    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    // Wind dynamics
    mWindReceptivityBuffer.emplace_back(structuralMaterial.WindReceptivity);

    // Ephemeral particles
    mEphemeralTypeBuffer.emplace_back(EphemeralType::None);
    mEphemeralStartTimeBuffer.emplace_back(0.0f);
    mEphemeralMaxLifetimeBuffer.emplace_back(0.0f);
    mEphemeralStateBuffer.emplace_back(EphemeralState::DebrisState());

    // Structure
    mConnectedSpringsBuffer.emplace_back();
    mConnectedTrianglesBuffer.emplace_back();
    mConnectedOwnedTrianglesCountBuffer.emplace_back(0);

    mConnectedComponentIdBuffer.emplace_back(NoneConnectedComponentId);
    mPlaneIdBuffer.emplace_back(NonePlaneId);
    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();

    mIsPinnedBuffer.emplace_back(false);

    mColorBuffer.emplace_back(color);
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
}

void Points::CreateEphemeralParticleAirBubble(
    vec2f const & position,
    float initialSize,
    float vortexAmplitude,
    float vortexFrequency,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    PlaneId planeId)
{
    // Get a free slot (but don't steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, false);
    if (NoneElementIndex == pointIndex)
        return; // No luck

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = vec2f::zero();
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mWaterVolumeFillBuffer[pointIndex] = structuralMaterial.WaterVolumeFill;
    mWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mWindReceptivityBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::AirBubble;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::numeric_limits<float>::max();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::AirBubbleState(
        GameRandomEngine::GetInstance().Choose<TextureFrameIndex>(2),
        position.y,
        initialSize,
        vortexAmplitude,
        vortexFrequency);

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;

    // Remember we're dirty now
    mAreEphemeralParticlesDirty = true;
}

void Points::CreateEphemeralParticleDebris(
    vec2f const & position,
    vec2f const & velocity,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    mWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mWindReceptivityBuffer[pointIndex] = 3.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Debris;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::DebrisState();

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;

    // Remember we're dirty now
    mAreEphemeralParticlesDirty = true;
}

void Points::CreateEphemeralParticleSparkle(
    vec2f const & position,
    vec2f const & velocity,
    StructuralMaterial const & structuralMaterial,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    PlaneId planeId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime, true);
    assert(NoneElementIndex != pointIndex);

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mIntegrationFactorTimeCoefficientBuffer[pointIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    mMaterialsBuffer[pointIndex] = Materials(&structuralMaterial, nullptr);

    mWaterVolumeFillBuffer[pointIndex] = 0.0f; // No buoyancy
    mWaterIntakeBuffer[pointIndex] = structuralMaterial.WaterIntake;
    mWaterRestitutionBuffer[pointIndex] = 1.0f - structuralMaterial.WaterRetention;
    mWaterDiffusionSpeedBuffer[pointIndex] = structuralMaterial.WaterDiffusionSpeed;
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mWindReceptivityBuffer[pointIndex] = 3.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Sparkle;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::SparkleState(
        GameRandomEngine::GetInstance().Choose<TextureFrameIndex>(2));

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    // Remember we're dirty now
    mAreEphemeralParticlesDirty = true;
}

void Points::Destroy(
    ElementIndex pointElementIndex,
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    assert(pointElementIndex < mElementCount);
    assert(!IsDeleted(pointElementIndex));

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(
            pointElementIndex,
            currentSimulationTime,
            gameParameters);
    }

    // Fire point destroy event
    mGameEventHandler->OnDestroy(
        GetStructuralMaterial(pointElementIndex),
        mParentWorld.IsUnderwater(GetPosition(pointElementIndex)),
        1u);

    // Flag ourselves as deleted
    mIsDeletedBuffer[pointElementIndex] = true;

    // Let the physical world forget about us
    mPositionBuffer[pointElementIndex] = vec2f::zero();
    mVelocityBuffer[pointElementIndex] = vec2f::zero();
    mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = 0.0f;
    mWaterVelocityBuffer[pointElementIndex] = vec2f::zero();
    mWaterMomentumBuffer[pointElementIndex] = vec2f::zero();
}

void Points::UpdateGameParameters(GameParameters const & gameParameters)
{
    float const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
    if (numMechanicalDynamicsIterations != mCurrentNumMechanicalDynamicsIterations)
    {
        // Recalc integration factor time coefficients
        for (ElementIndex i : *this)
        {
            if (!IsDeleted(i))
            {
                mIntegrationFactorTimeCoefficientBuffer[i] = CalculateIntegrationFactorTimeCoefficient(numMechanicalDynamicsIterations);
            }
            else
            {
                assert(mIntegrationFactorTimeCoefficientBuffer[i] = 0.0f);
            }
        }

        // Remember the new values
        mCurrentNumMechanicalDynamicsIterations = numMechanicalDynamicsIterations;
    }
}

void Points::UpdateEphemeralParticles(
    float currentSimulationTime,
    GameParameters const & /*gameParameters*/)
{
    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        auto const ephemeralType = GetEphemeralType(pointIndex);
        if (EphemeralType::None != ephemeralType)
        {
            //
            // Run this particle's state machine
            //

            switch (ephemeralType)
            {
                case EphemeralType::AirBubble:
                {
                    float const waterHeight = mParentWorld.GetWaterHeightAt(GetPosition(pointIndex).x);
                    float const deltaY = waterHeight - GetPosition(pointIndex).y;

                    if (deltaY <= 0.0f)
                    {
                        // Expire
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        //
                        // Update progress based off remaining y
                        //

                        mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY = deltaY;

                        mEphemeralStateBuffer[pointIndex].AirBubble.Progress = 1.0f -
                            deltaY
                            / (waterHeight - mEphemeralStateBuffer[pointIndex].AirBubble.InitialY);

                        //
                        // Update vortex
                        //

                        float const lifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];

                        float const vortexAmplitude =
                            mEphemeralStateBuffer[pointIndex].AirBubble.VortexAmplitude
                            + mEphemeralStateBuffer[pointIndex].AirBubble.Progress;

                        float vortexValue =
                            vortexAmplitude
                            * sin(lifetime * mEphemeralStateBuffer[pointIndex].AirBubble.VortexFrequency);

                        // Update position
                        mPositionBuffer[pointIndex].x +=
                            vortexValue - mEphemeralStateBuffer[pointIndex].AirBubble.LastVortexValue;

                        mEphemeralStateBuffer[pointIndex].AirBubble.LastVortexValue = vortexValue;
                    }

                    break;
                }

                case EphemeralType::Debris:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];
                    if (elapsedLifetime >= mEphemeralMaxLifetimeBuffer[pointIndex])
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update alpha based off remaining time

                        float alpha = std::max(
                            1.0f - elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex],
                            0.0f);

                        mColorBuffer[pointIndex].w = alpha;
                    }

                    break;
                }

                case EphemeralType::Sparkle:
                {
                    // Check if expired
                    auto const elapsedLifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];
                    if (elapsedLifetime >= mEphemeralMaxLifetimeBuffer[pointIndex])
                    {
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        // Update progress based off remaining time

                        mEphemeralStateBuffer[pointIndex].Sparkle.Progress =
                            elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex];
                    }

                    break;
                }

                default:
                {
                    // Do nothing
                }
            }
        }
    }
}

void Points::Query(ElementIndex pointElementIndex) const
{
    LogMessage("PointIndex: ", pointElementIndex);
    LogMessage("Position: ", mPositionBuffer[pointElementIndex].toString());
    LogMessage("Velocity: ", mVelocityBuffer[pointElementIndex].toString());
    LogMessage("Water: ", mWaterBuffer[pointElementIndex]);
    LogMessage("PlaneID: ", mPlaneIdBuffer[pointElementIndex]);
    LogMessage("ConnectedComponentID: ", mConnectedComponentIdBuffer[pointElementIndex]);
}

void Points::UploadPlaneIds(
    ShipId shipId,
    PlaneId maxMaxPlaneId,
    Render::RenderContext & renderContext) const
{
    if (mIsPlaneIdBufferNonEphemeralDirty)
    {
        if (mIsPlaneIdBufferEphemeralDirty)
        {
            // Whole

            renderContext.UploadShipPointPlaneIds(
                shipId,
                mPlaneIdBuffer.data(),
                0,
                mAllPointCount,
                maxMaxPlaneId);

            mIsPlaneIdBufferEphemeralDirty = false;
        }
        else
        {
            // Just non-ephemeral portion

            renderContext.UploadShipPointPlaneIds(
                shipId,
                mPlaneIdBuffer.data(),
                0,
                mShipPointCount,
                maxMaxPlaneId);
        }

        mIsPlaneIdBufferNonEphemeralDirty = false;
    }
    else if (mIsPlaneIdBufferEphemeralDirty)
    {
        // Just ephemeral portion

        renderContext.UploadShipPointPlaneIds(
            shipId,
            &(mPlaneIdBuffer.data()[mShipPointCount]),
            mShipPointCount,
            mEphemeralPointCount,
            maxMaxPlaneId);

        mIsPlaneIdBufferEphemeralDirty = false;
    }
}

void Points::UploadMutableAttributes(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Upload immutable attributes, if we haven't uploaded them yet
    if (mIsColorBufferDirty
        || mIsTextureCoordinatesBufferDirty)
    {
        renderContext.UploadShipPointImmutableGraphicalAttributes(
            shipId,
            mColorBuffer.data(),
            mTextureCoordinatesBuffer.data());

        mIsColorBufferDirty = false;
        mIsTextureCoordinatesBufferDirty = false;
    }

    // Upload mutable attributes
    renderContext.UploadShipPoints(
        shipId,
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data());
}

void Points::UploadElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (ElementIndex pointIndex : NonEphemeralPoints())
    {
        if (!mIsDeletedBuffer[pointIndex])
        {
            renderContext.UploadShipElementPoint(
                shipId,
                pointIndex);
        }
    }
}

void Points::UploadVectors(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    static constexpr vec4f VectorColor(0.5f, 0.1f, 0.f, 1.0f);

    if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointVelocity)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdBuffer.data(),
            mVelocityBuffer.data(),
            0.25f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointForce)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdBuffer.data(),
            mForceRenderBuffer.data(),
            0.0005f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointWaterVelocity)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdBuffer.data(),
            mWaterVelocityBuffer.data(),
            1.0f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointWaterMomentum)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
            mPlaneIdBuffer.data(),
            mWaterMomentumBuffer.data(),
            0.4f,
            VectorColor);
    }
}

void Points::UploadEphemeralParticles(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    //
    // 1. Upload ephemeral-particle portion of point colors
    //

    renderContext.UploadShipPointColorRange(
        shipId,
        &(mColorBuffer.data()[mShipPointCount]),
        mShipPointCount,
        mEphemeralPointCount);


    //
    // 2. Upload points and/or textures
    //


    // TBD: at this moment we can't pass the point's connected component ID,
    // as the ShipRenderContext doesn't know how many connected components there are
    // (the number of connected components may vary depending on the connectivity visit,
    //  which is independent from ephemeral particles; the latter might insist on using
    //  a connected component ID that is well gone after a new connectivity visit).
    // This will be fixed with the Z buffer work - at that moment points will already
    // have an associated plane ID, and the shader will automagically
    // draw ephemeral points at the right Z for their point's plane ID.
    // Remember to make sure Ship always tracks the max plane ID it has
    // ever seen, and that it specifies it at RenderContext::RenderShipStart() via an
    // additional, new argument.

    if (mAreEphemeralParticlesDirty)
    {
        renderContext.UploadShipEphemeralPointsStart(shipId);
    }

    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        switch (GetEphemeralType(pointIndex))
        {
            case EphemeralType::AirBubble:
            {
                renderContext.UploadShipGenericTextureRenderSpecification(
                    shipId,
                    GetPlaneId(pointIndex),
                    TextureFrameId(TextureGroupType::AirBubble, mEphemeralStateBuffer[pointIndex].AirBubble.FrameIndex),
                    GetPosition(pointIndex),
                    mEphemeralStateBuffer[pointIndex].AirBubble.InitialSize, // Scale
                    mEphemeralStateBuffer[pointIndex].AirBubble.VortexAmplitude
                        + mEphemeralStateBuffer[pointIndex].AirBubble.Progress, // Angle
                    std::min(1.0f, mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY / 4.0f)); // Alpha

                break;
            }

            case EphemeralType::Debris:
            {
                // Don't upload point unless there's been a change
                if (mAreEphemeralParticlesDirty)
                {
                    renderContext.UploadShipEphemeralPoint(
                        shipId,
                        pointIndex);
                }

                break;
            }

            case EphemeralType::Sparkle:
            {
                renderContext.UploadShipGenericTextureRenderSpecification(
                    shipId,
                    GetPlaneId(pointIndex),
                    TextureFrameId(TextureGroupType::SawSparkle, mEphemeralStateBuffer[pointIndex].Sparkle.FrameIndex),
                    GetPosition(pointIndex),
                    1.0f,
                    4.0f * mEphemeralStateBuffer[pointIndex].Sparkle.Progress,
                    1.0f - mEphemeralStateBuffer[pointIndex].Sparkle.Progress);

                break;
            }

            case EphemeralType::None:
            default:
            {
                // Ignore
                break;
            }
        }
    }

    if (mAreEphemeralParticlesDirty)
    {
        renderContext.UploadShipEphemeralPointsEnd(shipId);

        mAreEphemeralParticlesDirty = false;
    }
}

void Points::SetMassToStructuralMaterialOffset(
    ElementIndex pointElementIndex,
    float offset,
    Springs & springs)
{
    assert(pointElementIndex < mElementCount);

    mMassBuffer[pointElementIndex] = GetStructuralMaterial(pointElementIndex).Mass + offset;

    // Notify all springs
    for (auto connectedSpring : mConnectedSpringsBuffer[pointElementIndex])
    {
        springs.OnPointMassUpdated(connectedSpring.SpringIndex, *this);
    }
}

void Points::UpdateTotalMasses(GameParameters const & gameParameters)
{
    //
    // Update:
    //  - TotalMass: material's mass + point's water mass
    //  - Integration factor: integration factor time coefficient / total mass
    //

    float const densityAdjustedWaterMass = GameParameters::WaterMass * gameParameters.WaterDensityAdjustment;

    for (ElementIndex i : *this)
    {
        float const totalMass =
            mMassBuffer[i]
            + std::min(GetWater(i), GetWaterVolumeFill(i)) * densityAdjustedWaterMass;

        assert(totalMass > 0.0f);

        mTotalMassBuffer[i] = totalMass;

        mIntegrationFactorBuffer[i] = vec2f(
            mIntegrationFactorTimeCoefficientBuffer[i] / totalMass,
            mIntegrationFactorTimeCoefficientBuffer[i] / totalMass);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ElementIndex Points::FindFreeEphemeralParticle(
    float currentSimulationTime,
    bool force)
{
    //
    // Search for the firt free ephemeral particle; if a free one is not found, reuse the
    // oldest particle
    //

    ElementIndex oldestParticle = NoneElementIndex;
    float oldestParticleLifetime = 0.0f;

    assert(mFreeEphemeralParticleSearchStartIndex >= mShipPointCount
        && mFreeEphemeralParticleSearchStartIndex < mAllPointCount);

    for (ElementIndex p = mFreeEphemeralParticleSearchStartIndex; ; /*incremented in loop*/)
    {
        if (EphemeralType::None == GetEphemeralType(p))
        {
            // Found!

            // Remember to start after this one next time
            mFreeEphemeralParticleSearchStartIndex = p + 1;
            if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
                mFreeEphemeralParticleSearchStartIndex = mShipPointCount;

            return p;
        }

        // Check whether it's the oldest
        auto lifetime = currentSimulationTime - mEphemeralStartTimeBuffer[p];
        if (lifetime >= oldestParticleLifetime)
        {
            oldestParticle = p;
            oldestParticleLifetime = lifetime;
        }

        // Advance
        ++p;
        if (p >= mAllPointCount)
            p = mShipPointCount;

        if (p == mFreeEphemeralParticleSearchStartIndex)
        {
            // Went around
            break;
        }
    }

    //
    // No luck
    //

    if (!force)
        return NoneElementIndex;


    //
    // Steal the oldest
    //

    assert(NoneElementIndex != oldestParticle);

    // Remember to start after this one next time
    mFreeEphemeralParticleSearchStartIndex = oldestParticle + 1;
    if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
        mFreeEphemeralParticleSearchStartIndex = mShipPointCount;

    return oldestParticle;
}

}