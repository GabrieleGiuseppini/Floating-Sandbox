/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"
#include "../MaterialDatabase.h"
#include "../Materials.h"

#include <Render/GameTextureDatabases.h>
#include <Render/RenderContext.h>

#include <Core/AABB.h>
#include <Core/Buffer.h>
#include <Core/BufferAllocator.h>
#include <Core/ElementContainer.h>
#include <Core/ElementIndexRangeIterator.h>
#include <Core/EnumFlags.h>
#include <Core/FixedSizeVector.h>
#include <Core/GameMath.h>
#include <Core/GameRandomEngine.h>
#include <Core/GameTypes.h>
#include <Core/GameWallClock.h>
#include <Core/Vectors.h>

#include <algorithm>
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
        None = 0,
        GenerateDebris = 1,
        FireDestroyEvent = 2,
    };

    /*
     * The types of ephemeral particles.
     */
    enum class EphemeralType
    {
        None,   // Not an ephemeral particle (or not an _active_ ephemeral particle)
        AirBubble,
        Debris,
        Smoke,
        Sparkle,
        WakeBubble
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
        FixedSizeVector<ConnectedSpring, SimulationParameters::MaxSpringsPerPoint> ConnectedSprings;
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
     * The state required for repairing particles.
     */
    struct RepairState
    {
        // The last steps at which this point had the specified repair role
        SequenceNumber LastAttractorRepairStepId;
        SequenceNumber LastAttracteeRepairStepId;

        // Total number of consecutive steps that this point has been
        // an attractee for
        std::uint64_t CurrentAttracteeConsecutiveNumberOfSteps;

        // Visit sequence ID for attractor propagation
        SequenceNumber CurrentAttractorPropagationVisitStepId;

        RepairState()
            : LastAttractorRepairStepId()
            , LastAttracteeRepairStepId()
            , CurrentAttracteeConsecutiveNumberOfSteps()
            , CurrentAttractorPropagationVisitStepId()
        {}
    };

    /*
     * The leaking-related properties of a particle.
     */
    union LeakingComposite
    {
#pragma pack(push, 1)
        struct LeakingSourcesType
        {
            float StructuralLeak; // 0.0 or 1.0
            float WaterPumpForce; // -1.0 [out], ..., +1.0 [in]

            LeakingSourcesType(
                float structuralLeak,
                float waterPumpNominalForce)
                : StructuralLeak(structuralLeak)
                , WaterPumpForce(waterPumpNominalForce)
            {}
        };
#pragma pack(pop)

        LeakingSourcesType LeakingSources;
        uint64_t IsCumulativelyLeaking; // Allows for "if (IsLeaking)"

        LeakingComposite(bool isStructurallyLeaking)
            : LeakingSources(
                isStructurallyLeaking ? 1.0f : 0.0f,
                0.0f)
        {}
    };

    /*
     * Packed precalculated buoyancy coefficients.
     */
    struct BuoyancyCoefficients
    {
        float Coefficient1; // Temperature-independent
        float Coefficient2; // Temperature-dependent

        BuoyancyCoefficients(
            float coefficient1,
            float coefficient2)
            : Coefficient1(coefficient1)
            , Coefficient2(coefficient2)
        {}
    };

    /*
     * Packed ocean floor collision factors.
     */
    struct OceanFloorCollisionFactors
    {
        float ElasticityFactor;
        float StaticFrictionFactor;
        float KineticFrictionFactor;

        OceanFloorCollisionFactors(
            float elasticityFactor,
            float staticFrictionFactor,
            float kineticFrictionFactor)
            : ElasticityFactor(elasticityFactor)
            , StaticFrictionFactor(staticFrictionFactor)
            , KineticFrictionFactor(kineticFrictionFactor)
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
            Extinguishing_SmotheredRain,
            Extinguishing_SmotheredWater,

            Exploded
        };

    public:

        StateType State;

        float FlameDevelopment;
        float MaxFlameDevelopment;
        // FUTUREWORK
        //float NextSmokeEmissionSimulationTimestamp;

        // The current flame vector, which provides direction and magnitude
        // of the flame quad.
        // Slowly converges to the target vector, which is the resultant of
        // (air) buoyancy making the flame upwards, added to the particle's
        // current velocity
        vec2f FlameVector;

        // Angle of the flame tilt due to moving air; consumed by the shader.
        // Slowly converges to the target value.
        // Domain: ~[-0.5, 0.5].
        float FlameWindRotationAngle;

        CombustionState()
        {
            Reset();
        }

        inline void Reset()
        {
            State = StateType::NotBurning;
            FlameDevelopment = 0.0f;
            MaxFlameDevelopment = 0.0f;
            // FUTUREWORK
            //NextSmokeEmissionSimulationTimestamp = 0.0f;
            FlameVector = vec2f(0.0f, 1.0f);
            FlameWindRotationAngle = 0.0f;
        }
    };

    /*
     * The state of water reactions.
     */
    struct WaterReactionState
    {
    public:

        enum class StateType
        {
            Inert, // Material will never react to water
            Unreacted, // Material is reactive, but reaction hasn't triggered yet
            ReactionTriggered, // Reaction has triggered, waiting to explode
            Consumed // Exploded, won't react anymore
        };

    public:

        StateType State;
        float Threshold;
        GameWallClock::float_time ExplosionTimestamp; // Only valid in ReactionTriggered state

        WaterReactionState(float materialWaterReactivityThreshold)
        {
            Threshold = materialWaterReactivityThreshold;
            Reset();
        }

        inline void Reset()
        {
            State = (Threshold == 0.0f) ? StateType::Inert : StateType::Unreacted;
            ExplosionTimestamp = std::numeric_limits<GameWallClock::float_time>::min();
        }
    };

    /*
     * The state of ephemeral particles.
     */
    union EphemeralState
    {
        struct AirBubbleState
        {
            float FinalScale;

            float VortexAmplitude;
            float NormalizedVortexAngularVelocity;

            float CurrentDeltaY;
            float SimulationLifetime;

            AirBubbleState()
            {}

            AirBubbleState(
                float finalScale,
                float vortexAmplitude,
                float vortexPeriod)
                : FinalScale(finalScale)
                , VortexAmplitude(vortexAmplitude)
                , NormalizedVortexAngularVelocity(1.0f / vortexPeriod) // (2PI/vortexPeriod)/2PI
                , CurrentDeltaY(0.0f)
                , SimulationLifetime(0.0f)
            {}
        };

        struct DebrisState
        {
        };

        struct SmokeState
        {
            enum class GrowthType
            {
                Slow,
                Fast
            };

            GameTextureDatabases::GenericMipMappedTextureGroups TextureGroup;
            GrowthType Growth;
            float PersonalitySeed;
            float LifetimeProgress;
            float ScaleProgress;

            SmokeState()
                : TextureGroup(GameTextureDatabases::GenericMipMappedTextureGroups::SmokeLight) // Arbitrary
                , Growth(GrowthType::Slow) // Arbitrary
                , PersonalitySeed(0.0f)
                , LifetimeProgress(0.0f)
                , ScaleProgress(0.0f)
            {}

            SmokeState(
                GameTextureDatabases::GenericMipMappedTextureGroups textureGroup,
                GrowthType growth,
                float personalitySeed)
                : TextureGroup(textureGroup)
                , Growth(growth)
                , PersonalitySeed(personalitySeed)
                , LifetimeProgress(0.0f)
                , ScaleProgress(0.0f)
            {}
        };

        struct SparkleState
        {
            float Progress;

            SparkleState()
                : Progress(0.0f)
            {}
        };

        struct WakeBubbleState
        {
            float Progress;

            WakeBubbleState()
                : Progress(0.0f)
            {}
        };

        AirBubbleState AirBubble;
        DebrisState Debris;
        SmokeState Smoke;
        SparkleState Sparkle;
        WakeBubbleState WakeBubble;

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

        EphemeralState(WakeBubbleState wakeBubble)
            : WakeBubble(wakeBubble)
        {}
    };

    /*
     * First cluster of ephemeral particle attributes that are used
     * always together, mostly when looking for free ephemeral
     * particle slots.
     */
    struct EphemeralParticleAttributes1
    {
        EphemeralType Type;
        float StartSimulationTime;

        EphemeralParticleAttributes1()
            : Type(EphemeralType::None)
            , StartSimulationTime(0.0f)
        {}
    };

    /*
     * Second cluster of ephemeral particle attributes that are used
     * (almost) always together.
     */
    struct EphemeralParticleAttributes2
    {
        EphemeralState State;
        float MaxSimulationLifetime;

        EphemeralParticleAttributes2()
            : State(EphemeralState::DebrisState()) // Arbitrary
            , MaxSimulationLifetime(0.0f)
        {}
    };

    /*
     * The metadata of all the triangles connected to a point.
     */
    struct ConnectedTrianglesVector
    {
        FixedSizeVector<ElementIndex, SimulationParameters::MaxTrianglesPerPoint> ConnectedTriangles;
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

    /*
     * A point being highlighted in the "ElectricalElement" mode, half-way through its highlight state machine.
     */
    struct ElectricalElementHighlightState
    {
        ElementIndex PointIndex;
        rgbColor HighlightColor;
        GameWallClock::float_time StartTime;
        float Progress;

        ElectricalElementHighlightState(
            ElementIndex pointIndex,
            rgbColor const & highlightColor,
            GameWallClock::float_time startTime)
            : PointIndex(pointIndex)
            , HighlightColor(highlightColor)
            , StartTime(startTime)
            , Progress(0.0f)
        {}

        void Reset(
            rgbColor const & highlightColor,
            GameWallClock::float_time startTime)
        {
            HighlightColor = highlightColor;
            StartTime = startTime;
            Progress = 0.0f;
        }
    };

    /*
     * A point being highlighted in the "Circle" mode.
     */
    struct CircleHighlightState
    {
        ElementIndex PointIndex;
        rgbColor HighlightColor;
        size_t SimulationStepsExperienced;

        CircleHighlightState(
            ElementIndex pointIndex,
            rgbColor const & highlightColor)
            : PointIndex(pointIndex)
            , HighlightColor(highlightColor)
            , SimulationStepsExperienced(0)
        {}

        void Reset(rgbColor const & highlightColor)
        {
            HighlightColor = highlightColor;
            SimulationStepsExperienced = 0;
        }
    };

public:

    Points(
        ElementCount shipPointCount,
        World & parentWorld,
        MaterialDatabase const & materialDatabase,
        SimulationEventDispatcher & simulationEventDispatcher,
        SimulationParameters const & simulationParameters)
        : ElementContainer(make_aligned_float_element_count(shipPointCount) + SimulationParameters::MaxEphemeralParticles)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDamagedBuffer(mBufferElementCount, shipPointCount, false)
        // Materials
        , mMaterialsBuffer(mBufferElementCount, shipPointCount, Materials(nullptr, nullptr))
        , mIsRopeBuffer(mBufferElementCount, shipPointCount, false)
        // Mechanical dynamics
        , mPositionBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mFactoryPositionBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mDynamicForceBuffers() // We'll start later with at least one
        , mDynamicForceRawBuffers()
        , mStaticForceBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mAugmentedMaterialMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mTransientAdditionalMassBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMassBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mMaterialBuoyancyVolumeFillBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mStrengthBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mStressBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mDecayBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mIsDecayBufferDirty(true)
        , mPinningCoefficientBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mIntegrationFactorTimeCoefficientBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mOceanFloorCollisionFactorsBuffer(mBufferElementCount, shipPointCount, OceanFloorCollisionFactors(0.0f, 0.0f, 0.0f))
        , mAirWaterInterfaceInverseWidthBuffer(mBufferElementCount, shipPointCount, 1.0f)
        , mBuoyancyCoefficientsBuffer(mBufferElementCount, shipPointCount, BuoyancyCoefficients(0.0f, 0.0f))
        , mCachedDepthBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mIntegrationFactorBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        // Pressure and water dynamics
        , mIsHullBuffer(mBufferElementCount, shipPointCount, false)
        , mInternalPressureBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterIntakeBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterRestitutionBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialWaterDiffusionSpeedBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mWaterVelocityBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mWaterMomentumBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mCumulatedIntakenWater(mBufferElementCount, shipPointCount, 0.0f)
        , mLeakingCompositeBuffer(mBufferElementCount, shipPointCount, LeakingComposite(false))
        , mFactoryIsStructurallyLeakingBuffer(mBufferElementCount, shipPointCount, false)
        , mTotalFactoryWetPoints(0)
        // Heat dynamics
        , mTemperatureBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialHeatCapacityReciprocalBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialThermalExpansionCoefficientBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialIgnitionTemperatureBuffer(mBufferElementCount, shipPointCount, 0.0f)
        , mMaterialCombustionTypeBuffer(mBufferElementCount, shipPointCount, StructuralMaterial::MaterialCombustionType::Combustion) // Arbitrary
        , mCombustionStateBuffer(mBufferElementCount, shipPointCount, CombustionState())
        // Water reaction dynamics
        , mWaterReactionStateBuffer(mBufferElementCount, shipPointCount, WaterReactionState(0.0f))
        // Electrical dynamics
        , mElectricalElementBuffer(mBufferElementCount, shipPointCount, NoneElementIndex)
        , mLightBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Wind dynamics
        , mMaterialWindReceptivityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Rust dynamics
        , mMaterialRustReceptivityBuffer(mBufferElementCount, shipPointCount, 0.0f)
        // Various interactions
        , mIsElectrifiedBuffer(mBufferElementCount, shipPointCount, false)
        // Ephemeral particles
        , mEphemeralParticleAttributes1Buffer(mBufferElementCount, shipPointCount, EphemeralParticleAttributes1())
        , mEphemeralParticleAttributes2Buffer(mBufferElementCount, shipPointCount, EphemeralParticleAttributes2())
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
        // Repair
        , mRepairStateBuffer(mBufferElementCount, shipPointCount, RepairState())
        // Highlights
        , mElectricalElementHighlightedPoints()
        , mCircleHighlightedPoints()
        // Gadgets
        , mIsGadgetAttachedBuffer(mBufferElementCount, mElementCount, false)
        // Randomness
        , mRandomNormalizedUniformFloatBuffer(mBufferElementCount, shipPointCount, [](size_t){ return GameRandomEngine::GetInstance().GenerateNormalizedUniformReal(); })
        // Immutable render attributes
        , mColorBuffer(mBufferElementCount, shipPointCount, vec4f::zero())
        , mIsWholeColorBufferDirty(true)
        , mIsEphemeralColorBufferDirty(true)
        , mTextureCoordinatesBuffer(mBufferElementCount, shipPointCount, vec2f::zero())
        , mIsTextureCoordinatesBufferDirty(true)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mRawShipPointCount(shipPointCount)
        , mAlignedShipPointCount(make_aligned_float_element_count(shipPointCount))
        , mEphemeralPointCount(SimulationParameters::MaxEphemeralParticles)
        , mAllPointCount(mAlignedShipPointCount + mEphemeralPointCount)
        //
        , mParentWorld(parentWorld)
        , mMaterialDatabase(materialDatabase)
        , mSimulationEventHandler(simulationEventDispatcher)
        , mShipPhysicsHandler(nullptr)
        , mHaveWholeBuffersBeenUploadedOnce(false)
        , mCurrentNumMechanicalDynamicsIterations(simulationParameters.NumMechanicalDynamicsIterations<float>())
        , mCurrentElasticityAdjustment(simulationParameters.ElasticityAdjustment)
        , mCurrentStaticFrictionAdjustment(simulationParameters.StaticFrictionAdjustment)
        , mCurrentKineticFrictionAdjustment(simulationParameters.KineticFrictionAdjustment)
        , mCurrentOceanFloorElasticityCoefficient(simulationParameters.OceanFloorElasticityCoefficient)
        , mCurrentOceanFloorFrictionCoefficient(simulationParameters.OceanFloorFrictionCoefficient)
        , mCurrentCumulatedIntakenWaterThresholdForAirBubbles(SimulationParameters::AirBubblesDensityToCumulatedIntakenWater(simulationParameters.AirBubblesDensity))
        , mCurrentCombustionSpeedAdjustment(simulationParameters.CombustionSpeedAdjustment)
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
        , mCombustionIgnitionCandidates(mRawShipPointCount)
        , mCombustionExplosionCandidates(mRawShipPointCount)
        , mWaterReactionExplosionCandidates(mRawShipPointCount)
        , mBurningPoints()
        , mStoppedBurningPoints()
        , mFreeEphemeralParticleSearchStartIndex(mAlignedShipPointCount)
        , mAreEphemeralPointElementsDirtyForRendering(false)
#ifdef _DEBUG
        , mDiagnostic_ArePositionsDirty(false)
#endif
    {
        // Add first (implicit) buffer
        mDynamicForceBuffers.emplace_back(mBufferElementCount, shipPointCount, vec2f::zero());
        mDynamicForceRawBuffers.emplace_back(reinterpret_cast<float *>(mDynamicForceBuffers[0].data()));

        CalculateCombustionDecayParameters(mCurrentCombustionSpeedAdjustment, SimulationParameters::ParticleUpdateLowFrequencyStepTimeDuration<float>);
    }

    Points(Points && other) = default;

    /*
     * Returns an iterator for the (unaligned) ship (i.e. non-ephemeral) points only.
     */
    inline auto RawShipPoints() const
    {
        return ElementIndexRangeIterable(0, mRawShipPointCount);
    }

    ElementCount GetRawShipPointCount() const
    {
        return mRawShipPointCount;
    }

    ElementCount GetAlignedShipPointCount() const
    {
        return mAlignedShipPointCount;
    }

    /*
     * Returns a reverse iterator for the (unaligned) ship (i.e. non-ephemeral) points only.
     */
    inline auto RawShipPointsReverse() const
    {
        return ElementIndexReverseRangeIterable(0, mRawShipPointCount);
    }

    /*
     * Returns an iterator for the ephemeral points only.
     */
    inline auto EphemeralPoints() const
    {
        return ElementIndexRangeIterable(mAlignedShipPointCount, mAllPointCount);
    }

    /*
     * Returns a flag indicating whether the point is active in the world.
     *
     * Active points are all non-ephemeral points and non-expired ephemeral points.
     */
    inline bool IsActive(ElementIndex pointIndex) const
    {
        return pointIndex < mRawShipPointCount
            || EphemeralType::None != mEphemeralParticleAttributes1Buffer[pointIndex].Type;
    }

    inline bool IsEphemeral(ElementIndex pointIndex) const
    {
        return pointIndex >= mAlignedShipPointCount;
    }

    Geometry::AABB CalculateAABB() const
    {
        Geometry::AABB box;

        vec2f const * restrict pPtr = mPositionBuffer.data();
        for (ElementIndex pointIndex : RawShipPoints())
        {
            vec2f const pos = pPtr[pointIndex];
            box.ExtendTo(pos);
        }

        return box;
    }

    void RegisterShipPhysicsHandler(IShipPhysicsHandler * shipPhysicsHandler)
    {
        mShipPhysicsHandler = shipPhysicsHandler;
    }

    void Add(
        vec2f const & position,
        float water,
        float internalPressure,
        StructuralMaterial const & structuralMaterial,
        ElectricalMaterial const * electricalMaterial,
        bool isRope,
        float strength,
        ElementIndex electricalElementIndex,
        bool isStructurallyLeaking,
        rgbaColor const & color,
        vec2f const & textureCoordinates,
        float randomNormalizedUniformFloat);

    void CreateEphemeralParticleAirBubble(
        vec2f const & position,
        float depth,
        float finalScale,
        float temperature,
        float buoyancyVolumeFillAdjustment,
        float vortexAmplitude,
        float vortexPeriod,
        float currentSimulationTime,
        PlaneId planeId);

    void CreateEphemeralParticleDebris(
        vec2f const & position,
        vec2f const & velocity,
        float depth,
        float water,
        StructuralMaterial const & structuralMaterial,
        float currentSimulationTime,
        float maxSimulationLifetime,
        PlaneId planeId);

    void CreateEphemeralParticleLightSmoke(
        vec2f const & position,
        float depth,
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters)
    {
        CreateEphemeralParticleSmoke(
            GameTextureDatabases::GenericMipMappedTextureGroups::SmokeLight,
            EphemeralState::SmokeState::GrowthType::Slow,
            position,
            depth,
            temperature,
            currentSimulationTime,
            planeId,
            simulationParameters);
    }

    void CreateEphemeralParticleHeavySmoke(
        vec2f const & position,
        float depth,
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters)
    {
        CreateEphemeralParticleSmoke(
            GameTextureDatabases::GenericMipMappedTextureGroups::SmokeDark,
            EphemeralState::SmokeState::GrowthType::Fast,
            position,
            depth,
            temperature,
            currentSimulationTime,
            planeId,
            simulationParameters);
    }

    void CreateEphemeralParticleSmoke(
        GameTextureDatabases::GenericMipMappedTextureGroups textureGroup,
        EphemeralState::SmokeState::GrowthType growth,
        vec2f const & position,
        float depth,
        float temperature,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters);

    void CreateEphemeralParticleSparkle(
        vec2f const & position,
        vec2f const & velocity,
        StructuralMaterial const & structuralMaterial,
        float depth,
        float currentSimulationTime,
        float maxSimulationLifetime,
        PlaneId planeId);

    void CreateEphemeralParticleWakeBubble(
        vec2f const & position,
        vec2f const & velocity,
        float depth,
        float currentSimulationTime,
        PlaneId planeId,
        SimulationParameters const & simulationParameters);

    void DestroyEphemeralParticle(
        ElementIndex pointElementIndex);

    void Detach(
        ElementIndex pointElementIndex,
        vec2f const & detachVelocity,
        DetachOptions detachOptions,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void Restore(
        ElementIndex pointElementIndex,
        float currentSimulationTime);

    void OnOrphaned(ElementIndex pointElementIndex);

    void UpdateForSimulationParameters(SimulationParameters const & simulationParameters);

    void UpdateCombustionLowFrequency(
        ElementIndex pointOffset,
        ElementIndex pointStride,
        GameWallClock::float_time currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        SimulationParameters const & simulationParameters);

    void UpdateCombustionHighFrequency(
        float currentSimulationTime,
        float dt,
        vec2f const & globalWindSpeed,
        std::optional<Wind::RadialWindField> const & radialWindField,
        SimulationParameters const & simulationParameters);

    void ReorderBurningPointsForDepth();

    void UpdateEphemeralParticles(
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void UpdateHighlights(GameWallClock::float_time currentWallClockTime);

    void Query(ElementIndex pointElementIndex) const;

    // For debugging
    void ColorPointForDebugging(
        ElementIndex pointIndex,
        rgbaColor const & color);

    //
    // Render
    //

    void UploadAttributes(
        ShipId shipId,
        RenderContext & renderContext) const;

    void UploadNonEphemeralPointElements(
        ShipId shipId,
        RenderContext & renderContext) const;

    size_t GetBurningPointCount() const
    {
        return mBurningPoints.size();
    }

    void UploadFlames(ShipRenderContext & shipRenderContext) const;

    void UploadVectors(
        ShipId shipId,
        RenderContext & renderContext) const;

    void UploadEphemeralParticles(
        ShipId shipId,
        RenderContext & renderContext) const;

    void UploadHighlights(
        ShipId shipId,
        RenderContext & renderContext) const;

public:

    //
    // IsDamaged (i.e. whether it has been irrevocable modified, such as detached or
    // set to leaking)
    //

    bool IsDamaged(ElementIndex springElementIndex) const
    {
        return mIsDamagedBuffer[springElementIndex];
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

    vec2f const & GetPosition(ElementIndex pointElementIndex) const noexcept
    {
        return mPositionBuffer[pointElementIndex];
    }

    Buffer<vec2f> const & GetPositionBuffer() const
    {
        return mPositionBuffer;
    }

    vec2f const * GetPositionBufferAsVec2() const
    {
        return mPositionBuffer.data();
    }

    vec2f * GetPositionBufferAsVec2()
    {
        return mPositionBuffer.data();
    }

    float * GetPositionBufferAsFloat()
    {
        return reinterpret_cast<float *>(mPositionBuffer.data());
    }

    std::shared_ptr<Buffer<vec2f>> MakePositionBufferCopy()
    {
        auto positionBufferCopy = mVec2fBufferAllocator.Allocate();
        positionBufferCopy->copy_from(mPositionBuffer);

        return positionBufferCopy;
    }

    void SetPosition(
        ElementIndex pointElementIndex,
        vec2f const & position) noexcept
    {
        mPositionBuffer[pointElementIndex] = position;

#ifdef _DEBUG
        mDiagnostic_ArePositionsDirty = true;
#endif
    }

    vec2f const & GetFactoryPosition(ElementIndex pointElementIndex) const noexcept
    {
        return mFactoryPositionBuffer[pointElementIndex];
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const noexcept
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f const * GetVelocityBufferAsVec2() const
    {
        return mVelocityBuffer.data();
    }

    vec2f * GetVelocityBufferAsVec2()
    {
        return mVelocityBuffer.data();
    }

    float * GetVelocityBufferAsFloat()
    {
        return reinterpret_cast<float *>(mVelocityBuffer.data());
    }

    std::shared_ptr<Buffer<vec2f>> MakeVelocityBufferCopy()
    {
        auto velocityBufferCopy = mVec2fBufferAllocator.Allocate();
        velocityBufferCopy->copy_from(mVelocityBuffer);

        return velocityBufferCopy;
    }

    void SetVelocity(
        ElementIndex pointElementIndex,
        vec2f const & velocity) noexcept
    {
        mVelocityBuffer[pointElementIndex] = velocity;
    }

    vec2f const & GetDynamicForce(ElementIndex pointElementIndex) const noexcept
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        return mDynamicForceBuffers[0][pointElementIndex];
    }

    float * GetDynamicForceBufferAsFloat()
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        return reinterpret_cast<float *>(mDynamicForceBuffers[0].data());
    }

    vec2f * GetDynamicForceBufferAsVec2()
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        return mDynamicForceBuffers[0].data();
    }

    vec2f * GetParallelDynamicForceBuffer(size_t parallelIndex)
    {
        assert(parallelIndex < mDynamicForceBuffers.size());
        return mDynamicForceBuffers[parallelIndex].data();
    }

    float * const * GetDynamicForceBuffersAsFloat()
    {
        return mDynamicForceRawBuffers.data();
    }

    vec2f * const * GetDynamicForceBuffersAsVec2()
    {
        return reinterpret_cast<vec2f * const *>(mDynamicForceRawBuffers.data());
    }

    void SetDynamicForce(
        ElementIndex pointElementIndex,
        vec2f const & force) noexcept
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        mDynamicForceBuffers[0][pointElementIndex] = force;
    }

    void AddDynamicForce(
        ElementIndex pointElementIndex,
        vec2f const & force) noexcept
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        mDynamicForceBuffers[0][pointElementIndex] += force;
    }

    void ResetDynamicForces()
    {
        // First buffer implicitly
        assert(mDynamicForceBuffers.size() >= 1);
        mDynamicForceBuffers[0].fill(vec2f::zero());
    }

    void SetDynamicForceParallelism(size_t parallelism)
    {
        assert(parallelism >= 1);

        // Maintain current buffers' contents, so to save contents of first buffer
        if (parallelism < mDynamicForceBuffers.size())
        {
            while (mDynamicForceBuffers.size() != parallelism)
            {
                mDynamicForceBuffers.pop_back();
                mDynamicForceRawBuffers.pop_back();
            }
        }
        else if (parallelism > mDynamicForceBuffers.size())
        {
            for (size_t b = mDynamicForceBuffers.size(); b < parallelism; ++b)
            {
                mDynamicForceBuffers.emplace_back(mBufferElementCount, vec2f::zero());
                mDynamicForceRawBuffers.emplace_back(reinterpret_cast<float *>(mDynamicForceBuffers.back().data()));
            }
        }
    }

    vec2f const & GetStaticForce(ElementIndex pointElementIndex) const noexcept
    {
        return mStaticForceBuffer[pointElementIndex];
    }

    float * GetStaticForceBufferAsFloat()
    {
        return reinterpret_cast<float *>(mStaticForceBuffer.data());
    }

    vec2f * GetStaticForceBufferAsVec2()
    {
        return mStaticForceBuffer.data();
    }

    void SetStaticForce(
        ElementIndex pointElementIndex,
        vec2f const & force) noexcept
    {
        mStaticForceBuffer[pointElementIndex] = force;
    }

    void AddStaticForce(
        ElementIndex pointElementIndex,
        vec2f const & force) noexcept
    {
        mStaticForceBuffer[pointElementIndex] += force;
    }

    void ResetStaticForces()
    {
        mStaticForceBuffer.fill(vec2f::zero());
    }

    float GetAugmentedMaterialMass(ElementIndex pointElementIndex) const
    {
        return mAugmentedMaterialMassBuffer[pointElementIndex];
    }

    /*
     * Adds a transient mass to the specified particle.
     * The particle's total mass is slowly smoothed to include this one.
     * Reset at end of Ship::Update().
     */
    void AddTransientAdditionalMass(
        ElementIndex pointElementIndex,
        float value)
    {
        mTransientAdditionalMassBuffer[pointElementIndex] += value;
    }

    void ResetTransientAdditionalMasses()
    {
        mTransientAdditionalMassBuffer.fill(0.0f);
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
    float GetMass(ElementIndex pointElementIndex) noexcept
    {
        return mMassBuffer[pointElementIndex];
    }

    void UpdateMasses(SimulationParameters const & simulationParameters);

    float GetStrength(ElementIndex pointElementIndex) const
    {
        return mStrengthBuffer[pointElementIndex];
    }

    float GetStress(ElementIndex pointElementIndex) const
    {
        return mStressBuffer[pointElementIndex];
    }

    void SetStress(
        ElementIndex pointElementIndex,
        float value)
    {
        mStressBuffer[pointElementIndex] = value;
    }

    void ResetStress()
    {
        mStressBuffer.fill(0.0f);
    }

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

    bool IsPinned(ElementIndex pointElementIndex) const
    {
        return (mPinningCoefficientBuffer[pointElementIndex] == 0.0f);
    }

    float const * GetIsPinnedBufferAsFloat() const
    {
        return mPinningCoefficientBuffer.data();
    }

    void Pin(ElementIndex pointElementIndex)
    {
        assert(1.0f == mPinningCoefficientBuffer[pointElementIndex]);

        Freeze(pointElementIndex); // Recalculates integration coefficient
    }

    void Unpin(ElementIndex pointElementIndex)
    {
        assert(0.0f == mPinningCoefficientBuffer[pointElementIndex]);

        Thaw(pointElementIndex); // Recalculates integration coefficient
    }

    OceanFloorCollisionFactors const & GetOceanFloorCollisionFactors(ElementIndex pointElementIndex)
    {
        return mOceanFloorCollisionFactorsBuffer[pointElementIndex];
    }

    float GetAirWaterInterfaceInverseWidth(ElementIndex pointElementIndex)
    {
        return mAirWaterInterfaceInverseWidthBuffer[pointElementIndex];
    }

    BuoyancyCoefficients const & GetBuoyancyCoefficients(ElementIndex pointElementIndex)
    {
        return mBuoyancyCoefficientsBuffer[pointElementIndex];
    }

    /*
     * Valid only when positions haven't changed since the last time depths have been calculated.
     */
    float GetCachedDepth(ElementIndex pointElementIndex) const
    {
#ifdef _DEBUG
        assert(!mDiagnostic_ArePositionsDirty);
#endif
        return mCachedDepthBuffer[pointElementIndex];
    }

    bool IsCachedUnderwater(ElementIndex pointElementIndex) const
    {
#ifdef _DEBUG
        assert(!mDiagnostic_ArePositionsDirty);
#endif
        return mCachedDepthBuffer[pointElementIndex] > 0.0f;
    }

    float * GetCachedDepthBufferAsFloat()
    {
        return mCachedDepthBuffer.data();
    }

    void SwapCachedDepthBuffer(Buffer<float> & other)
    {
        mCachedDepthBuffer.swap(other);
    }

    /*
     * The integration factor is the quantity which, when multiplied with the force on the point,
     * yields the change in position that occurs during a time interval equal to the dynamics simulation step.
     *
     * It basically is:
     *      dt^2 / mass
     *
     * Only valid after a call to UpdateMasses() and when
     * neither water quantities nor masses have changed since then.
     */

    float * GetIntegrationFactorBufferAsFloat()
    {
        return reinterpret_cast<float *>(mIntegrationFactorBuffer.data());
    }

    vec2f * GetIntegrationFactorBufferAsVec2()
    {
        return mIntegrationFactorBuffer.data();
    }

    // Sticky, until NumMechanicalDynamicsIterations changes or PinningCoefficient changes,
    // but we're fine with that as this is only used - at this moment - by MoveGripped
    // interaction
    void SetForcesReceptivity(
        ElementIndex pointElementIndex,
        float receptivity)
    {
        mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = CalculateIntegrationFactorTimeCoefficient(
            mCurrentNumMechanicalDynamicsIterations,
            mPinningCoefficientBuffer[pointElementIndex] * receptivity);
    }

    // Changes the point's dynamics so that it freezes in place
    // and becomes oblivious to forces
    void Freeze(ElementIndex pointElementIndex)
    {
        // Remember this point is now pinned
        mPinningCoefficientBuffer[pointElementIndex] = 0.0f;

        // Recalc integration factor time coefficient, freezing point
        mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = CalculateIntegrationFactorTimeCoefficient(
            mCurrentNumMechanicalDynamicsIterations,
            mPinningCoefficientBuffer[pointElementIndex]);

        // Also zero-out velocity, wiping all traces of this point moving
        mVelocityBuffer[pointElementIndex] = vec2f(0.0f, 0.0f);
    }

    // Changes the point's dynamics so that the point reacts again to forces
    void Thaw(ElementIndex pointElementIndex)
    {
        // This point is not pinned anymore
        mPinningCoefficientBuffer[pointElementIndex] = 1.0f;

        // Re-populate its integration factor time coefficient, thawing point
        mIntegrationFactorTimeCoefficientBuffer[pointElementIndex] = CalculateIntegrationFactorTimeCoefficient(
            mCurrentNumMechanicalDynamicsIterations,
            mPinningCoefficientBuffer[pointElementIndex]);
    }

    //
    // Pressure and water dynamics
    //

    float GetInternalPressure(ElementIndex pointElementIndex) const
    {
        return mInternalPressureBuffer[pointElementIndex];
    }

    void SetInternalPressure(
        ElementIndex pointElementIndex,
        float value)
    {
        mInternalPressureBuffer[pointElementIndex] = value;
    }

    float * GetInternalPressureBufferAsFloat()
    {
        return mInternalPressureBuffer.data();
    }

    bool GetIsHull(ElementIndex pointElementIndex) const
    {
        return mIsHullBuffer[pointElementIndex];
    }

    void SetIsHull(
        ElementIndex pointElementIndex,
        bool value)
    {
        mIsHullBuffer[pointElementIndex] = value;
    }

    bool const * GetIsHullBuffer() const
    {
        return mIsHullBuffer.data();
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

    float GetWater(ElementIndex pointElementIndex) const
    {
        return mWaterBuffer[pointElementIndex];
    }

    void SetWater(
        ElementIndex pointElementIndex,
        float value)
    {
        mWaterBuffer[pointElementIndex] = value;
    }

    float * GetWaterBufferAsFloat()
    {
        return mWaterBuffer.data();
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

    vec2f const & GetWaterVelocity(ElementIndex pointElementIndex) const
    {
        return mWaterVelocityBuffer[pointElementIndex];
    }

    void SetWaterVelocity(
        ElementIndex pointElementIndex,
        vec2f const & waterVelocity)
    {
        mWaterVelocityBuffer[pointElementIndex] = waterVelocity;
    }

    vec2f * GetWaterVelocityBufferAsVec2()
    {
        return mWaterVelocityBuffer.data();
    }

    /*
     * Only valid after a call to UpdateWaterMomentaFromVelocities() and when
     * neither water quantities nor velocities have changed.
     */
    vec2f * GetWaterMomentumBufferAsVec2f()
    {
        return mWaterMomentumBuffer.data();
    }

    void UpdateWaterMomentaFromVelocities()
    {
        float * const restrict waterBuffer = mWaterBuffer.data();
        vec2f * const restrict waterVelocityBuffer = mWaterVelocityBuffer.data();
        vec2f * restrict waterMomentumBuffer = mWaterMomentumBuffer.data();

        // No need to visit ephemerals, as they don't get water
        for (ElementIndex p = 0; p < mRawShipPointCount; ++p)
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

        // No need to visit ephemerals, as they don't get water
        for (ElementIndex p = 0; p < mRawShipPointCount; ++p)
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

    LeakingComposite const & GetLeakingComposite(ElementIndex pointElementIndex) const
    {
        return mLeakingCompositeBuffer[pointElementIndex];
    }

    LeakingComposite & GetLeakingComposite(ElementIndex pointElementIndex)
    {
        return mLeakingCompositeBuffer[pointElementIndex];
    }

    ElementCount GetTotalFactoryWetPoints() const
    {
        return mTotalFactoryWetPoints;
    }

    void Damage(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters)
    {
        // Start structural leaking - but only if the point is originally not hull,
        // as we never allow hull points to take water in
        if (!mMaterialsBuffer[pointElementIndex].Structural->IsHull)
        {
            SetStructurallyLeaking(pointElementIndex);
        }

        // Check if it's the first time we get damaged
        if (!mIsDamagedBuffer[pointElementIndex])
        {
            // Do damage
            InternalDoDamage(pointElementIndex, currentSimulationTime, simulationParameters);

            // Flag ourselves as damaged
            mIsDamagedBuffer[pointElementIndex] = true;
        }
    }

    //
    // Heat dynamics
    //

    float GetTemperature(ElementIndex pointElementIndex) const
    {
        return mTemperatureBuffer[pointElementIndex];
    }

    float * GetTemperatureBufferAsFloat()
    {
        return mTemperatureBuffer.data();
    }

    void SetTemperature(
        ElementIndex pointElementIndex,
        float value)
    {
        mTemperatureBuffer[pointElementIndex] = value;
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

    float GetMaterialHeatCapacityReciprocal(ElementIndex pointElementIndex) const
    {
        return mMaterialHeatCapacityReciprocalBuffer[pointElementIndex];
    }

    float GetMaterialIgnitionTemperature(ElementIndex pointElementIndex) const
    {
        return mMaterialIgnitionTemperatureBuffer[pointElementIndex];
    }

    /*
     * Checks whether a point is simply burning.
     */
    bool IsBurning(ElementIndex pointElementIndex) const
    {
        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        return combustionState == CombustionState::StateType::Burning
            || combustionState == CombustionState::StateType::Developing_1
            || combustionState == CombustionState::StateType::Developing_2;
    }

    /*
     * Checks whether a point is eligible for being extinguished by smothering.
     */
    bool IsBurningForSmothering(ElementIndex pointElementIndex) const
    {
        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        return IsBurning(pointElementIndex)
            || combustionState == CombustionState::StateType::Extinguishing_Consumed;
    }

    /*
     * Checks whether a point is eligible for being subtracted heat during fire extinguishing.
     */
    bool IsBurningForExtinguisherHeatSubtraction(ElementIndex pointElementIndex) const
    {
        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        return IsBurning(pointElementIndex)
            || combustionState == CombustionState::StateType::Extinguishing_Consumed
            || combustionState == CombustionState::StateType::Extinguishing_SmotheredRain
            || combustionState == CombustionState::StateType::Extinguishing_SmotheredWater;
    }

    void SmotherCombustion(
        ElementIndex pointElementIndex,
        bool isWater)
    {
        assert(IsBurningForSmothering(pointElementIndex) || IsBurningForExtinguisherHeatSubtraction(pointElementIndex));

        auto const combustionState = mCombustionStateBuffer[pointElementIndex].State;

        // Notify combustion end - if we are burning
        if (combustionState == CombustionState::StateType::Developing_1
            || combustionState == CombustionState::StateType::Developing_2
            || combustionState == CombustionState::StateType::Burning)
        {
            mSimulationEventHandler.OnPointCombustionEnd();
        }

        // Transition
        mCombustionStateBuffer[pointElementIndex].State = isWater
            ? CombustionState::StateType::Extinguishing_SmotheredWater
            : CombustionState::StateType::Extinguishing_SmotheredRain;

        // Notify sizzling
        mSimulationEventHandler.OnCombustionSmothered();
    }

    void AddHeat(
        ElementIndex pointElementIndex,
        float heat) // J
    {
        mTemperatureBuffer[pointElementIndex] +=
            heat
            * GetMaterialHeatCapacityReciprocal(pointElementIndex);
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

    float * GetLightBufferAsFloat()
    {
        return mLightBuffer.data();
    }

    void ZeroLightBuffer()
    {
        mLightBuffer.fill(0.0f);
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
    // Various interactions
    //

    bool GetIsElectrified(ElementIndex pointElementIndex) const
    {
        return mIsElectrifiedBuffer[pointElementIndex];
    }

    void SetIsElectrified(
        ElementIndex pointElementIndex,
        bool value)
    {
        mIsElectrifiedBuffer[pointElementIndex] = value;
    }

    void ResetIsElectrifiedBuffer()
    {
        mIsElectrifiedBuffer.fill(false);
    }

    //
    // Ephemeral Particles
    //

    EphemeralType GetEphemeralType(ElementIndex pointElementIndex) const
    {
        return mEphemeralParticleAttributes1Buffer[pointElementIndex].Type;
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
        ElementIndex otherEndpointElementIndex)
    {
        assert(mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectedSprings.contains(
            [springElementIndex](auto const & cs)
            {
                return cs.SpringIndex == springElementIndex;
            }));

        // Make it so that a point owns only those springs whose other endpoint comes later
        bool const isAtOwner = pointElementIndex < otherEndpointElementIndex;

        mConnectedSpringsBuffer[pointElementIndex].ConnectSpring(
            springElementIndex,
            otherEndpointElementIndex,
            isAtOwner);
    }

    void DisconnectSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        ElementIndex otherEndpointElementIndex)
    {
        // Make it so that a point owns only those springs whose other endpoint comes later
        bool const isAtOwner = pointElementIndex < otherEndpointElementIndex;

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
        ElementIndex otherEndpointElementIndex)
    {
        // Make it so that a point owns only those springs whose other endpoint comes later
        bool const isAtOwner = pointElementIndex < otherEndpointElementIndex;

        // Add spring to factory-connected springs
        mFactoryConnectedSpringsBuffer[pointElementIndex].ConnectSpring(
            springElementIndex,
            otherEndpointElementIndex,
            isAtOwner);

        // Connect spring
        mConnectedSpringsBuffer[pointElementIndex].ConnectSpring(
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

        assert(!mConnectedTrianglesBuffer[pointElementIndex].ConnectedTriangles.contains(
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

    PlaneId * GetPlaneIdBufferAsPlaneId()
    {
        return mPlaneIdBuffer.data();
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
    // Repair
    //

    RepairState & GetRepairState(ElementIndex pointElementIndex)
    {
        return mRepairStateBuffer[pointElementIndex];
    }

    //
    // Highlights
    //

    void StartElectricalElementHighlight(
        ElementIndex pointElementIndex,
        rgbColor highlightColor,
        GameWallClock::float_time currentWallClockTime)
    {
        // See if we're already highlighting this point
        auto pointIt = std::find_if(
            mElectricalElementHighlightedPoints.begin(),
            mElectricalElementHighlightedPoints.end(),
            [&pointElementIndex](auto const & hs)
            {
                return hs.PointIndex == pointElementIndex;
            });

        if (pointIt != mElectricalElementHighlightedPoints.end())
        {
            // Restart it
            pointIt->Reset(highlightColor, currentWallClockTime);
        }
        else
        {
            // Start new highlight altogether
            mElectricalElementHighlightedPoints.emplace_back(
                pointElementIndex,
                highlightColor,
                currentWallClockTime);
        }
    }

    void StartCircleHighlight(
        ElementIndex pointElementIndex,
        rgbColor highlightColor)
    {
        // See if we're already highlighting this point
        auto pointIt = std::find_if(
            mCircleHighlightedPoints.begin(),
            mCircleHighlightedPoints.end(),
            [&pointElementIndex](auto const & hs)
            {
                return hs.PointIndex == pointElementIndex;
            });

        if (pointIt != mCircleHighlightedPoints.end())
        {
            // Restart it
            pointIt->Reset(highlightColor);
        }
        else
        {
            // Start new highlight altogether
            mCircleHighlightedPoints.emplace_back(
                pointElementIndex,
                highlightColor);
        }
    }

    //
    // Gadgets
    //

    bool IsGadgetAttached(ElementIndex pointElementIndex) const
    {
        return mIsGadgetAttachedBuffer[pointElementIndex];
    }

    void AttachGadget(
        ElementIndex pointElementIndex,
        float mass,
        Springs & springs)
    {
        assert(false == mIsGadgetAttachedBuffer[pointElementIndex]);

        mIsGadgetAttachedBuffer[pointElementIndex] = true;

        // Augment mass due to gadget
        AugmentMaterialMass(
            pointElementIndex,
            mass,
            springs);
    }

    void DetachGadget(
        ElementIndex pointElementIndex,
        Springs & springs)
    {
        assert(true == mIsGadgetAttachedBuffer[pointElementIndex]);

        mIsGadgetAttachedBuffer[pointElementIndex] = false;

        // Reset mass of endpoints

        AugmentMaterialMass(
            pointElementIndex,
            0.0f,
            springs);
    }

    //
    // Randomness
    //

    // [0.0, 1.0]
    float GetRandomNormalizedUniformPersonalitySeed(ElementIndex pointElementIndex) const
    {
        return mRandomNormalizedUniformFloatBuffer[pointElementIndex];
    }

    //
    // Immutable attributes
    //

    vec4f & GetColor(ElementIndex pointElementIndex)
    {
        return mColorBuffer[pointElementIndex];
    }

    vec2f const & GetTextureCoordinates(ElementIndex pointElementIndex) const
    {
        return mTextureCoordinatesBuffer[pointElementIndex];
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

    //
    // Diagnostics
    //

#ifdef _DEBUG

    bool Diagnostic_ArePositionsDirty() const
    {
        return mDiagnostic_ArePositionsDirty;
    }

    void Diagnostic_ClearDirtyPositions()
    {
        mDiagnostic_ArePositionsDirty = false;
    }

    void Diagnostic_MarkPositionsAsDirty()
    {
        mDiagnostic_ArePositionsDirty = true;
    }

#endif

private:

    void InternalDoDamage(
        ElementIndex pointElementIndex,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void CalculateCombustionDecayParameters(
        float combustionSpeedAdjustment,
        float dt);

    static inline float CalculateIntegrationFactorTimeCoefficient(
        float numMechanicalDynamicsIterations,
        float isPinnedCoefficient)
    {
        return SimulationParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations)
            * SimulationParameters::MechanicalSimulationStepTimeDuration<float>(numMechanicalDynamicsIterations)
            * isPinnedCoefficient;
    }

    static inline BuoyancyCoefficients CalculateBuoyancyCoefficients(
        float buoyancyVolumeFill,
        float thermalExpansionCoefficient)
    {
        float const coefficient1 =
            SimulationParameters::GravityMagnitude
            * buoyancyVolumeFill
            * (1.0f - thermalExpansionCoefficient * SimulationParameters::Temperature0);

        float const coefficient2 =
            SimulationParameters::GravityMagnitude
            * buoyancyVolumeFill
            * thermalExpansionCoefficient;

        return BuoyancyCoefficients(
            coefficient1,
            coefficient2);
    }

    static inline OceanFloorCollisionFactors CalculateOceanFloorCollisionFactors(
        float elasticityAdjustment,
        float staticFrictionAdjustment,
        float kineticFrictionAdjustment,
        float oceanFloorElasticityCoefficient,
        float oceanFloorFrictionCoefficient,
        float materialElasticityCoefficient,
        float materialStaticFrictionCoefficient,
        float materialKineticFrictionCoefficient)
    {
        //
        // Somewhat arbitrarily, we use the average of the ocean's and material's coefficients
        //

        return OceanFloorCollisionFactors(
            Clamp(-(materialElasticityCoefficient + oceanFloorElasticityCoefficient) / 2.0f * elasticityAdjustment, -1.0f, 0.0f),
            Clamp(1.0f - (materialStaticFrictionCoefficient + oceanFloorFrictionCoefficient) / 2.0f * staticFrictionAdjustment, 0.0f, 1.0f),
            Clamp(1.0f - (materialKineticFrictionCoefficient + oceanFloorFrictionCoefficient) / 2.0f * kineticFrictionAdjustment, 0.0f, 1.0f));
    }

    static inline float RandomizeCumulatedIntakenWater(float cumulatedIntakenWaterThresholdForAirBubbles)
    {
        return GameRandomEngine::GetInstance().GenerateUniformReal(
            0.0f,
            cumulatedIntakenWaterThresholdForAirBubbles);
    }

    inline void SetStructurallyLeaking(ElementIndex pointElementIndex)
    {
        mLeakingCompositeBuffer[pointElementIndex].LeakingSources.StructuralLeak = 1.0f;

        // Randomize the initial water intaken, so that air bubbles won't come out all at the same moment
        mCumulatedIntakenWater[pointElementIndex] = RandomizeCumulatedIntakenWater(mCurrentCumulatedIntakenWaterThresholdForAirBubbles);
    }

    inline ElementIndex FindFreeEphemeralParticle(
        float currentSimulationTime,
        bool doForce);

    inline void ExpireEphemeralParticle(ElementIndex pointElementIndex)
    {
        // Freeze the particle (just to prevent drifting)
        Freeze(pointElementIndex);

        // Hide this particle from ephemeral particles; this will prevent this particle from:
        // - Being rendered
        // - Being updated
        // ...and it will allow its slot to be chosen for a new ephemeral particle
        mEphemeralParticleAttributes1Buffer[pointElementIndex].Type = EphemeralType::None;
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Damage: true when the point has been irrevocably modified
    // (such as detached or set to leaking); only a Restore will
    // make things right again
    Buffer<bool> mIsDamagedBuffer;

    // Materials
    Buffer<Materials> mMaterialsBuffer;
    Buffer<bool> mIsRopeBuffer;

    //
    // Dynamics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mFactoryPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    std::vector<Buffer<vec2f>> mDynamicForceBuffers; // Forces that vary across the multiple mechanical iterations (i.e. spring, hydrostatic surface pressure) for each thread; always at least one
    std::vector<float *> mDynamicForceRawBuffers;
    Buffer<vec2f> mStaticForceBuffer; // Forces that never change across the multiple mechanical iterations (all other forces)
    Buffer<float> mAugmentedMaterialMassBuffer; // Structural + Offset
    Buffer<float> mTransientAdditionalMassBuffer; // Anything; total mass is slowly updated to include this. Reset at end of Update()
    Buffer<float> mMassBuffer; // Augmented + Transient + Water
    Buffer<float> mMaterialBuoyancyVolumeFillBuffer;
    Buffer<float> mStrengthBuffer; // Immutable
    Buffer<float> mStressBuffer; // -1.0 -> 1.0, only calculated (at springs) if rendering it
    Buffer<float> mDecayBuffer; // 1.0 -> 0.0 (completely decayed)
    bool mutable mIsDecayBufferDirty; // Only tracks non-ephemerals
    Buffer<float> mPinningCoefficientBuffer; // 1.0: not pinned; 0.0f: pinned
    Buffer<float> mIntegrationFactorTimeCoefficientBuffer; // dt^2 or zero when the point is frozen
    Buffer<OceanFloorCollisionFactors> mOceanFloorCollisionFactorsBuffer;
    Buffer<float> mAirWaterInterfaceInverseWidthBuffer; // The reciprocal of the air-water interface, to control the damping we perform against buoyancy oscillations
    Buffer<BuoyancyCoefficients> mBuoyancyCoefficientsBuffer;
    Buffer<float> mCachedDepthBuffer; // Positive when underwater

    Buffer<vec2f> mIntegrationFactorBuffer;

    //
    // Pressure and water dynamics
    //

    Buffer<bool> mIsHullBuffer; // Externally-computed resultant of material hullness and dynamic hullness
    Buffer<float> mInternalPressureBuffer; // Pressure at this particle (Pa)
    Buffer<float> mMaterialWaterIntakeBuffer;
    Buffer<float> mMaterialWaterRestitutionBuffer;
    Buffer<float> mMaterialWaterDiffusionSpeedBuffer;

    // Height of a 1m2 column of water which provides a pressure equivalent to the pressure at
    // this point. Quantity of water is min(water, 1.0)
    Buffer<float> mWaterBuffer;

    // Total velocity of the water at this point
    Buffer<vec2f> mWaterVelocityBuffer;

    // Total momentum of the water at this point
    Buffer<vec2f> mWaterMomentumBuffer;

    // Total amount of water in/out taken which has not yet been
    // utilized for air bubbles
    Buffer<float> mCumulatedIntakenWater;

    // Indicators of point intaking water
    Buffer<LeakingComposite> mLeakingCompositeBuffer;
    Buffer<bool> mFactoryIsStructurallyLeakingBuffer;

    // Total number of points that where wet at factory time
    ElementCount mTotalFactoryWetPoints;

    //
    // Heat dynamics
    //

    Buffer<float> mTemperatureBuffer; // Kelvin
    Buffer<float> mMaterialHeatCapacityReciprocalBuffer;
    Buffer<float> mMaterialThermalExpansionCoefficientBuffer;
    Buffer<float> mMaterialIgnitionTemperatureBuffer;
    Buffer<StructuralMaterial::MaterialCombustionType> mMaterialCombustionTypeBuffer;
    Buffer<CombustionState> mCombustionStateBuffer;

    //
    // Water reaction dynamics
    //

    Buffer<WaterReactionState> mWaterReactionStateBuffer;

    //
    // Electrical dynamics
    //

    // Electrical element (index in ElectricalElements container), if any
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
    // Various interactions
    //

    Buffer<bool> mIsElectrifiedBuffer;

    //
    // Ephemeral particles attributes
    //

    Buffer<EphemeralParticleAttributes1> mEphemeralParticleAttributes1Buffer;
    Buffer<EphemeralParticleAttributes2> mEphemeralParticleAttributes2Buffer;

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
    // Repair state
    //

    Buffer<RepairState> mRepairStateBuffer;

    //
    // Highlights
    //

    std::vector<ElectricalElementHighlightState> mElectricalElementHighlightedPoints;
    std::vector<CircleHighlightState> mCircleHighlightedPoints;

    //
    // Gadgets
    //

    Buffer<bool> mIsGadgetAttachedBuffer;

    //
    // Randomness
    //

    Buffer<float> mRandomNormalizedUniformFloatBuffer; // [0.0, 1.0]

    //
    // Immutable render attributes
    //

    Buffer<vec4f> mColorBuffer;
    bool mutable mIsWholeColorBufferDirty;  // Whether or not whole buffer is dirty since last render upload
    bool mutable mIsEphemeralColorBufferDirty; // Whether or not ephemeral portion of buffer is dirty since last render upload
    Buffer<vec2f> mTextureCoordinatesBuffer;
    bool mutable mIsTextureCoordinatesBufferDirty; // Whether or not is dirty since last render upload

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    // Count of ship points; these are followed by ephemeral points
    ElementCount const mRawShipPointCount;
    ElementCount const mAlignedShipPointCount;

    // Count of ephemeral points
    ElementCount const mEphemeralPointCount;

    // Count of all points (sum of two above, including ship point padding, but not aligned)
    ElementCount const mAllPointCount;

    World & mParentWorld;
    MaterialDatabase const & mMaterialDatabase;
    SimulationEventDispatcher & mSimulationEventHandler;
    IShipPhysicsHandler * mShipPhysicsHandler;

    // Flag remembering whether or not we've uploaded *entire*
    // (as opposed to just non-ephemeral portion) buffers at
    // least once
    bool mutable mHaveWholeBuffersBeenUploadedOnce;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentNumMechanicalDynamicsIterations;
    float mCurrentElasticityAdjustment;
    float mCurrentStaticFrictionAdjustment;
    float mCurrentKineticFrictionAdjustment;
    float mCurrentOceanFloorElasticityCoefficient;
    float mCurrentOceanFloorFrictionCoefficient;
    float mCurrentCumulatedIntakenWaterThresholdForAirBubbles;
    float mCurrentCombustionSpeedAdjustment;

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;

    // The list of candidates for burning and exploding during combustion,
    // and for exploding during a reaction with water;
    // member only to save allocations at use time
    BoundedVector<std::tuple<ElementIndex, float>> mCombustionIgnitionCandidates;
    BoundedVector<std::tuple<ElementIndex, float>> mCombustionExplosionCandidates;
    BoundedVector<std::tuple<ElementIndex, float>> mWaterReactionExplosionCandidates;

    // The indices of the points that are currently burning
    std::vector<ElementIndex> mBurningPoints;

    // The indices of the points that have stopped burning;
    // member only to save allocations at use time
    std::vector<ElementIndex> mStoppedBurningPoints;

    // The index at which to start searching for free ephemeral particles
    // (just an optimization over restarting from zero each time)
    ElementIndex mFreeEphemeralParticleSearchStartIndex;

    // Flag remembering whether the set of ephemeral point *elements* is dirty
    // (i.e. whether there are more or less points than previously
    // reported to the rendering engine); only tracks dirtyness
    // of ephemeral types that are uploaded as ephemeral point *elements*
    // (thus no AirBubbles nor Sparkles, which are both uploaded specially)
    bool mutable mAreEphemeralPointElementsDirtyForRendering;

    // Calculated constants for combustion decay
    float mCombustionDecayAlphaFunctionA;
    float mCombustionDecayAlphaFunctionB;
    float mCombustionDecayAlphaFunctionC;

#ifdef _DEBUG
    bool mDiagnostic_ArePositionsDirty;
#endif
};

}

template <> struct is_flag<Physics::Points::DetachOptions> : std::true_type {};
