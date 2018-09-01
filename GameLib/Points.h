/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "GameParameters.h"
#include "GameTypes.h"
#include "IGameEventHandler.h"
#include "Material.h"
#include "RenderContext.h"
#include "Vectors.h"

#include <cassert>
#include <functional>
#include <vector>

namespace Physics
{

class Points : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(ElementIndex)>;

private:

    /*
     * The elements connected to a point.
     */
    struct Network
    {        
        FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint> ConnectedSprings; 

        FixedSizeVector<ElementIndex, GameParameters::MaxTrianglesPerPoint> ConnectedTriangles;

        Network()
            : ConnectedSprings()
            , ConnectedTriangles()
        {}
    };

public:

    Points(
        ElementCount elementCount,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(elementCount)
        // Material
        , mMaterialBuffer(elementCount)
        , mIsHullBuffer(elementCount)
        , mIsRopeBuffer(elementCount)
        // Mechanical dynamics
        , mPositionBuffer(elementCount)
        , mVelocityBuffer(elementCount)
        , mForceBuffer(elementCount)
        , mIntegrationFactorBuffer(elementCount)
        , mMassBuffer(elementCount)
        // Water dynamics
        , mBuoyancyBuffer(elementCount)        
        , mWaterBuffer(elementCount)
        , mWaterMomentumBuffer(elementCount)
        , mIsLeakingBuffer(elementCount)
        // Electrical dynamics
        , mElectricalElementBuffer(elementCount)
        , mLightBuffer(elementCount)
        // Structure
        , mNetworkBuffer(elementCount)
        // Connected component
        , mConnectedComponentIdBuffer(elementCount)
        , mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer(elementCount)
        // Pinning
        , mIsPinnedBuffer(elementCount)
        // Immutable render attributes
        , mColorBuffer(elementCount)
        , mTextureCoordinatesBuffer(elementCount)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mDestroyHandler()
        , mAreImmutableRenderAttributesUploaded(false)
    {
    }

    Points(Points && other) = default;
    
    /*
     * Sets a (single) handler that is invoked whenever a point is destroyed.
     *
     * The handler is invoked right before the point is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted point might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other points from it is not supported 
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterDestroyHandler(DestroyHandler destroyHandler)
    {
        assert(!mDestroyHandler);
        mDestroyHandler = std::move(destroyHandler);
    }

    void Add(
        vec2 const & position,
        Material const * material,
        bool isHull,
        bool isRope,
        ElementIndex electricalElementIndex,
        float buoyancy,
        vec3f const & color,
        vec2f const & textureCoordinates);

    void Destroy(ElementIndex pointElementIndex);

    void Breach(
        ElementIndex pointElementIndex,
        Triangles & triangles);        

    void Upload(
        int shipId,
        RenderContext & renderContext) const;

    void UploadElements(
        int shipId,
        RenderContext & renderContext) const;

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex pointElementIndex) const
    {
        return mIsDeletedBuffer[pointElementIndex];
    }

    //
    // Material
    //

    inline Material const * GetMaterial(ElementIndex pointElementIndex) const
    {
        return mMaterialBuffer[pointElementIndex];
    }

    inline bool IsHull(ElementIndex pointElementIndex) const
    {
        return mIsHullBuffer[pointElementIndex];
    }

    inline bool IsRope(ElementIndex pointElementIndex) const
    {
        return mIsRopeBuffer[pointElementIndex];
    }

    //
    // Dynamics
    //

    vec2f const & GetPosition(ElementIndex pointElementIndex) const
    {
        return mPositionBuffer[pointElementIndex];
    }

    vec2f & GetPosition(ElementIndex pointElementIndex) 
    {
        return mPositionBuffer[pointElementIndex];
    }

    float * restrict GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(mPositionBuffer.data());
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f & GetVelocity(ElementIndex pointElementIndex)
    {
        return mVelocityBuffer[pointElementIndex];
    }

    float * restrict GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(mVelocityBuffer.data());
    }

    vec2f const & GetForce(ElementIndex pointElementIndex) const
    {
        return mForceBuffer[pointElementIndex];
    }

    vec2f & GetForce(ElementIndex pointElementIndex)
    {
        return mForceBuffer[pointElementIndex];
    }

    float * restrict GetForceBufferAsFloat()
    {
        return reinterpret_cast<float *>(mForceBuffer.data());
    }

    vec2f const & GetIntegrationFactor(ElementIndex pointElementIndex) const
    {
        return mIntegrationFactorBuffer[pointElementIndex];
    }

    float * restrict GetIntegrationFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mIntegrationFactorBuffer.data());
    }

    float GetMass(ElementIndex pointElementIndex) const
    {
        return mMassBuffer[pointElementIndex];
    }

    void SetMassToMaterialOffset(
        ElementIndex pointElementIndex,
        float offset,
        Springs & springs);

    //
    // Water dynamics
    //

    inline float GetBuoyancy(ElementIndex pointElementIndex) const
    {
        return mBuoyancyBuffer[pointElementIndex];
    }

    inline float GetWater(ElementIndex pointElementIndex) const
    {
        return mWaterBuffer[pointElementIndex];
    }

    inline void AddWater(
        ElementIndex pointElementIndex,
        float water)
    {
        mWaterBuffer[pointElementIndex] += water;
        assert(mWaterBuffer[pointElementIndex] >= 0.0f);
    }

    inline vec2f GetWaterMomentum(ElementIndex pointElementIndex) const
    {
        return mWaterMomentumBuffer[pointElementIndex];
    }

    inline void AddWaterMomentum(
        ElementIndex pointElementIndex,
        vec2f const & momentum)
    {
        if (momentum.length() > 1000000000.f)
            LogMessage("TODO1");
        if ((mWaterMomentumBuffer[pointElementIndex] + momentum).length() > 1000000000.f)
            LogMessage("TODO2");
        mWaterMomentumBuffer[pointElementIndex] += momentum;
    }

    // TODOOLD (verify first that it's not used)
    inline float GetExternalWaterPressure(
        ElementIndex pointElementIndex,
        float waterLevel) const
    {
        // Negative Y == under water line
        if (GetPosition(pointElementIndex).y < waterLevel)
        {
            // Pressure of a column of water of area 1m2 from the point up to the surface
            return GameParameters::GravityMagnitude * (waterLevel - GetPosition(pointElementIndex).y);
        }
        else
        {
            return 0.0f;
        }
    }

    inline bool IsLeaking(ElementIndex pointElementIndex) const
    {
        return mIsLeakingBuffer[pointElementIndex];
    }

    inline void SetLeaking(ElementIndex pointElementIndex)
    {
        mIsLeakingBuffer[pointElementIndex] = true;
    }

    //
    // Electrical dynamics
    //

    inline ElementIndex GetElectricalElement(ElementIndex pointElementIndex) const
    {
        return mElectricalElementBuffer[pointElementIndex];
    }

    inline float GetLight(ElementIndex pointElementIndex) const
    {
        return mLightBuffer[pointElementIndex];
    }

    inline float & GetLight(ElementIndex pointElementIndex)
    {
        return mLightBuffer[pointElementIndex];
    }

    //
    // Network
    //

    inline auto const & GetConnectedSprings(ElementIndex pointElementIndex) const
    {
        return mNetworkBuffer[pointElementIndex].ConnectedSprings;
    }

    inline void AddConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        mNetworkBuffer[pointElementIndex].ConnectedSprings.push_back(springElementIndex);
    }

    inline void RemoveConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        bool found = mNetworkBuffer[pointElementIndex].ConnectedSprings.erase_first(springElementIndex);

        assert(found);
        (void)found;
    }

    inline auto const & GetConnectedTriangles(ElementIndex pointElementIndex) const
    {
        return mNetworkBuffer[pointElementIndex].ConnectedTriangles;
    }

    inline void AddConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        mNetworkBuffer[pointElementIndex].ConnectedTriangles.push_back(triangleElementIndex);
    }

    inline void RemoveConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        bool found = mNetworkBuffer[pointElementIndex].ConnectedTriangles.erase_first(triangleElementIndex);

        assert(found);
        (void)found;
    }

    //
    // Pinning
    //

    inline bool IsPinned(ElementIndex pointElementIndex) const
    {
        return mIsPinnedBuffer[pointElementIndex];
    }

    void Pin(ElementIndex pointElementIndex) 
    {
        assert(false == mIsPinnedBuffer[pointElementIndex]);
    
        mIsPinnedBuffer[pointElementIndex] = true;

        // Zero-out integration factor and velocity, freezing point
        mIntegrationFactorBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
        mVelocityBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
    }

    void Unpin(ElementIndex pointElementIndex)
    {
        assert(true == mIsPinnedBuffer[pointElementIndex]);
    
        mIsPinnedBuffer[pointElementIndex] = false;

        // Re-populate its integration factor, thawing point
        mIntegrationFactorBuffer[pointElementIndex] = CalculateIntegrationFactor(mMassBuffer[pointElementIndex]);
    }

    //
    // Connected component
    //

    inline ConnectedComponentId GetConnectedComponentId(ElementIndex pointElementIndex) const
    {
        return mConnectedComponentIdBuffer[pointElementIndex];
    }

    inline void SetConnectedComponentId(
        ElementIndex pointElementIndex,
        ConnectedComponentId connectedComponentId)
    { 
        mConnectedComponentIdBuffer[pointElementIndex] = connectedComponentId;
    }

    inline VisitSequenceNumber GetCurrentConnectedComponentDetectionVisitSequenceNumber(ElementIndex pointElementIndex) const
    {
        return mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer[pointElementIndex];
    }

    inline void SetCurrentConnectedComponentDetectionVisitSequenceNumber(
        ElementIndex pointElementIndex,
        VisitSequenceNumber connectedComponentDetectionVisitSequenceNumber)
    { 
        mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer[pointElementIndex] =
            connectedComponentDetectionVisitSequenceNumber;
    }

private:

    static vec2f CalculateIntegrationFactor(float mass);

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Material
    Buffer<Material const *> mMaterialBuffer;
    Buffer<bool> mIsHullBuffer;
    Buffer<bool> mIsRopeBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mForceBuffer;
    Buffer<vec2f> mIntegrationFactorBuffer;
    Buffer<float> mMassBuffer;

    //
    // Water dynamics
    //

    Buffer<float> mBuoyancyBuffer;
    
    // Height of a 1m2 column of water which provides a pressure equivalent to the pressure at
    // this point. Quantity of water is max(water, 1.0)
    Buffer<float> mWaterBuffer;

    // Total momentum of the water at this point
    Buffer<vec2f> mWaterMomentumBuffer;

    Buffer<bool> mIsLeakingBuffer;

    //
    // Electrical dynamics
    //

    // Electrical element, when any
    Buffer<ElementIndex> mElectricalElementBuffer;

    // Total illumination, 0.0->1.0
    Buffer<float> mLightBuffer;

    //
    // Structure
    //

    Buffer<Network> mNetworkBuffer;

    //
    // Connected component
    //

    Buffer<ConnectedComponentId> mConnectedComponentIdBuffer; 
    Buffer<VisitSequenceNumber> mCurrentConnectedComponentDetectionVisitSequenceNumberBuffer;

    //
    // Pinning
    //

    Buffer<bool> mIsPinnedBuffer;

    //
    // Immutable render attributes
    //

    Buffer<vec3f> mColorBuffer;
    Buffer<vec2f> mTextureCoordinatesBuffer;


    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> const mGameEventHandler;

    // The handler registered for point deletions
    DestroyHandler mDestroyHandler;

    // Flag remembering whether or not we've already uploaded
    // the immutable render attributes
    bool mutable mAreImmutableRenderAttributesUploaded;
};

}