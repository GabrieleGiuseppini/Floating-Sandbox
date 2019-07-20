/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "Materials.h"
#include "RenderContext.h"

#include <GameCore/Buffer.h>
#include <GameCore/BufferAllocator.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/ElementIndexRangeIterator.h>
#include <GameCore/EnumFlags.h>
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

    enum class DetachOptions
    {
        DoNotGenerateDebris = 0,
        GenerateDebris = 1
    };

    using DetachHandler = std::function<void(
        ElementIndex,
        bool /*generateDebris*/,
        float /*currentSimulationTime*/,
        GameParameters const &)>;

    using EphemeralParticleDestroyHandler = std::function<void(
        ElementIndex)>;

    /*
     * The types of ephemeral particles.
     */
    enum class EphemeralType
    {
        None,   // Not an ephemeral particle (or not an active ephemeral particle)
        AirBubble,
        Debris,
        Smoke,
        Sparkle
    };

    /*
     * The state required for repairing particles.
     */
    struct RepairState
    {
        // Each point is only allowed one role (attractor or attracted) per session step;
        // these keep track of when they last had each role
        RepairSessionId LastAttractorSessionId;
        RepairSessionStepId LastAttractorSessionStepId;
        RepairSessionId LastAttractedSessionId;
        RepairSessionStepId LastAttractedSessionStepId;

        // The number of consecutive steps - in this session - that this point has been
        // acting as attracted for
        std::uint64_t CurrentAttractedNumberOfSteps;

        RepairState()
            : LastAttractorSessionId(0)
            , LastAttractorSessionStepId(0)
            , LastAttractedSessionId(0)
            , LastAttractedSessionStepId(0)
            , CurrentAttractedNumberOfSteps(0)
        {}
    };

private:

    /*
     * The combustion state.
     */
    struct CombustionState
    {
    public:

        enum class StateType
        {
            NotBurning,
            Developing_1,
            Developing_2,
            Burning,
            Extinguishing_Consumed,
            Extinguishing_Smothered
        };

    public:

        StateType State;

        float FlameDevelopment;
        float MaxFlameDevelopment;
        float Personality; // Random number between 0.0 and 1.0

        CombustionState()
            : State(StateType::NotBurning)
            , FlameDevelopment(0.0f)
            , MaxFlameDevelopment(0.0f)
            , Personality(0.0f)
        {}
    };


    /*
     * The state of ephemeral particles.
     */
    union EphemeralState
    {
        struct AirBubbleState
        {
            TextureFrameIndex FrameIndex;
            float InitialSize;
            float VortexAmplitude;
            float NormalizedVortexAngularVelocity;

            float CurrentDeltaY;
            float Progress;
            float LastVortexValue;

            AirBubbleState()
            {}

            AirBubbleState(
                TextureFrameIndex frameIndex,
                float initialSize,
                float vortexAmplitude,
                float vortexPeriod)
                : FrameIndex(frameIndex)
                , InitialSize(initialSize)
                , VortexAmplitude(vortexAmplitude)
                , NormalizedVortexAngularVelocity(1.0f / vortexPeriod) // (2PI/vortexPeriod)/2PI
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
     * The metadata of a single spring connected to a point.
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

    /*
     * The metadata of all the springs connected to a point.
     */
    struct ConnectedSpringsVector
    {
        FixedSizeVector<ConnectedSpring, GameParameters::MaxSpringsPerPoint> ConnectedSprings;
        size_t OwnedConnectedSpringsCount;

        ConnectedSpringsVector()
            : ConnectedSprings()
            , OwnedConnectedSpringsCount(0u)
        {}

        inline void ConnectSpring(
            ElementIndex springElementIndex,
            ElementIndex otherEndpointElementIndex,
            bool isAtOwner)
        {
            // Add so that all springs owned by this point come first
            if (isAtOwner)
            {
                ConnectedSprings.emplace_front(springElementIndex, otherEndpointElementIndex);
                ++OwnedConnectedSpringsCount;
            }
            else
            {
                ConnectedSprings.emplace_back(springElementIndex, otherEndpointElementIndex);
            }
        }

        inline void DisconnectSpring(
            ElementIndex springElementIndex,
            bool isAtOwner)
        {
            bool found = ConnectedSprings.erase_first(
                [springElementIndex](ConnectedSpring const & c)
                {
                    return c.SpringIndex == springElementIndex;
                });

            assert(found);
            (void)found;

            // Update count of owned springs, if this spring is owned
            if (isAtOwner)
            {
                assert(OwnedConnectedSpringsCount > 0);
                --OwnedConnectedSpringsCount;
            }
        }
    };

    /*
     * The metadata of all the triangles connected to a point.
     */
    struct ConnectedTrianglesVector
    {
        FixedSizeVector<ElementIndex, GameParameters::MaxTrianglesPerPoint> ConnectedTriangles;
        size_t OwnedConnectedTrianglesCount;

        ConnectedTrianglesVector()
            : ConnectedTriangles()
            , OwnedConnectedTrianglesCount(0u)
        {}

        inline void ConnectTriangle(
            ElementIndex triangleElementIndex,
            bool isAtOwner)
        {
            // Add so that all triangles owned by this point come first
            if (isAtOwner)
            {
                ConnectedTriangles.emplace_front(triangleElementIndex);
                ++OwnedConnectedTrianglesCount;
            }
            else
            {
                ConnectedTriangles.emplace_back(triangleElementIndex);
            }
        }

        inline void DisconnectTriangle(
            ElementIndex triangleElementIndex,
            bool isAtOwner)
        {
            bool found = ConnectedTriangles.erase_first(
                [triangleElementIndex](ElementIndex c)
                {
                    return c == triangleElementIndex;
                });

            assert(found);
            (void)found;

            // Update count of owned triangles, if this triangle is owned
            if (isAtOwner)
            {
                assert(OwnedConnectedTrianglesCount > 0);
                --OwnedConnectedTrianglesCount;
            }
        }
    };


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
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters)
        : ElementContainer(shipPointCount + GameParameters::MaxEphemeralParticles)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        // Materials
        , mMaterialsBuffer(mBufferElementCount, shipPointCount, Materials(nullptr, nullptr))
        , mIsRopeBuffer(mBufferElementCount, shipPointCount, false)
        // Mechanical dynamics
        , mPositionBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mForceBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mAugmentedMaterialMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mDecayBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mIsDecayBufferDirty(true)
        , mIntegrationFactorTimeCoefficientBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mIntegrationFactorBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mForceRenderBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        // Water dynamics
        , mMaterialIsHullBuffer(mBufferElementCount, shipPointCount, false)
        , mMaterialWaterVolumeFillBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterIntakeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterRestitutionBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterDiffusionSpeedBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mWaterMomentumBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mCumulatedIntakenWater(mBufferElementCount, shipPointCount, 0.0f)
        , mIsLeakingBuffer(mBufferElementCount, shipPointCount, false)
        , mFactoryIsLeakingBuffer(mBufferElementCount, shipPointCount, false)
        // Heat dynamics
        , mTemperatureBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mIsTemperatureBufferDirty(true)
        , mMaterialHeatCapacityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialIgnitionTemperatureBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mCombustionStateBuffer(mBufferElementCount, shipPointCount, CombustionState())
        // Electrical dynamics
        , mElectricalElementBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        , mLightBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Wind dynamics
        , mMaterialWindReceptivityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Rust dynamics
        , mMaterialRustReceptivityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Ephemeral particles
        , mEphemeralTypeBuffer(mBufferElementCount, shipPointCount, EphemeralType::None)
        , mEphemeralStartTimeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mEphemeralMaxLifetimeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mEphemeralStateBuffer(mBufferElementCount, shipPointCount, EphemeralState::DebrisState())
        // Structure
        , mConnectedSpringsBuffer(mBufferElementCount, shipPointCount, ConnectedSpringsVector())
        , mFactoryConnectedSpringsBuffer(mBufferElementCount, shipPointCount, ConnectedSpringsVector())
        , mConnectedTrianglesBuffer(mBufferElementCount, shipPointCount, ConnectedTrianglesVector())
        , mFactoryConnectedTrianglesBuffer(mBufferElementCount, shipPointCount, ConnectedTrianglesVector())
        // Connected component and plane ID
        , mConnectedComponentIdBuffer(mBufferElementCount, shipPointCount, NoneConnectedComponentId)
        , mPlaneIdBuffer(mBufferElementCount, shipPointCount, NonePlaneId)
        , mPlaneIdFloatBuffer(mBufferElementCount, shipPointCount, 0.0)
        , mIsPlaneIdBufferNonEphemeralDirty(true)
        , mIsPlaneIdBufferEphemeralDirty(true)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, shipPointCount, SequenceNumber())
        // Pinning
        , mIsPinnedBuffer(mBufferElementCount, shipPointCount, false)
        // Repair
        , mRepairStateBuffer(mBufferElementCount, shipPointCount, RepairState())
        // Immutable render attributes
        , mColorBuffer(mBufferElementCount, shipPointCount, vec4f::zero())
        , mIsWholeColorBufferDirty(true)
        , mTextureCoordinatesBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mIsTextureCoordinatesBufferDirty(true)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mShipPointCount(shipPointCount)
        , mEphemeralPointCount(GameParameters::MaxEphemeralParticles)
        , mAllPointCount(mShipPointCount + mEphemeralPointCount)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mDetachHandler()
        , mEphemeralParticleDestroyHandler()
        , mCurrentNumMechanicalDynamicsIterations(gameParameters.NumMechanicalDynamicsIterations<float>())
        , mCurrentCumulatedIntakenWaterThresholdForAirBubbles(gameParameters.CumulatedIntakenWaterThresholdForAirBubbles)
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
        , mIgnitionCandidates(mShipPointCount)
        , mBurningPoints()
        , mFreeEphemeralParticleSearchStartIndex(mShipPointCount)
        , mAreEphemeralPointsDirty(false)
    {
    }

    Points(Points && other) = default;

    /*
     * Returns an iterator for the non-ephemeral (ship) points only.
     */
    inline auto const NonEphemeralPoints() const
    {
        return ElementIndexRangeIterator(0, mShipPointCount);
    }

    size_t const GetShipPointCount() const
    {
        return mShipPointCount;
    }

    /*
     * Returns a reverse iterator for the non-ephemeral (ship) points only.
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
     * Returns a flag indicating whether the point is active in the world.
     *
     * Active points are all non-ephemeral points and non-expired ephemeral points.
     */
    inline bool IsActive(ElementIndex pointIndex) const
    {
        return pointIndex < mShipPointCount
            || EphemeralType::None != mEphemeralTypeBuffer[pointIndex];
    }

    inline bool IsEphemeral(ElementIndex pointIndex) const
    {
        return pointIndex >= mShipPointCount;
    }

    /*
     * Sets a (single) handler that is invoked whenever a point is detached.
     *
     * The handler is invoked right before the point is modified for the detachment. However,
     * other elements connected to the soon-to-be-detached point might already have been
     * deleted.
     *
     * The handler is not re-entrant: detaching other points from it is not supported
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterDetachHandler(DetachHandler detachHandler)
    {
        assert(!mDetachHandler);
        mDetachHandler = std::move(detachHandler);
    }

    /*
     * Sets a (single) handler that is invoked whenever an ephemeral particle is destroyed.
     *
     * The handler is invoked right before the particle is modified for the destroy.
     *
     * The handler is not re-entrant: destroying other ephemeral particles from it is not supported
     * and leads to undefined behavior.
     *
     * Setting more than one handler is not supported and leads to undefined behavior.
     */
    void RegisterEphemeralParticleDestroyHandler(EphemeralParticleDestroyHandler ephemeralParticleDestroyHandler)
    {
        assert(!mEphemeralParticleDestroyHandler);
        mEphemeralParticleDestroyHandler = std::move(ephemeralParticleDestroyHandler);
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
        float vortexPeriod,
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

    void DestroyEphemeralParticle(
        ElementIndex pointElementIndex);

    void Detach(
        ElementIndex pointElementIndex,
        vec2f const & detachVelocity,
        DetachOptions detachOptions,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void OnOrphaned(ElementIndex pointElementIndex);

    void UpdateForGameParameters(GameParameters const & gameParameters);

    void UpdateCombustionLowFrequency(
        ElementIndex pointOffset,
        ElementIndex pointStride,
        float currentSimulationTime,
        float dt,
        GameParameters const & gameParameters);

    void UpdateCombustionHighFrequency(
        float currentSimulationTime,
        float dt,
        GameParameters const & gameParameters);

    void ReorderBurningPointsForDepth();

    void UpdateEphemeralParticles(
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void Query(ElementIndex pointElementIndex) const;

    //
    // Render
    //

    void UploadAttributes(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadNonEphemeralPointElements(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    void UploadFlames(
        ShipId shipId,
        float windSpeedMagnitude,
        Render::RenderContext & renderContext) const;

    void UploadVectors(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

    bool AreEphemeralPointsDirty() const
    {
        return mAreEphemeralPointsDirty;
    }

    void UploadEphemeralParticles(
        ShipId shipId,
        Render::RenderContext & renderContext) const;

public:

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

    void SetVelocity(
        ElementIndex pointElementIndex,
        vec2f const & velocity)
    {
        mVelocityBuffer[pointElementIndex] = velocity;
    }

    vec2f const & GetForce(ElementIndex pointElementIndex) const
    {
        return mForceBuffer[pointElementIndex];
    }

    vec2f & GetForce(ElementIndex pointElementIndex)
    {
        return mForceBuffer[pointElementIndex];
    }

    void AddForce(
        ElementIndex pointElementIndex,
        vec2f const & force)
    {
        mForceBuffer[pointElementIndex] += force;
    }

    float GetAugmentedMaterialMass(ElementIndex pointElementIndex) const
    {
        return mAugmentedMaterialMassBuffer[pointElementIndex];
    }

    void AugmentMaterialMass(
        ElementIndex pointElementIndex,
        float offset,
        Springs & springs);

    /*
     * Returns the total mass of the point, which equals the point's material's mass with
     * all modifiers (offsets, water, etc.).
     *
     * Only valid after a call to UpdateTotalMasses() and when
     * neither water quantities nor masses have changed since then.
     */
    float GetMass(ElementIndex pointElementIndex)
    {
        return mMassBuffer[pointElementIndex];
    }

    void UpdateMasses(GameParameters const & gameParameters);

    float GetDecay(ElementIndex pointElementIndex) const
    {
        return mDecayBuffer[pointElementIndex];
    }

    void SetDecay(
        ElementIndex pointElementIndex,
        float value)
    {
        mDecayBuffer[pointElementIndex] = value;
    }

    void MarkDecayBufferAsDirty()
    {
        mIsDecayBufferDirty = true;
    }

    /*
     * The integration factor is the quantity which, when multiplied with the force on the point,
     * yields the change in position that occurs during a time interval equal to the dynamics simulation step.
     *
     * Only valid after a call to UpdateMasses() and when
     * neither water quantities nor masses have changed since then.
     */
    float * restrict GetIntegrationFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mIntegrationFactorBuffer.data());
    }

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


    float * restrict GetForceBufferAsFloat()
    {
        return reinterpret_cast<float *>(mForceBuffer.data());
    }

    void CopyForceBufferToForceRenderBuffer()
    {
        mForceRenderBuffer.copy_from(mForceBuffer);
    }

    //
    // Water dynamics
    //

    bool GetMaterialIsHull(ElementIndex pointElementIndex) const
    {
        return mMaterialIsHullBuffer[pointElementIndex];
    }

    float GetMaterialWaterVolumeFill(ElementIndex pointElementIndex) const
    {
        return mMaterialWaterVolumeFillBuffer[pointElementIndex];
    }

    float GetMaterialWaterIntake(ElementIndex pointElementIndex) const
    {
        return mMaterialWaterIntakeBuffer[pointElementIndex];
    }

    float GetMaterialWaterRestitution(ElementIndex pointElementIndex) const
    {
        return mMaterialWaterRestitutionBuffer[pointElementIndex];
    }

    float GetMaterialWaterDiffusionSpeed(ElementIndex pointElementIndex) const
    {
        return mMaterialWaterDiffusionSpeedBuffer[pointElementIndex];
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
        mCumulatedIntakenWater[pointElementIndex] = RandomizeCumulatedIntakenWater(mCurrentCumulatedIntakenWaterThresholdForAirBubbles);
    }

    void RestoreFactoryIsLeaking(ElementIndex pointElementIndex)
    {
        mIsLeakingBuffer[pointElementIndex] = mFactoryIsLeakingBuffer[pointElementIndex];
    }

    //
    // Heat dynamics
    //

    float * restrict GetTemperatureBufferAsFloat()
    {
        return mTemperatureBuffer.data();
    }

    float GetTemperature(ElementIndex pointElementIndex) const
    {
        return mTemperatureBuffer[pointElementIndex];
    }

    void SetTemperature(
        ElementIndex pointElementIndex,
        float value)
    {
        mTemperatureBuffer[pointElementIndex] = value;
    }

    void MarkTemperatureBufferAsDirty()
    {
        mIsTemperatureBufferDirty = true;
    }

    std::shared_ptr<Buffer<float>> MakeTemperatureBufferCopy()
    {
        auto temperatureBufferCopy = mFloatBufferAllocator.Allocate();
        temperatureBufferCopy->copy_from(mTemperatureBuffer);

        return temperatureBufferCopy;
    }

    void UpdateTemperatureBuffer(std::shared_ptr<Buffer<float>> newTemperatureBuffer)
    {
        mTemperatureBuffer.copy_from(*newTemperatureBuffer);
    }

    float GetMaterialHeatCapacity(ElementIndex pointElementIndex) const
    {
        return mMaterialHeatCapacityBuffer[pointElementIndex];
    }

    /*
     * Checks whether a point is eligible for being extinguished by smotherning.
     */
    bool IsBurningForSmothering(ElementIndex pointElementIndex) const
    {
        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        return combustionState == CombustionState::StateType::Burning
            || combustionState == CombustionState::StateType::Developing_1
            || combustionState == CombustionState::StateType::Developing_2
            || combustionState == CombustionState::StateType::Extinguishing_Consumed;
    }

    void SmotherCombustion(ElementIndex pointElementIndex)
    {
        assert(IsBurningForSmothering(pointElementIndex));

        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        // Notify combustion end - if we are burning
        if (combustionState == CombustionState::StateType::Developing_1
            || combustionState == CombustionState::StateType::Developing_2
            || combustionState == CombustionState::StateType::Burning)
            mGameEventHandler->OnPointCombustionEnd();

        // Transition
        mCombustionStateBuffer[pointElementIndex].State = CombustionState::StateType::Extinguishing_Smothered;

        // Notify sizzling
        mGameEventHandler->OnCombustionSmothered();
    }

    void AddHeat(
        ElementIndex pointElementIndex,
        float heat) // J
    {
        mTemperatureBuffer[pointElementIndex] +=
            heat
            / GetMaterialHeatCapacity(pointElementIndex);
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

    float GetMaterialWindReceptivity(ElementIndex pointElementIndex) const
    {
        return mMaterialWindReceptivityBuffer[pointElementIndex];
    }

    //
    // Rust dynamics
    //

    float GetMaterialRustReceptivity(ElementIndex pointElementIndex) const
    {
        return mMaterialRustReceptivityBuffer[pointElementIndex];
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

    void ConnectSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        ElementIndex otherEndpointElementIndex,
        bool isAtOwner)
    {
        assert(mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.contains(
            [springElementIndex](auto const & cs)
            {
                return cs.SpringIndex == springElementIndex;
            }));

        mConnectedSpringsBuffer[pointElementIndex].ConnectSpring(
            springElementIndex,
            otherEndpointElementIndex,
            isAtOwner);
    }

    void DisconnectSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        bool isAtOwner)
    {
        mConnectedSpringsBuffer[pointElementIndex].DisconnectSpring(
            springElementIndex,
            isAtOwner);
    }

    auto const & GetFactoryConnectedSprings(ElementIndex pointElementIndex) const
    {
        return mFactoryConnectedSpringsBuffer[pointElementIndex];
    }

    void AddFactoryConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        ElementIndex otherEndpointElementIndex,
        bool isAtOwner)
    {
        // Add spring
        mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectSpring(
            springElementIndex,
            otherEndpointElementIndex,
            isAtOwner);

        // Connect spring
        ConnectSpring(
            pointElementIndex,
            springElementIndex,
            otherEndpointElementIndex,
            isAtOwner);
    }

    auto const & GetConnectedTriangles(ElementIndex pointElementIndex) const
    {
        return mConnectedTrianglesBuffer[pointElementIndex];
    }

    void ConnectTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex,
        bool isAtOwner)
    {
        assert(mFactoryConnectedTrianglesBuffer[pointElementIndex].ConnectedTriangles.contains(
            [triangleElementIndex](auto const & ct)
            {
                return ct == triangleElementIndex;
            }));

        mConnectedTrianglesBuffer[pointElementIndex].ConnectTriangle(
            triangleElementIndex,
            isAtOwner);
    }

    void DisconnectTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex,
        bool isAtOwner)
    {
        mConnectedTrianglesBuffer[pointElementIndex].DisconnectTriangle(
            triangleElementIndex,
            isAtOwner);
    }

    size_t GetConnectedOwnedTrianglesCount(ElementIndex pointElementIndex) const
    {
        return mConnectedTrianglesBuffer[pointElementIndex].OwnedConnectedTrianglesCount;
    }

    auto const & GetFactoryConnectedTriangles(ElementIndex pointElementIndex) const
    {
        return mFactoryConnectedTrianglesBuffer[pointElementIndex];
    }

    void AddFactoryConnectedTriangle(
        ElementIndex pointElementIndex,
        ElementIndex triangleElementIndex,
        bool isAtOwner)
    {
        // Add triangle
        mFactoryConnectedTrianglesBuffer[pointElementIndex].ConnectTriangle(
            triangleElementIndex,
            isAtOwner);

        // Connect triangle
        ConnectTriangle(
            pointElementIndex,
            triangleElementIndex,
            isAtOwner);
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
        PlaneId planeId,
        float planeIdFloat)
    {
        mPlaneIdBuffer[pointElementIndex] = planeId;
        mPlaneIdFloatBuffer[pointElementIndex] = planeIdFloat;
    }

    void MarkPlaneIdBufferNonEphemeralAsDirty()
    {
        mIsPlaneIdBufferNonEphemeralDirty = true;
    }

    SequenceNumber GetCurrentConnectivityVisitSequenceNumber(ElementIndex pointElementIndex) const
    {
        return mCurrentConnectivityVisitSequenceNumberBuffer[pointElementIndex];
    }

    void SetCurrentConnectivityVisitSequenceNumber(
        ElementIndex pointElementIndex,
        SequenceNumber ConnectivityVisitSequenceNumber)
    {
        mCurrentConnectivityVisitSequenceNumberBuffer[pointElementIndex] =
            ConnectivityVisitSequenceNumber;
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
    // Repair
    //

    RepairState & GetRepairState(ElementIndex pointElementIndex)
    {
        return mRepairStateBuffer[pointElementIndex];
    }

    //
    // Immutable attributes
    //

    vec4f & GetColor(ElementIndex pointElementIndex)
    {
        return mColorBuffer[pointElementIndex];
    }

    // Mostly for debugging
    void MarkColorBufferAsDirty()
    {
        mIsWholeColorBufferDirty = true;
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

    static inline float CalculateIntegrationFactorTimeCoefficient(float numMechanicalDynamicsIterations)
    {
        return GameParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations)
            * GameParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations);
    }

    static inline float RandomizeCumulatedIntakenWater(float cumulatedIntakenWaterThresholdForAirBubbles)
    {
        return GameRandomEngine::GetInstance().GenerateRandomReal(
            0.0f,
            cumulatedIntakenWaterThresholdForAirBubbles);
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
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Materials
    Buffer<Materials> mMaterialsBuffer;
    Buffer<bool> mIsRopeBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<vec2f> mForceBuffer;
    Buffer<float> mAugmentedMaterialMassBuffer; // Structural + Offset
    Buffer<float> mMassBuffer; // Augmented + Water
    Buffer<float> mDecayBuffer; // 1.0 -> 0.0 (completely decayed)
    bool mutable mIsDecayBufferDirty;
    Buffer<float> mIntegrationFactorTimeCoefficientBuffer; // dt^2 or zero when the point is frozen

    Buffer<vec2f> mIntegrationFactorBuffer;
    Buffer<vec2f> mForceRenderBuffer;

    //
    // Water dynamics
    //

    Buffer<bool> mMaterialIsHullBuffer;
    Buffer<float> mMaterialWaterVolumeFillBuffer;
    Buffer<float> mMaterialWaterIntakeBuffer;
    Buffer<float> mMaterialWaterRestitutionBuffer;
    Buffer<float> mMaterialWaterDiffusionSpeedBuffer;

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

    // When true, the point is intaking water
    Buffer<bool> mIsLeakingBuffer;
    Buffer<bool> mFactoryIsLeakingBuffer;

    //
    // Heat dynamics
    //

    Buffer<float> mTemperatureBuffer; // Kelvin
    bool mutable mIsTemperatureBufferDirty;
    Buffer<float> mMaterialHeatCapacityBuffer;
    Buffer<float> mMaterialIgnitionTemperatureBuffer;
    Buffer<CombustionState> mCombustionStateBuffer;

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

    Buffer<float> mMaterialWindReceptivityBuffer;

    //
    // Rust dynamics
    //

    Buffer<float> mMaterialRustReceptivityBuffer;

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
    Buffer<ConnectedSpringsVector> mFactoryConnectedSpringsBuffer;
    Buffer<ConnectedTrianglesVector> mConnectedTrianglesBuffer;
    Buffer<ConnectedTrianglesVector> mFactoryConnectedTrianglesBuffer;

    //
    // Connectivity
    //

    Buffer<ConnectedComponentId> mConnectedComponentIdBuffer;
    Buffer<PlaneId> mPlaneIdBuffer;
    Buffer<float> mPlaneIdFloatBuffer;
    bool mutable mIsPlaneIdBufferNonEphemeralDirty;
    bool mutable mIsPlaneIdBufferEphemeralDirty;
    Buffer<SequenceNumber> mCurrentConnectivityVisitSequenceNumberBuffer;

    //
    // Pinning
    //

    Buffer<bool> mIsPinnedBuffer;

    //
    // Repair state
    //

    Buffer<RepairState> mRepairStateBuffer;

    //
    // Immutable render attributes
    //

    Buffer<vec4f> mColorBuffer;
    bool mutable mIsWholeColorBufferDirty;  // Whether or not is dirty since last render upload
    Buffer<vec2f> mTextureCoordinatesBuffer;
    bool mutable mIsTextureCoordinatesBufferDirty; // Whether or not is dirty since last render upload


    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Count of ship points; these are followed by ephemeral points
    ElementCount const mShipPointCount;

    // Count of ephemeral points
    ElementCount const mEphemeralPointCount;

    // Count of all points (sum of two above)
    ElementCount const mAllPointCount;

    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;

    // The handler registered for point detachments
    DetachHandler mDetachHandler;

    // The handler registered for ephemeral particle destroy's
    EphemeralParticleDestroyHandler mEphemeralParticleDestroyHandler;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;
    float mCurrentCumulatedIntakenWaterThresholdForAirBubbles;

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;

    // The list of candidates for burning; member only
    // to save allocations at use time
    BoundedVector<std::tuple<ElementIndex, float>> mIgnitionCandidates;

    // The indices of the points that are currently burning
    std::vector<ElementIndex> mBurningPoints;

    // The index at which to start searching for free ephemeral particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeEphemeralParticleSearchStartIndex;

    // Flag remembering whether the set of ephemeral points is dirty
    // (i.e. whether there are more or less points than previously
    // reported to the rendering engine)
    bool mutable mAreEphemeralPointsDirty;
};

}

template <> struct is_flag<Physics::Points::DetachOptions> : std::true_type {};