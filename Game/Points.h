/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "Materials.h"
#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/BufferAllocator.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/ElementIndexRangeIterator.h>
#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <vector>

namespace Physics
{

class Points : public ElementContainer
{
public:

    using DestroyHandler = std::function<void(
        ElementIndex,
        float /*currentSimulationTime*/,
        GameParameters const &)>;

    enum class EphemeralType
    {
        None,
        AirBubble,
        Debris,
        Smoke,
        Sparkle
    };

private:

    /*
     * The state of ephemeral particles.
     */
    union EphemeralState
    {
        struct AirBubbleState
        {
            TextureFrameIndex FrameIndex;
            float InitialY;
            float InitialSize;
            float VortexAmplitude;
            float VortexFrequency;

            float CurrentDeltaY;
            float Progress;
            float LastVortexValue;

            AirBubbleState()
            {}

            AirBubbleState(
                TextureFrameIndex frameIndex,
                float initialY,
                float initialSize,
                float vortexAmplitude,
                float vortexFrequency)
                : FrameIndex(frameIndex)
                , InitialY(initialY)
                , InitialSize(initialSize)
                , VortexAmplitude(vortexAmplitude)
                , VortexFrequency(vortexFrequency)
                , CurrentDeltaY(0.0f)
                , Progress(0.0f)
                , LastVortexValue(0.0f)
            {}
        };

        struct DebrisState
        {
        };

        struct SmokeState
        {
        };

        struct SparkleState
        {
            TextureFrameIndex FrameIndex;
            float Progress;

            SparkleState()
            {}

            SparkleState(
                TextureFrameIndex frameIndex)
                : FrameIndex(frameIndex)
                , Progress(0.0f)
            {}
        };

        AirBubbleState AirBubble;
        DebrisState Debris;
        SmokeState Smoke;
        SparkleState Sparkle;

        EphemeralState(AirBubbleState airBubble)
            : AirBubble(airBubble)
        {}

        EphemeralState(DebrisState debris)
            : Debris(debris)
        {}

        EphemeralState(SmokeState smoke)
            : Smoke(smoke)
        {}

        EphemeralState(SparkleState sparkle)
            : Sparkle(sparkle)
        {}
    };

    /*
     * The metadata for the springs connected to a point.
     */
    struct ConnectedSpring
    {
        ElementIndex SpringIndex;
        ElementIndex OtherEndpointIndex;

        ConnectedSpring()
            : SpringIndex(NoneElementIndex)
            , OtherEndpointIndex(NoneElementIndex)
        {}

        ConnectedSpring(
            ElementIndex springIndex,
            ElementIndex otherEndpointIndex)
            : SpringIndex(springIndex)
            , OtherEndpointIndex(otherEndpointIndex)
        {}
    };

    using ConnectedSpringsVector = FixedSizeVector<ConnectedSpring, GameParameters::MaxSpringsPerPoint>;

    /*
     * The metadata for the triangles connected to a point.
     */
    struct ConnectedTriangle
    {
        ElementIndex TriangleIndex;
        bool IsAtOwner; // true if the point "owns" this triangle (each triangle is owned by one and only one point)

        ConnectedTriangle()
            : TriangleIndex(NoneElementIndex)
            , IsAtOwner(false)
        {}

        ConnectedTriangle(
            ElementIndex triangleIndex,
            bool isAtOwner)
            : TriangleIndex(triangleIndex)
            , IsAtOwner(isAtOwner)
        {}
    };

    using ConnectedTrianglesVector = FixedSizeVector<ConnectedTriangle, GameParameters::MaxTrianglesPerPoint>;

    /*
     * The materials of this point.
     */
    struct Materials
    {
        StructuralMaterial const * Structural; // The only reason this is a pointer is that placeholders have no material
        ElectricalMaterial const * Electrical;

        Materials(
            StructuralMaterial const * structural,
            ElectricalMaterial const * electrical)
            : Structural(structural)
            , Electrical(electrical)
        {}
    };

public:

    Points(
        ElementCount shipPointCount,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        GameParameters const & gameParameters)
        : ElementContainer(shipPointCount + GameParameters::MaxEphemeralParticles)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, shipPointCount, false)
        // Materials
        , mMaterialsBuffer(mBufferElementCount, shipPointCount, Materials(nullptr, nullptr))
        , mIsRopeBuffer(mBufferElementCount, shipPointCount, false)
        // Mechanical dynamics
        , mPositionBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mForceBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mIntegrationFactorTimeCoefficientBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mTotalMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mIntegrationFactorBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mForceRenderBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        // Water dynamics
        , mIsHullBuffer(mBufferElementCount, shipPointCount, false)
        , mWaterVolumeFillBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterRestitutionBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterDiffusionSpeedBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mWaterMomentumBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mCumulatedIntakenWater(mBufferElementCount, shipPointCount, 0.0f)
        , mIsLeakingBuffer(mBufferElementCount, shipPointCount, false)
        // Electrical dynamics
        , mElectricalElementBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        , mLightBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Wind dynamics
        , mWindReceptivityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Ephemeral particles
        , mEphemeralTypeBuffer(mBufferElementCount, shipPointCount, EphemeralType::None)
        , mEphemeralStartTimeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mEphemeralMaxLifetimeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mEphemeralStateBuffer(mBufferElementCount, shipPointCount, EphemeralState::DebrisState())
        // Structure
        , mConnectedSpringsBuffer(mBufferElementCount, shipPointCount, ConnectedSpringsVector())
        , mConnectedTrianglesBuffer(mBufferElementCount, shipPointCount, ConnectedTrianglesVector())
        // Connected component and plane ID
        , mConnectedComponentIdBuffer(mBufferElementCount, shipPointCount, NoneConnectedComponentId)
        , mPlaneIdBuffer(mBufferElementCount, shipPointCount, NonePlaneId)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, shipPointCount, NoneVisitSequenceNumber)
        // Pinning
        , mIsPinnedBuffer(mBufferElementCount, shipPointCount, false)
        // Immutable render attributes
        , mColorBuffer(mBufferElementCount, shipPointCount, vec4f::zero())
        , mIsColorBufferDirty(false)
        , mTextureCoordinatesBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mIsTextureCoordinatesBufferDirty(false)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mShipPointCount(shipPointCount)
        , mEphemeralPointCount(GameParameters::MaxEphemeralParticles)
        , mAllPointCount(shipPointCount + mEphemeralPointCount)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mDestroyHandler()
        , mCurrentNumMechanicalDynamicsIterations(gameParameters.NumMechanicalDynamicsIterations<float>())
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
        , mFreeEphemeralParticleSearchStartIndex(mShipPointCount)
        , mAreEphemeralParticlesDirty(false)
    {
    }

    Points(Points && other) = default;

    /*
     * Returns an iterator for the non-ephemeral points only.
     */
    inline auto const NonEphemeralPoints() const
    {
        return ElementIndexRangeIterator(0, mShipPointCount);
    }

    /*
     * Returns a reverse iterator for the non-ephemeral points only.
     */
    inline auto const NonEphemeralPointsReverse() const
    {
        return ElementIndexReverseRangeIterator(0, mShipPointCount);
    }

    /*
     * Returns an iterator for the ephemeral points only.
     */
    inline auto const EphemeralPoints() const
    {
        return ElementIndexRangeIterator(mShipPointCount, mAllPointCount);
    }

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
        vec2f const & position,
        StructuralMaterial const & structuralMaterial,
        ElectricalMaterial const * electricalMaterial,
        bool isRope,
        ElementIndex electricalElementIndex,
        bool isLeaking,
        vec4f const & color,
        vec2f const & textureCoordinates);

    void CreateEphemeralParticleAirBubble(
        vec2f const & position,
        float initialSize,
        float vortexAmplitude,
        float vortexFrequency,
        StructuralMaterial const & structuralMaterial,
        float currentSimulationTime,
        PlaneId planeId);

    void CreateEphemeralParticleDebris(
        vec2f const & position,
        vec2f const & velocity,
        StructuralMaterial const & structuralMaterial,
        float currentSimulationTime,
        std::chrono::milliseconds maxLifetime,
        PlaneId planeId);

    void CreateEphemeralParticleSparkle(
        vec2f const & position,
        vec2f const & velocity,
        StructuralMaterial const & structuralMaterial,
        float currentSimulationTime,
        std::chrono::milliseconds maxLifetime,
        PlaneId planeId);

    void Destroy(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void UpdateGameParameters(GameParameters const & gameParameters);

    void UpdateEphemeralParticles(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void Query(ElementIndex pointElementIndex) const;

    //
    // Render
    //

    void UploadMutableAttributes(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadPlaneIds(
        ShipId shipId,
        PlaneId maxMaxPlaneId,
        Render::RenderContext & renderContext) const;

    void UploadElements(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadVectors(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadEphemeralParticles(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

public:

    //
    // IsDeleted
    //

    bool IsDeleted(ElementIndex pointElementIndex) const
    {
        return mIsDeletedBuffer[pointElementIndex];
    }

    //
    // Materials
    //

    StructuralMaterial const & GetStructuralMaterial(ElementIndex pointElementIndex) const
    {
        // If this method is invoked, this is not a placeholder
        assert(nullptr != mMaterialsBuffer[pointElementIndex].Structural);
        return *(mMaterialsBuffer[pointElementIndex].Structural);
    }

    ElectricalMaterial const * GetElectricalMaterial(ElementIndex pointElementIndex) const
    {
        return mMaterialsBuffer[pointElementIndex].Electrical;
    }

    bool IsRope(ElementIndex pointElementIndex) const
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

    vec2f * restrict GetPositionBufferAsVec2()
    {
        return mPositionBuffer.data();
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

    vec2f * restrict GetVelocityBufferAsVec2()
    {
        return mVelocityBuffer.data();
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

    void CopyForceBufferToForceRenderBuffer()
    {
        mForceRenderBuffer.copy_from(mForceBuffer);
    }

    float GetMass(ElementIndex pointElementIndex) const
    {
        return mMassBuffer[pointElementIndex];
    }

    void SetMassToStructuralMaterialOffset(
        ElementIndex pointElementIndex,
        float offset,
        Springs & springs);

    /*
     * Return's the total mass of the point, which equals the point's material's mass with
     * all modifiers (offsets, water, etc.).
     *
     * Only valid after a call to UpdateTotalMasses() and when
     * neither water quantities nor masses have changed since then.
     */
    float GetTotalMass(ElementIndex pointElementIndex)
    {
        return mTotalMassBuffer[pointElementIndex];
    }

    /*
     * The integration factor is the quantity which, when multiplied with the force on the point,
     * yields the change in position that occurs during a time interval equal to the dynamics simulation step.
     *
     * Only valid after a call to UpdateTotalMasses() and when
     * neither water quantities nor masses have changed since then.
     */
    float * restrict GetIntegrationFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mIntegrationFactorBuffer.data());
    }

    void UpdateTotalMasses(GameParameters const & gameParameters);

    // Changes the point's dynamics so that it freezes in place
    // and becomes oblivious to forces
    void Freeze(ElementIndex pointElementIndex)
    {
        // Zero-out integration factor time coefficient and velocity, freezing point
        mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = 0.0f;
        mVelocityBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
    }

    // Changes the point's dynamics so that the point reacts again to forces
    void Thaw(ElementIndex pointElementIndex)
    {
        // Re-populate its integration factor time coefficient, thawing point
        mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = CalculateIntegrationFactorTimeCoefficient(mCurrentNumMechanicalDynamicsIterations);
    }

    //
    // Water dynamics
    //

    bool IsHull(ElementIndex pointElementIndex) const
    {
        return mIsHullBuffer[pointElementIndex];
    }

    float GetWaterVolumeFill(ElementIndex pointElementIndex) const
    {
        return mWaterVolumeFillBuffer[pointElementIndex];
    }

    float GetWaterRestitution(ElementIndex pointElementIndex) const
    {
        return mWaterRestitutionBuffer[pointElementIndex];
    }

    float GetWaterDiffusionSpeed(ElementIndex pointElementIndex) const
    {
        return mWaterDiffusionSpeedBuffer[pointElementIndex];
    }

    float * restrict GetWaterBufferAsFloat()
    {
        return mWaterBuffer.data();
    }

    float GetWater(ElementIndex pointElementIndex) const
    {
        return mWaterBuffer[pointElementIndex];
    }

    float & GetWater(ElementIndex pointElementIndex)
    {
        return mWaterBuffer[pointElementIndex];
    }

    bool IsWet(
        ElementIndex pointElementIndex,
        float threshold) const
    {
        return mWaterBuffer[pointElementIndex] > threshold;
    }

    std::shared_ptr<Buffer<float>> MakeWaterBufferCopy()
    {
        auto waterBufferCopy = mFloatBufferAllocator.Allocate();
        waterBufferCopy->copy_from(mWaterBuffer);

        return waterBufferCopy;
    }

    void UpdateWaterBuffer(std::shared_ptr<Buffer<float>> newWaterBuffer)
    {
        mWaterBuffer.copy_from(*newWaterBuffer);
    }

    vec2f * restrict GetWaterVelocityBufferAsVec2()
    {
        return mWaterVelocityBuffer.data();
    }

    /*
     * Only valid after a call to UpdateWaterMomentaFromVelocities() and when
     * neither water quantities nor velocities have changed.
     */
    vec2f * restrict GetWaterMomentumBufferAsVec2f()
    {
        return mWaterMomentumBuffer.data();
    }

    void UpdateWaterMomentaFromVelocities()
    {
        float * const restrict waterBuffer = mWaterBuffer.data();
        vec2f * const restrict waterVelocityBuffer = mWaterVelocityBuffer.data();
        vec2f * restrict waterMomentumBuffer = mWaterMomentumBuffer.data();

        for (ElementIndex p = 0; p < mBufferElementCount; ++p)
        {
            waterMomentumBuffer[p] =
                waterVelocityBuffer[p]
                * waterBuffer[p];
        }
    }

    void UpdateWaterVelocitiesFromMomenta()
    {
        float * const restrict waterBuffer = mWaterBuffer.data();
        vec2f * restrict waterVelocityBuffer = mWaterVelocityBuffer.data();
        vec2f * const restrict waterMomentumBuffer = mWaterMomentumBuffer.data();

        for (ElementIndex p = 0; p < mBufferElementCount; ++p)
        {
            if (waterBuffer[p] != 0.0f)
            {
                waterVelocityBuffer[p] =
                    waterMomentumBuffer[p]
                    / waterBuffer[p];
            }
            else
            {
                // No mass, no velocity
                waterVelocityBuffer[p] = vec2f::zero();
            }
        }
    }

    float GetCumulatedIntakenWater(ElementIndex pointElementIndex) const
    {
        return mCumulatedIntakenWater[pointElementIndex];
    }

    float & GetCumulatedIntakenWater(ElementIndex pointElementIndex)
    {
        return mCumulatedIntakenWater[pointElementIndex];
    }

    bool IsLeaking(ElementIndex pointElementIndex) const
    {
        return mIsLeakingBuffer[pointElementIndex];
    }

    void SetLeaking(ElementIndex pointElementIndex)
    {
        mIsLeakingBuffer[pointElementIndex] = true;

        // Randomize the initial water intaken, so that air bubbles won't come out all at the same moment
        mCumulatedIntakenWater[pointElementIndex] = GameRandomEngine::GetInstance().GenerateRandomReal(
            0.0f,
            GameParameters::CumulatedIntakenWaterThresholdForAirBubbles);
    }

    //
    // Electrical dynamics
    //

    ElementIndex GetElectricalElement(ElementIndex pointElementIndex) const
    {
        return mElectricalElementBuffer[pointElementIndex];
    }

    float GetLight(ElementIndex pointElementIndex) const
    {
        return mLightBuffer[pointElementIndex];
    }

    float & GetLight(ElementIndex pointElementIndex)
    {
        return mLightBuffer[pointElementIndex];
    }

    //
    // Wind dynamics
    //

    float GetWindReceptivity(ElementIndex pointElementIndex) const
    {
        return mWindReceptivityBuffer[pointElementIndex];
    }

    //
    // Ephemeral Particles
    //

    EphemeralType GetEphemeralType(ElementIndex pointElementIndex) const
    {
        return mEphemeralTypeBuffer[pointElementIndex];
    }

    //
    // Network
    //

    auto const & GetConnectedSprings(ElementIndex pointElementIndex) const
    {
        return mConnectedSpringsBuffer[pointElementIndex];
    }

    void AddConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        ElementIndex otherEndpointElementIndex)
    {
        mConnectedSpringsBuffer[pointElementIndex].emplace_back(springElementIndex, otherEndpointElementIndex);
    }

    void RemoveConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex)
    {
        bool found = mConnectedSpringsBuffer[pointElementIndex].erase_first(
            [springElementIndex](ConnectedSpring const & c)
            {
                return c.SpringIndex == springElementIndex;
            });

        assert(found);
        (void)found;
    }

    auto const & GetConnectedTriangles(ElementIndex pointElementIndex) const
    {
        return mConnectedTrianglesBuffer[pointElementIndex];
    }

    void AddConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex,
        bool isAtOwner)
    {
        // Add so that all triangles owned by this point come first
        if (isAtOwner)
            mConnectedTrianglesBuffer[pointElementIndex].emplace_front(triangleElementIndex, isAtOwner);
        else
            mConnectedTrianglesBuffer[pointElementIndex].emplace_back(triangleElementIndex, isAtOwner);
    }

    void RemoveConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex)
    {
        bool found = mConnectedTrianglesBuffer[pointElementIndex].erase_first(
            [triangleElementIndex](ConnectedTriangle const & c)
            {
                return c.TriangleIndex == triangleElementIndex;
            });


        assert(found);
        (void)found;
    }

    //
    // Connected components and plane IDs
    //

    ConnectedComponentId GetConnectedComponentId(ElementIndex pointElementIndex) const
    {
        return mConnectedComponentIdBuffer[pointElementIndex];
    }

    void SetConnectedComponentId(
        ElementIndex pointElementIndex,
        ConnectedComponentId connectedComponentId)
    {
        mConnectedComponentIdBuffer[pointElementIndex] = connectedComponentId;
    }

    PlaneId GetPlaneId(ElementIndex pointElementIndex) const
    {
        return mPlaneIdBuffer[pointElementIndex];
    }

    void SetPlaneId(
        ElementIndex pointElementIndex,
        PlaneId planeId)
    {
        mPlaneIdBuffer[pointElementIndex] = planeId;
    }

    VisitSequenceNumber GetCurrentConnectivityVisitSequenceNumber(ElementIndex pointElementIndex) const
    {
        return mCurrentConnectivityVisitSequenceNumberBuffer[pointElementIndex];
    }

    void SetCurrentConnectivityVisitSequenceNumber(
        ElementIndex pointElementIndex,
        VisitSequenceNumber connectivityVisitSequenceNumber)
    {
        mCurrentConnectivityVisitSequenceNumberBuffer[pointElementIndex] =
            connectivityVisitSequenceNumber;
    }

    //
    // Pinning
    //

    bool IsPinned(ElementIndex pointElementIndex) const
    {
        return mIsPinnedBuffer[pointElementIndex];
    }

    void Pin(ElementIndex pointElementIndex)
    {
        assert(false == mIsPinnedBuffer[pointElementIndex]);

        mIsPinnedBuffer[pointElementIndex] = true;

        Freeze(pointElementIndex);
    }

    void Unpin(ElementIndex pointElementIndex)
    {
        assert(true == mIsPinnedBuffer[pointElementIndex]);

        mIsPinnedBuffer[pointElementIndex] = false;

        Thaw(pointElementIndex);
    }

    //
    // Immutable attributes
    //

    vec4f & GetColor(ElementIndex pointElementIndex)
    {
        return mColorBuffer[pointElementIndex];
    }

    void MarkColorBufferAsDirty()
    {
        mIsColorBufferDirty = true;
    }


    //
    // Temporary buffer
    //

    std::shared_ptr<Buffer<float>> AllocateWorkBufferFloat()
    {
        return mFloatBufferAllocator.Allocate();
    }

    std::shared_ptr<Buffer<vec2f>> AllocateWorkBufferVec2f()
    {
        return mVec2fBufferAllocator.Allocate();
    }

private:

    static float CalculateIntegrationFactorTimeCoefficient(float numMechanicalDynamicsIterations)
    {
        return GameParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations)
            * GameParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations);
    }

    ElementIndex FindFreeEphemeralParticle(
        float currentSimulationTime,
        bool force);

    inline void ExpireEphemeralParticle(ElementIndex pointElementIndex)
    {
        // Freeze the particle (just to prevent drifting)
        Freeze(pointElementIndex);

        // Hide this particle from ephemeral particles; this will prevent this particle from:
        // - Being rendered
        // - Being updated
        mEphemeralTypeBuffer[pointElementIndex] = EphemeralType::None;

        // Remember we're now dirty
        mAreEphemeralParticlesDirty = true;
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Materials
    Buffer<Materials> mMaterialsBuffer;
    Buffer<bool> mIsRopeBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mForceBuffer;
    Buffer<float> mMassBuffer;
    Buffer<float> mIntegrationFactorTimeCoefficientBuffer; // dt^2 or zero when the point is frozen

    // Calculated values
    Buffer<float> mTotalMassBuffer;
    Buffer<vec2f> mIntegrationFactorBuffer;
    Buffer<vec2f> mForceRenderBuffer;

    //
    // Water dynamics
    //

    Buffer<bool> mIsHullBuffer;
    Buffer<float> mWaterVolumeFillBuffer;
    Buffer<float> mWaterRestitutionBuffer;
    Buffer<float> mWaterDiffusionSpeedBuffer;

    // Height of a 1m2 column of water which provides a pressure equivalent to the pressure at
    // this point. Quantity of water is max(water, 1.0)
    Buffer<float> mWaterBuffer;

    // Total velocity of the water at this point
    Buffer<vec2f> mWaterVelocityBuffer;

    // Total momentum of the water at this point
    Buffer<vec2f> mWaterMomentumBuffer;

    // Total amount of water in/out taken which has not yet been
    // utilized for air bubbles
    Buffer<float> mCumulatedIntakenWater;

    Buffer<bool> mIsLeakingBuffer;

    //
    // Electrical dynamics
    //

    // Electrical element, when any
    Buffer<ElementIndex> mElectricalElementBuffer;

    // Total illumination, 0.0->1.0
    Buffer<float> mLightBuffer;

    //
    // Wind dynamics
    //

    Buffer<float> mWindReceptivityBuffer;

    //
    // Ephemeral Particles
    //

    Buffer<EphemeralType> mEphemeralTypeBuffer;
    Buffer<float> mEphemeralStartTimeBuffer;
    Buffer<float> mEphemeralMaxLifetimeBuffer;
    Buffer<EphemeralState> mEphemeralStateBuffer;

    //
    // Structure
    //

    Buffer<ConnectedSpringsVector> mConnectedSpringsBuffer;
    Buffer<ConnectedTrianglesVector> mConnectedTrianglesBuffer;

    //
    // Connectivity
    //

    Buffer<ConnectedComponentId> mConnectedComponentIdBuffer;
    Buffer<PlaneId> mPlaneIdBuffer;
    Buffer<VisitSequenceNumber> mCurrentConnectivityVisitSequenceNumberBuffer;

    //
    // Pinning
    //

    Buffer<bool> mIsPinnedBuffer;

    //
    // Immutable render attributes
    //

    Buffer<vec4f> mColorBuffer;
    bool mutable mIsColorBufferDirty;  // Whether or not is dirty since last render upload
    Buffer<vec2f> mTextureCoordinatesBuffer;
    bool mutable mIsTextureCoordinatesBufferDirty; // Whether or not is dirty since last render upload


    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Count of ship points; these are followed by ephemeral points
    ElementCount const mShipPointCount;

    // Count of ephemeral points
    ElementCount const mEphemeralPointCount;

    // Count of all points
    ElementCount const mAllPointCount;

    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> const mGameEventHandler;

    // The handler registered for point deletions
    DestroyHandler mDestroyHandler;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;

    // The index at which to start searching for free ephemeral particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeEphemeralParticleSearchStartIndex;

    // Flag remembering whether the set of ephemeral particles is dirty
    // (i.e. whether there are more or less particles than previously
    // reported to the rendering engine)
    bool mutable mAreEphemeralParticlesDirty;
};

}