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
    vec2 const & position,
    Material const * material,
    bool isHull,
    bool isRope,
    ElementIndex electricalElementIndex,
    float buoyancy,
    vec3f const & color,
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
    mIsLeakingBuffer.emplace_back(false);    

    mElectricalElementBuffer.emplace_back(electricalElementIndex);
    mLightBuffer.emplace_back(0.0f);

    mNetworkBuffer.emplace_back();

    mConnectedComponentIdBuffer.emplace_back(0u);
    mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer.emplace_back(NoneVisitSequenceNumber);

    mIsPinnedBuffer.emplace_back(false);

    mColorBuffer.emplace_back(color);
    mTextureCoordinatesBuffer.emplace_back(textureCoordinates);
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
}

void Points::Breach(
    ElementIndex pointElementIndex,
    Triangles & triangles)
{
    assert(pointElementIndex < mElementCount);

    //
    // Start leaking
    //

    mIsLeakingBuffer[pointElementIndex] = true;

    //
    // Destroy all of our connected triangles
    //

    // Note: we can't simply iterate and destroy, as destroying a triangle causes
    // that triangle to be removed from the vector being iterated
    auto & connectedTriangles = GetConnectedTriangles(pointElementIndex);
    while (!connectedTriangles.empty())
    {
        assert(!triangles.IsDeleted(connectedTriangles.back()));
        triangles.Destroy(connectedTriangles.back());
    }

    assert(GetConnectedTriangles(pointElementIndex).empty());
}

void Points::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    // Upload immutable attributes, if we haven't uploaded them yet
    if (!mAreImmutableRenderAttributesUploaded)
    { 
        renderContext.UploadShipPointImmutableGraphicalAttributes(
            shipId,
            mElementCount,
            mColorBuffer.data(),
            mTextureCoordinatesBuffer.data());

        mAreImmutableRenderAttributesUploaded = true;
    }

    // Upload mutable attributes
    renderContext.UploadShipPoints(
        shipId,  
        mElementCount,
        mPositionBuffer.data(),
        mLightBuffer.data(),
        mWaterBuffer.data());
}

void Points::UploadElements(
    int shipId,
    RenderContext & renderContext) const
{
    for (ElementIndex i : *this)
    {
        if (!mIsDeletedBuffer[i])
        {
            renderContext.UploadShipElementPoint(
                shipId,
                i,
                mConnectedComponentIdBuffer[i]);
        }
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

}