/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

#include <cmath>
#include <limits>

namespace Physics {

void Points::Add(
    vec2f const & position,
    Material const * material,
    bool isHull,
    bool isRope,
    ElementIndex electricalElementIndex,
    bool isLeaking,
    float buoyancy,
    vec4f const & color,
    vec2f const & textureCoordinates)
{
    mIsDeletedBuffer.emplace_back(false);

    mMaterialBuffer.emplace_back(material);
    mIsHullBuffer.emplace_back(isHull);
    mIsRopeBuffer.emplace_back(isRope);

    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());
    mForceBuffer.emplace_back(vec2f::zero());
    mIntegrationFactorBuffer.emplace_back(
        CalculateIntegrationFactor(
            material->Mass,
            mCurrentNumMechanicalDynamicsIterations));
    mMassBuffer.emplace_back(material->Mass);

    mBuoyancyBuffer.emplace_back(buoyancy);
    mWaterBuffer.emplace_back(0.0f);
    mWaterVelocityBuffer.emplace_back(vec2f::zero());
    mWaterMomentumBuffer.emplace_back(vec2f::zero());
    mIsLeakingBuffer.emplace_back(isLeaking);

    // Electrical dynamics
    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    // Ephemeral particles
    mEphemeralTypeBuffer.emplace_back(EphemeralType::None);
    mEphemeralStartTimeBuffer.emplace_back(0.0f);
    mEphemeralMaxLifetimeBuffer.emplace_back(0.0f);
    mEphemeralStateBuffer.emplace_back(EphemeralState::DebrisState());

    // Structure
    mNetworkBuffer.emplace_back();

    mConnectedComponentIdBuffer.emplace_back(0u);
    mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer.emplace_back(NoneVisitSequenceNumber);

    mIsPinnedBuffer.emplace_back(false);

    mColorBuffer.emplace_back(color);
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
}

void Points::CreateEphemeralParticleDebris(
    vec2f const & position,
    vec2f const & velocity,
    Material const * material,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    ConnectedComponentId connectedComponentId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime);

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mIntegrationFactorBuffer[pointIndex] = CalculateIntegrationFactor(material->Mass, mCurrentNumMechanicalDynamicsIterations);
    mMassBuffer[pointIndex] = material->Mass;
    mMaterialBuffer[pointIndex] = material;

    mBuoyancyBuffer[pointIndex] = 0.0f; // Debris is non-buoyant
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Debris;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::DebrisState();
    mConnectedComponentIdBuffer[pointIndex] = connectedComponentId;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = material->RenderColour;

    // Remember we're dirty now
    mAreEphemeralParticlesDirty = true;
}

void Points::CreateEphemeralParticleSparkle(
    vec2f const & position,
    vec2f const & velocity,
    Material const * material,
    float currentSimulationTime,
    std::chrono::milliseconds maxLifetime,
    ConnectedComponentId connectedComponentId)
{
    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(currentSimulationTime);

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mIntegrationFactorBuffer[pointIndex] = CalculateIntegrationFactor(material->Mass, mCurrentNumMechanicalDynamicsIterations);
    mMassBuffer[pointIndex] = material->Mass;
    mMaterialBuffer[pointIndex] = material;

    mBuoyancyBuffer[pointIndex] = 0.0f; // Sparkles are non-buoyant
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Sparkle;
    mEphemeralStartTimeBuffer[pointIndex] = currentSimulationTime;
    mEphemeralMaxLifetimeBuffer[pointIndex] = std::chrono::duration_cast<std::chrono::duration<float>>(maxLifetime).count();
    mEphemeralStateBuffer[pointIndex] = EphemeralState::SparkleState(
        GameRandomEngine::GetInstance().Choose<TextureFrameIndex>(2));
    mConnectedComponentIdBuffer[pointIndex] = connectedComponentId;

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
        GetMaterial(pointElementIndex),
        mParentWorld.IsUnderwater(GetPosition(pointElementIndex)),
        1u);

    // Flag ourselves as deleted
    mIsDeletedBuffer[pointElementIndex] = true;

    // Let the physical world forget about us
    mPositionBuffer[pointElementIndex] = vec2f::zero();
    mVelocityBuffer[pointElementIndex] = vec2f::zero();
    mIntegrationFactorBuffer[pointElementIndex] = vec2f::zero();
    mWaterVelocityBuffer[pointElementIndex] = vec2f::zero();
    mWaterMomentumBuffer[pointElementIndex] = vec2f::zero();
}

void Points::UpdateGameParameters(GameParameters const & gameParameters)
{
    float const numMechanicalDynamicsIterations = gameParameters.NumMechanicalDynamicsIterations<float>();
    if (numMechanicalDynamicsIterations != mCurrentNumMechanicalDynamicsIterations)
    {
        // Recalc integration factors
        for (ElementIndex i : *this)
        {
            if (!IsDeleted(i))
            {
                mIntegrationFactorBuffer[i] = CalculateIntegrationFactor(
                    mMassBuffer[i],
                    numMechanicalDynamicsIterations);
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
            // Check if expired
            auto const elapsedLifetime = currentSimulationTime - mEphemeralStartTimeBuffer[pointIndex];
            if (elapsedLifetime >= mEphemeralMaxLifetimeBuffer[pointIndex])
            {
                //
                // Expire this particle
                //

                // Freeze the particle (just to prevent drifting)
                Freeze(pointIndex);

                // Hide this particle from ephemeral particles; this will prevent this particle from:
                // - Being rendered
                // - Being updates
                mEphemeralTypeBuffer[pointIndex] = EphemeralType::None;

                // Remember we're now dirty
                mAreEphemeralParticlesDirty = true;
            }
            else
            {
                //
                // Run this particle's state machine
                //

                switch (ephemeralType)
                {
                    case EphemeralType::Debris:
                    {
                        // Update alpha based off remaining time

                        float alpha = std::max(
                            1.0f - elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex],
                            0.0f);

                        mColorBuffer[pointIndex].w = alpha;

                        break;
                    }

                    case EphemeralType::Sparkle:
                    {
                        // Update progress based off remaining time

                        mEphemeralStateBuffer[pointIndex].Sparkle.Progress =
                            elapsedLifetime / mEphemeralMaxLifetimeBuffer[pointIndex];

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
}

void Points::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    // Upload immutable attributes, if we haven't uploaded them yet
    if (!mAreImmutableRenderAttributesUploaded)
    {
        renderContext.UploadShipPointImmutableGraphicalAttributes(
            shipId,
            mColorBuffer.data(),
            mTextureCoordinatesBuffer.data());

        mAreImmutableRenderAttributesUploaded = true;
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
                pointIndex,
                mConnectedComponentIdBuffer[pointIndex]);
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
            mVelocityBuffer.data(),
            0.25f,
            VectorColor);
    }
    else if (renderContext.GetVectorFieldRenderMode() == VectorFieldRenderMode::PointWaterVelocity)
    {
        renderContext.UploadShipVectors(
            shipId,
            mElementCount,
            mPositionBuffer.data(),
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
    // have an associated ConnectedComponent buffer, and the shader will automagically
    // draw ephemeral points at the right Z for their point's connected component ID.
    // Remember to make sure Ship always tracks the max connected component ID it has
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
                    1, // Connected component ID - see note above
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

void Points::SetMassToMaterialOffset(
    ElementIndex pointElementIndex,
    float offset,
    Springs & springs)
{
    assert(pointElementIndex < mElementCount);

    mMassBuffer[pointElementIndex] = mMaterialBuffer[pointElementIndex]->Mass + offset;

    // Update integration factor
    mIntegrationFactorBuffer[pointElementIndex] = CalculateIntegrationFactor(
        mMassBuffer[pointElementIndex],
        mCurrentNumMechanicalDynamicsIterations);

    // Notify all springs
    for (auto springIndex : mNetworkBuffer[pointElementIndex].ConnectedSprings)
    {
        springs.OnPointMassUpdated(springIndex, *this);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

vec2f Points::CalculateIntegrationFactor(
    float mass,
    float numMechanicalDynamicsIterations)
{
    assert(mass > 0.0f);
    assert(numMechanicalDynamicsIterations > 0.0f);

    //
    // The integration factor is the quantity which, once multiplied with the force on the point,
    // yields the change in position that occurs during a time interval equal to the dynamics simulation step.
    //

    float const dt = GameParameters::SimulationStepTimeDuration<float> / numMechanicalDynamicsIterations;

    return vec2f(dt * dt / mass, dt * dt / mass);
}

ElementIndex Points::FindFreeEphemeralParticle(float currentSimulationTime)
{
    //
    // Search for the firt free ephemeral particle; if a free one is not found, reuse the
    // oldest particle
    //

    ElementIndex oldestParticle = NoneElementIndex;
    float oldestParticleLifetime = 0.0f;

    assert(mFreeEphemeralParticleSearchStartIndex >= mShipPointCount
        && mFreeEphemeralParticleSearchStartIndex < mAllPointCount);

    for (ElementIndex p = mFreeEphemeralParticleSearchStartIndex; ; )
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
    // No luck, have to steal the oldest
    //

    assert(NoneElementIndex != oldestParticle);

    // Remember to start after this one next time
    mFreeEphemeralParticleSearchStartIndex = oldestParticle + 1;
    if (mFreeEphemeralParticleSearchStartIndex >= mAllPointCount)
        mFreeEphemeralParticleSearchStartIndex = mShipPointCount;

    return oldestParticle;
}

}