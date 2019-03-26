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
    mDecayBuffer.emplace_back(1.0f);
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

    mConnectedComponentIdBuffer.emplace_back(NoneConnectedComponentId);
    mPlaneIdBuffer.emplace_back(NonePlaneId);
    mPlaneIdFloatBuffer.emplace_back(0.0f);
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

    mIsDeletedBuffer[pointIndex] = false;

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = vec2f::zero();
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mDecayBuffer[pointIndex] = 1.0f;
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
        initialSize,
        vortexAmplitude,
        vortexFrequency);

    mConnectedComponentIdBuffer[pointIndex] = NoneConnectedComponentId;
    mPlaneIdBuffer[pointIndex] = planeId;
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;
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

    mIsDeletedBuffer[pointIndex] = false;

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mDecayBuffer[pointIndex] = 1.0f;
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
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = structuralMaterial.RenderColor;

    // Remember that ephemeral points are dirty now
    mAreEphemeralPointsDirty = true;
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

    mIsDeletedBuffer[pointIndex] = false;

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mMassBuffer[pointIndex] = structuralMaterial.Mass;
    mDecayBuffer[pointIndex] = 1.0f;
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
    mPlaneIdFloatBuffer[pointIndex] = static_cast<float>(planeId);
    mIsPlaneIdBufferEphemeralDirty = true;

    assert(false == mIsPinnedBuffer[pointIndex]);
}

void Points::Destroy(
    ElementIndex pointElementIndex,
    DestroyOptions destroyOptions,
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
            !!(destroyOptions & Points::DestroyOptions::GenerateDebris),
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

    // We're not anymore an ephemeral particle
    // (in case we were one...
    mEphemeralTypeBuffer[pointElementIndex] = EphemeralType::None;
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
            assert(!IsDeleted(pointIndex));

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
                        // Got to the surface, expire
                        ExpireEphemeralParticle(pointIndex);
                    }
                    else
                    {
                        //
                        // Update progress based off y
                        //

                        mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY = deltaY;

                        mEphemeralStateBuffer[pointIndex].AirBubble.Progress =
                            -1.0f
                            / (-1.0f + std::min(GetPosition(pointIndex).y, 0.0f));

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

                        // Remember that ephemeral points are now dirty
                        mAreEphemeralPointsDirty = true;
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
    LogMessage("Decay: ", mDecayBuffer[pointElementIndex]);
    LogMessage("Water: ", mWaterBuffer[pointElementIndex]);
    LogMessage("PlaneID: ", mPlaneIdBuffer[pointElementIndex]);
    LogMessage("ConnectedComponentID: ", mConnectedComponentIdBuffer[pointElementIndex]);
}

void Points::UploadAttributes(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Upload immutable attributes, if we haven't uploaded them yet
    if (mIsTextureCoordinatesBufferDirty)
    {
        renderContext.UploadShipPointImmutableAttributes(
            shipId,
            mTextureCoordinatesBuffer.data());

        mIsTextureCoordinatesBufferDirty = false;
    }

    // Upload colors, if dirty
    if (mIsWholeColorBufferDirty)
    {
        renderContext.UploadShipPointColors(
            shipId,
            mColorBuffer.data(),
            0,
            mAllPointCount);

        mIsWholeColorBufferDirty = false;
    }
    else
    {
        // Only upload ephemeral particle portion
        renderContext.UploadShipPointColors(
            shipId,
            &(mColorBuffer.data()[mShipPointCount]),
            mShipPointCount,
            mEphemeralPointCount);
    }

    //
    // Upload mutable attributes
    //

    renderContext.UploadShipPointMutableAttributesStart(shipId);

    renderContext.UploadShipPointMutableAttributes(
        shipId,
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data());

    if (mIsPlaneIdBufferNonEphemeralDirty)
    {
        if (mIsPlaneIdBufferEphemeralDirty)
        {
            // Whole

            renderContext.UploadShipPointMutableAttributesPlaneId(
                shipId,
                mPlaneIdFloatBuffer.data(),
                0,
                mAllPointCount);

            mIsPlaneIdBufferEphemeralDirty = false;
        }
        else
        {
            // Just non-ephemeral portion

            renderContext.UploadShipPointMutableAttributesPlaneId(
                shipId,
                mPlaneIdFloatBuffer.data(),
                0,
                mShipPointCount);
        }

        mIsPlaneIdBufferNonEphemeralDirty = false;
    }
    else if (mIsPlaneIdBufferEphemeralDirty)
    {
        // Just ephemeral portion

        renderContext.UploadShipPointMutableAttributesPlaneId(
            shipId,
            &(mPlaneIdFloatBuffer.data()[mShipPointCount]),
            mShipPointCount,
            mEphemeralPointCount);

        mIsPlaneIdBufferEphemeralDirty = false;
    }

    if (mIsDecayBufferDirty)
    {
        renderContext.UploadShipPointMutableAttributesDecay(
            shipId,
            mDecayBuffer.data(),
            0,
            mAllPointCount);

        mIsDecayBufferDirty = false;
    }

    renderContext.UploadShipPointMutableAttributesEnd(shipId);
}

void Points::UploadNonEphemeralPointElements(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    bool doUploadAllPoints = (DebugShipRenderMode::Points == renderContext.GetDebugShipRenderMode());

    for (ElementIndex pointIndex : NonEphemeralPoints())
    {
        if (!mIsDeletedBuffer[pointIndex])
        {
            if (doUploadAllPoints
                || mConnectedSpringsBuffer[pointIndex].ConnectedSprings.empty()) // orphaned
            {
                renderContext.UploadShipElementPoint(
                    shipId,
                    pointIndex);
            }
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
            mPlaneIdFloatBuffer.data(),
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
            mPlaneIdFloatBuffer.data(),
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
            mPlaneIdFloatBuffer.data(),
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
            mPlaneIdFloatBuffer.data(),
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
    // Upload points and/or textures
    //

    if (mAreEphemeralPointsDirty)
    {
        renderContext.UploadShipElementEphemeralPointsStart(shipId);
    }

    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        assert(GetEphemeralType(pointIndex) == EphemeralType::None || !IsDeleted(pointIndex));

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
                    0.0f, // Angle
                    std::min(1.0f, mEphemeralStateBuffer[pointIndex].AirBubble.CurrentDeltaY / 4.0f)); // Alpha

                break;
            }

            case EphemeralType::Debris:
            {
                // Don't upload point unless there's been a change
                if (mAreEphemeralPointsDirty)
                {
                    renderContext.UploadShipElementEphemeralPoint(
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

    if (mAreEphemeralPointsDirty)
    {
        renderContext.UploadShipElementEphemeralPointsEnd(shipId);

        mAreEphemeralPointsDirty = false;
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
    for (auto connectedSpring : mConnectedSpringsBuffer[pointElementIndex].ConnectedSprings)
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