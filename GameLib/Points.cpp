/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <cmath>
#include <limits>

namespace Physics {

void Points::Add(
    vec2f const & position,
    Material const * material,
    bool isHull,
    bool isRope,
    ElementIndex electricalElementIndex,
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
    mIntegrationFactorBuffer.emplace_back(CalculateIntegrationFactor(material->Mass));
    mMassBuffer.emplace_back(material->Mass);

    mBuoyancyBuffer.emplace_back(buoyancy);
    mWaterBuffer.emplace_back(0.0f);
    mWaterVelocityBuffer.emplace_back(vec2f::zero());
    mWaterMomentumBuffer.emplace_back(vec2f::zero());
    mIsLeakingBuffer.emplace_back(false);    

    // Electrical dynamics
    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    // Ephemeral particles
    mEphemeralTypeBuffer.emplace_back(EphemeralType::None);
    mEphemeralStartTimeBuffer.emplace_back(GameWallClock::time_point::min());
    mEphemeralMaxLifetimeBuffer.emplace_back(std::chrono::milliseconds::zero());
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
    std::chrono::milliseconds maxLifetime,
    ConnectedComponentId connectedComponentId)
{
    auto now = GameWallClock::GetInstance().Now();

    // Get a free slot (or steal one)
    auto pointIndex = FindFreeEphemeralParticle(now);

    //
    // Store attributes
    //

    assert(false == mIsDeletedBuffer[pointIndex]);

    mPositionBuffer[pointIndex] = position;
    mVelocityBuffer[pointIndex] = velocity;
    mForceBuffer[pointIndex] = vec2f::zero();
    mIntegrationFactorBuffer[pointIndex] = CalculateIntegrationFactor(material->Mass);
    mMassBuffer[pointIndex] = material->Mass;
    mMaterialBuffer[pointIndex] = material;
    
    mBuoyancyBuffer[pointIndex] = 0.0f; // Debris is non-buoyant
    mWaterBuffer[pointIndex] = 0.0f;
    assert(false == mIsLeakingBuffer[pointIndex]);

    mLightBuffer[pointIndex] = 0.0f;

    mEphemeralTypeBuffer[pointIndex] = EphemeralType::Debris;
    mEphemeralStartTimeBuffer[pointIndex] = now;
    mEphemeralMaxLifetimeBuffer[pointIndex] = maxLifetime;
    mEphemeralStateBuffer[pointIndex] = EphemeralState::DebrisState();
    mConnectedComponentIdBuffer[pointIndex] = connectedComponentId;

    assert(false == mIsPinnedBuffer[pointIndex]);

    mColorBuffer[pointIndex] = material->RenderColour;


    // Remember we're dirty now
    mAreEphemeralParticlesDirty = true;
}

void Points::Destroy(ElementIndex pointElementIndex)
{
    assert(pointElementIndex < mElementCount);
    assert(!IsDeleted(pointElementIndex));

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(pointElementIndex);
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

void Points::UpdateEphemeralParticles(
    GameWallClock::time_point now,
    GameParameters const & /*gameParameters*/)
{
    for (ElementIndex pointIndex : this->EphemeralPoints())
    {
        auto const ephemeralType = GetEphemeralType(pointIndex);
        if (EphemeralType::None != ephemeralType)
        {
            // Check if expired
            auto const elapsedLifetime = now - mEphemeralStartTimeBuffer[pointIndex];
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

                        float alpha = 1.0f -
                            static_cast<float>(elapsedLifetime.count())
                            / static_cast<float>(std::chrono::duration_cast<GameWallClock::duration>(mEphemeralMaxLifetimeBuffer[pointIndex]).count());
                        
                        mColorBuffer[pointIndex].w = alpha;

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
    int shipId,
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
    int shipId,
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
    int shipId,
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
    int shipId,
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

    if (mAreEphemeralParticlesDirty)
    {
        renderContext.UploadShipElementEphemeralPointsStart(shipId);
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
                    // TBD: at this moment we can't pass the point's connected component ID,
                    // as the ShipRenderContext doesn't know how many connected components there are
                    // (the number of connected components may vary depending on the connectivity visit,
                    //  which is independent from ephemeral particles; the latter might insist on using
                    //  a connected component ID that is well gone after a new connectivity visit)

                    renderContext.UploadShipElementEphemeralPoint(
                        shipId,
                        pointIndex,
                        1); // TBD: read above; should be GetConnectedComponentId(pointIndex)
                }

                break;
            }

            //// FUTURE: Those that are textures: call directly ShipRenderContext::UploadGenericTexture(...)

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
        renderContext.UploadShipElementEphemeralPointsEnd(shipId);

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
    mIntegrationFactorBuffer[pointElementIndex] = CalculateIntegrationFactor(mMassBuffer[pointElementIndex]);

    // Notify all springs
    for (auto springIndex : mNetworkBuffer[pointElementIndex].ConnectedSprings)
    {
        springs.OnPointMassUpdated(springIndex, *this);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

vec2f Points::CalculateIntegrationFactor(float mass)
{
    assert(mass > 0.0f);

    //
    // The integration factor is the quantity which, once multiplied with the force on the point,
    // yields the change in position, during a time interval equal to the dynamics simulation step.
    //

    static constexpr float dt = GameParameters::MechanicalDynamicsSimulationStepTimeDuration<float>;
    
    return vec2f(dt * dt / mass, dt * dt / mass);
}

ElementIndex Points::FindFreeEphemeralParticle(GameWallClock::time_point const & now)
{
    //
    // Search for the firt free ephemeral particle; if a free one is not found, reuse the 
    // oldest particle
    //

    ElementIndex oldestParticle = NoneElementIndex;
    typename GameWallClock::duration oldestParticleLifetime = GameWallClock::duration::min();

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
        auto lifetime = now - mEphemeralStartTimeBuffer[p];
        if (lifetime > oldestParticleLifetime)
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