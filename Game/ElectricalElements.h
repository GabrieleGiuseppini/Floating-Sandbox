/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ElectricalPanel.h"
#include "Materials.h"

#include <GameCore/Buffer.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameWallClock.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

using namespace std::chrono_literals;

namespace Physics
{

class ElectricalElements : public ElementContainer
{
private:

    /* "Engine groups" are sets of engines connected to each other
     * (via engine transmissions, engines, and controllers)
     */
    using EngineGroupIndex = std::uint32_t;

    /*
     * The information we maintain with each instanced element.
     */
    struct InstanceInfo
    {
        ElectricalElementInstanceIndex InstanceIndex;
        std::optional<ElectricalPanel::ElementMetadata> PanelElementMetadata;

        InstanceInfo(
            ElectricalElementInstanceIndex instanceIndex,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata)
            : InstanceIndex(instanceIndex)
            , PanelElementMetadata(panelElementMetadata)
        {}
    };

    struct Conductivity
    {
        bool MaterialConductsElectricity;
        bool ConductsElectricity;

        explicit Conductivity(bool materialConductsElectricity)
            : MaterialConductsElectricity(materialConductsElectricity)
            , ConductsElectricity(materialConductsElectricity) // Init with material's
        {}
    };

    struct OperatingTemperatures
    {
        float MinOperatingTemperature;
        float MaxOperatingTemperature;

        OperatingTemperatures(
            float minOperatingTemperature,
            float maxOperatingTemperature)
            : MinOperatingTemperature(minOperatingTemperature)
            , MaxOperatingTemperature(maxOperatingTemperature)
        {}

        inline bool IsInRange(float temperature) const
        {
            float constexpr OuterWatermarkOffset = 10.0f;
            return temperature >= MinOperatingTemperature - OuterWatermarkOffset
                && temperature <= MaxOperatingTemperature + OuterWatermarkOffset;
        }

        inline bool IsBackInRange(float temperature) const
        {
            float constexpr InnerWatermarkOffset = 10.0f;
            return temperature >= MinOperatingTemperature + InnerWatermarkOffset
                && temperature <= MaxOperatingTemperature - InnerWatermarkOffset;
        }
    };

    union ElementState
    {
        struct CableState
        {
        };

        struct EngineState
        {
            float CCWDirection;
            float ThrustCapacity;
            float Responsiveness;

            ElementIndex ReferencePointIndex;
            // CW angle between engine direction and engine->reference_point vector
            float ReferencePointCWAngleCos;
            float ReferencePointCWAngleSin;

            float CurrentAbsRpm;
            float CurrentThrustMagnitude;
            vec2f CurrentThrustDir;
            vec2f CurrentJetEngineFlameVector;

            float LastPublishedAbsRpm;
            float LastPublishedThrustMagnitude;
            float LastHighlightedRpm;

            std::optional<float> SuperElectrificationSimulationTimestampEnd;

            SequenceNumber EngineConnectivityVisitSequenceNumber;
            EngineGroupIndex EngineGroup;

            EngineState(
                float ccwDirection,
                float thrustCapacity,
                float responsiveness)
                : CCWDirection(ccwDirection)
                , ThrustCapacity(thrustCapacity)
                , Responsiveness(responsiveness)
                , EngineConnectivityVisitSequenceNumber()
            {
                Reset();
            }

            void Reset()
            {
                ReferencePointIndex = NoneElementIndex;
                ReferencePointCWAngleCos = 0.0f;
                ReferencePointCWAngleSin = 0.0f;

                CurrentAbsRpm = 0.0f;
                CurrentThrustMagnitude = 0.0f;
                CurrentThrustDir = vec2f::zero();
                CurrentJetEngineFlameVector = vec2f::zero();

                LastPublishedAbsRpm = 0.0f;
                LastPublishedThrustMagnitude = 0.0f;
                LastHighlightedRpm = 0.0f;

                SuperElectrificationSimulationTimestampEnd.reset();

                EngineGroup = 0;
            }
        };

        struct EngineControllerState
        {
            float CurrentValue; // Between -1.0 and +1.0
            bool IsPowered;

            SequenceNumber EngineConnectivityVisitSequenceNumber;
            EngineGroupIndex EngineGroup;

            EngineControllerState(float currentValue, bool isPowered)
                : CurrentValue(currentValue)
                , IsPowered(isPowered)
                , EngineConnectivityVisitSequenceNumber()
                , EngineGroup(0)
            {}
        };

        struct EngineTransmissionState
        {
            SequenceNumber EngineConnectivityVisitSequenceNumber;

            EngineTransmissionState()
                : EngineConnectivityVisitSequenceNumber()
            {}
        };

        struct GeneratorState
        {
            bool IsProducingCurrent;
            std::optional<float> DisabledSimulationTimestampEnd;

            GeneratorState(bool isProducingCurrent)
                : IsProducingCurrent(isProducingCurrent)
            {}

            void Reset()
            {
                DisabledSimulationTimestampEnd.reset();
            }
        };

        struct LampState
        {
            ElementIndex LampElementIndex;

            bool IsSelfPowered;

            // Probability that light fails within 1 second
            float WetFailureRateCdf;

            // Max external pressure before breaking; in Pa
            float ExternalPressureBreakageThreshold;

            enum class StateType
            {
                Initial, // At ship load
                LightOn,
                FlickerA,
                FlickerB,
                FlickerOvercharge,
                LightOff,
                ImplosionLeadIn,
                Implosion,
                DisabledLeadIn1,
                DisabledLeadIn2
            };

            static constexpr auto FlickerStartInterval = 100ms;
            static constexpr auto FlickerAInterval = 150ms;
            static constexpr auto FlickerBInterval = 100ms;

            StateType State;
            std::uint8_t SubStateCounter;
            GameWallClock::time_point NextStateTransitionTimePoint;
            GameWallClock::time_point NextWetFailureCheckTimePoint;
            std::optional<float> DisabledSimulationTimestampEnd;

            LampState(
                ElementIndex lampElementIndex,
                bool isSelfPowered,
                float wetFailureRate,
                float externalPressureBreakageThreshold)
                : LampElementIndex(lampElementIndex)
                , IsSelfPowered(isSelfPowered)
                , WetFailureRateCdf(1.0f - exp(-wetFailureRate / 60.0f))
                , ExternalPressureBreakageThreshold(externalPressureBreakageThreshold)
                , State(StateType::Initial)
                , SubStateCounter(0u)
                , NextStateTransitionTimePoint()
                , NextWetFailureCheckTimePoint()
                , DisabledSimulationTimestampEnd()
            {}

            void Reset()
            {
                State = StateType::Initial;
                SubStateCounter = 0;
                DisabledSimulationTimestampEnd.reset();
            }
        };

        struct OtherSinkState
        {
            bool IsPowered;

            OtherSinkState(bool isPowered)
                : IsPowered(isPowered)
            {}
        };

        struct PowerMonitorState
        {
            bool IsPowered;

            PowerMonitorState(bool isPowered)
                : IsPowered(isPowered)
            {}
        };

        struct ShipSoundState
        {
            bool IsSelfPowered;
            bool IsPlaying; // Current announced playing state

            // The toggle status is encoded in the conductivity buffer

            ShipSoundState(
                bool isSelfPowered,
                bool isPlaying)
                : IsSelfPowered(isSelfPowered)
                , IsPlaying(isPlaying)
            {}
        };

        struct SmokeEmitterState
        {
            float EmissionRate;
            bool IsOperating;
            float NextEmissionSimulationTimestamp; // When zero, it will be calculated

            SmokeEmitterState(
                float emissionRate,
                bool isOperating)
                : EmissionRate(emissionRate)
                , IsOperating(isOperating)
                , NextEmissionSimulationTimestamp(0.0f)
            {}
        };

        struct WaterPumpState
        {
            float NominalForce;
            float TargetNormalizedForce; // 0.0...1.0; <> 0.0 <=> powered, == 0.0 <=> not powered
            float CurrentNormalizedForce;
            float LastPublishedNormalizedForce;

            WaterPumpState(float nominalForce)
                : NominalForce(nominalForce)
                , TargetNormalizedForce(0.0f)
                , CurrentNormalizedForce(0.0f)
                , LastPublishedNormalizedForce(0.0f)
            {}
        };

        struct WaterSensingSwitchState
        {
            float GracePeriodEndSimulationTime;

            WaterSensingSwitchState()
                : GracePeriodEndSimulationTime(0.0f)
            {}
        };

        struct WatertightDoorState
        {
            bool IsActivated; // Current state: operating when true, not operating when false
            bool DefaultIsOpen; // Door state when inactive

            WatertightDoorState(
                bool isActivated,
                bool defaultIsOpen)
                : IsActivated(isActivated)
                , DefaultIsOpen(defaultIsOpen)
            {}

            bool IsOpen() const
            {
                return IsActivated == false
                    ? DefaultIsOpen
                    : !DefaultIsOpen;
            }
        };

        struct DummyState
        {};

        CableState Cable;
        EngineState Engine;
        EngineControllerState EngineController;
        EngineTransmissionState EngineTransmission;
        GeneratorState Generator;
        LampState Lamp;
        OtherSinkState OtherSink;
        PowerMonitorState PowerMonitor;
        ShipSoundState ShipSound;
        SmokeEmitterState SmokeEmitter;
        WaterPumpState WaterPump;
        WaterSensingSwitchState WaterSensingSwitch;
        WatertightDoorState WatertightDoor;
        DummyState Dummy;

        ElementState(CableState cable)
            : Cable(cable)
        {}

        ElementState(EngineState engine)
            : Engine(engine)
        {}

        ElementState(EngineControllerState engineController)
            : EngineController(engineController)
        {}

        ElementState(EngineTransmissionState engineTransmissionState)
            : EngineTransmission(engineTransmissionState)
        {}

        ElementState(GeneratorState generator)
            : Generator(generator)
        {}

        ElementState(LampState lamp)
            : Lamp(lamp)
        {}

        ElementState(OtherSinkState otherSink)
            : OtherSink(otherSink)
        {}

        ElementState(PowerMonitorState powerMonitor)
            : PowerMonitor(powerMonitor)
        {}

        ElementState(ShipSoundState shipSound)
            : ShipSound(shipSound)
        {}

        ElementState(SmokeEmitterState smokeEmitter)
            : SmokeEmitter(smokeEmitter)
        {}

        ElementState(WaterPumpState waterPump)
            : WaterPump(waterPump)
        {}

        ElementState(WaterSensingSwitchState waterSensingSwitch)
            : WaterSensingSwitch(waterSensingSwitch)
        {}

        ElementState(WatertightDoorState watertightDoor)
            : WatertightDoor(watertightDoor)
        {}

        ElementState(DummyState dummy)
            : Dummy(dummy)
        {}
    };

    struct EngineGroupState
    {
        float GroupRpm; // Signed
        float GroupThrustMagnitude; // Signed

        EngineGroupState()
            : GroupRpm(0.0f)
            , GroupThrustMagnitude(0.0f)
        {}
    };

    /*
     * The possible reasons for which power to sinks may disappear
     * because of failures.
     */
    enum class PowerFailureReason
    {
        ElectricSpark,
        PowerSourceFlood,
        Other
    };

    enum class LampOffSequenceType
    {
        Flicker,
        Overcharge
    };

public:

    enum class DestroyReason
    {
        LampImplosion,
        LampExplosion,
        SilentRemoval,
        Other
    };

public:

    ElectricalElements(
        ElementCount allElementCount,
        ElementCount lampElementCount,
        ShipId shipId,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters)
        : ElementContainer(allElementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        , mPointIndexBuffer(mBufferElementCount, mElementCount, NoneElementIndex)
        , mMaterialBuffer(mBufferElementCount, mElementCount, nullptr)
        , mMaterialTypeBuffer(mBufferElementCount, mElementCount, ElectricalMaterial::ElectricalElementType::Cable)
        , mConductivityBuffer(mBufferElementCount, mElementCount, Conductivity(false))
        , mMaterialHeatGeneratedBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialOperatingTemperaturesBuffer(mBufferElementCount, mElementCount, OperatingTemperatures(0.0f, 0.0f))
        , mMaterialLuminiscenceBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialLightColorBuffer(mBufferElementCount, mElementCount, vec4f::zero())
        , mMaterialLightSpreadBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mConnectedElectricalElementsBuffer(mBufferElementCount, mElementCount, FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint>())
        , mConductingConnectedElectricalElementsBuffer(mBufferElementCount, mElementCount, FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint>())
        , mElementStateBuffer(mBufferElementCount, mElementCount, ElementState::CableState())
        , mEngineGroupStates()
        , mAvailableLightBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, mElementCount, SequenceNumber())
        , mInstanceInfos()
        //////////////////////////////////
        // Lamps
        //////////////////////////////////
        , mBufferLampCount(make_aligned_float_element_count(lampElementCount))
        , mLampRawDistanceCoefficientBuffer(mBufferLampCount, lampElementCount, 0.0f)
        , mLampLightSpreadMaxDistanceBuffer(mBufferLampCount, lampElementCount, 0.0f)
        , mLampPositionWorkBuffer(mBufferLampCount, lampElementCount, vec2f::zero())
        , mLampPlaneIdWorkBuffer(mBufferLampCount, lampElementCount, 0)
        , mLampDistanceCoefficientWorkBuffer(mBufferLampCount, lampElementCount, 0.0f)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mShipId(shipId)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(nullptr)
        , mAutomaticConductivityTogglingElements()
        , mSources()
        , mSinks()
        , mLamps()
        , mEngineControllers()
        , mEngines()
        , mJetEnginesSortedByPlaneId()
        , mCurrentLightSpreadAdjustment(gameParameters.LightSpreadAdjustment)
        , mCurrentLuminiscenceAdjustment(gameParameters.LuminiscenceAdjustment)
        , mHasConnectivityStructureChangedInCurrentStep(true)
        , mPowerFailureReasonInCurrentStep()
    {
        mInstanceInfos.reserve(mElementCount);
    }

    ElectricalElements(ElectricalElements && other) = default;

    /*
     * Returns an iterator for the lamp elements only.
     */
    inline auto const & Lamps() const
    {
        return mLamps;
    }

    void RegisterShipPhysicsHandler(IShipPhysicsHandler * shipPhysicsHandler)
    {
        mShipPhysicsHandler = shipPhysicsHandler;
    }

    void Add(
        ElementIndex pointElementIndex,
        ElectricalElementInstanceIndex instanceIndex,
        std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata,
        ElectricalMaterial const & electricalMaterial,        
        bool flipH,
        bool flipV,
        bool rotate90CW,
        Points const & points);

    void AnnounceInstancedElements();

    void HighlightElectricalElement(
        ElectricalElementId electricalElementId,
        Points & points)
    {
        assert(electricalElementId.GetShipId() == mShipId);

        HighlightElectricalElement(
            electricalElementId.GetLocalObjectId(),
            points);
    }

    void HighlightElectricalElement(
        ElementIndex elementIndex,
        Points & points);

    void Query(ElementIndex electricalElementIndex) const;

    void SetSwitchState(
        ElectricalElementId electricalElementId,
        ElectricalState switchState,
        Points & points,
        GameParameters const & gameParameters);

    void SetEngineControllerState(
        ElectricalElementId electricalElementId,
        float controllerValue,
        GameParameters const & gameParameters);

    void Destroy(
        ElementIndex electricalElementIndex,
        DestroyReason reason,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void Restore(ElementIndex electricalElementIndex);

    void OnPhysicalStructureChanged(Points const & points);

    void OnElectricSpark(
        ElementIndex electricalElementIndex,
        float currentSimulationTime,
        GameParameters const & gameParameters);

    void UpdateForGameParameters(GameParameters const & gameParameters);

    void Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        SequenceNumber newConnectivityVisitSequenceNumber,
        Points & points,
        Springs const & springs,
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void Upload(
        Render::ShipRenderContext & shipRenderContext,
        Points const & points) const;

public:

    //
    // IsDeleted
    //

    inline bool IsDeleted(ElementIndex electricalElementIndex) const
    {
        return mIsDeletedBuffer[electricalElementIndex];
    }

    //
    // Point
    //

    inline ElementIndex GetPointIndex(ElementIndex electricalElementIndex) const
    {
        return mPointIndexBuffer[electricalElementIndex];
    }

    inline ElementIndex GetPointIndex(ElectricalElementId electricalElementId) const
    {
        return mPointIndexBuffer[electricalElementId.GetLocalObjectId()];
    }

    //
    // Type
    //

    inline ElectricalMaterial::ElectricalElementType GetMaterialType(ElementIndex electricalElementIndex) const
    {
        return mMaterialTypeBuffer[electricalElementIndex];
    }

    //
    // Connected elements
    //

    auto const & GetConnectedElectricalElements(ElementIndex electricalElementIndex) const
    {
        return mConnectedElectricalElementsBuffer[electricalElementIndex];
    }

    auto const & GetConductingConnectedElectricalElements(ElementIndex electricalElementIndex) const
    {
        return mConductingConnectedElectricalElementsBuffer[electricalElementIndex];
    }

    inline bool AreConnected(
        ElementIndex electricalElementIndexA,
        ElementIndex electricalElementIndexB)
    {
        bool const areConnected = mConnectedElectricalElementsBuffer[electricalElementIndexA].contains(electricalElementIndexB);
        assert(areConnected == mConnectedElectricalElementsBuffer[electricalElementIndexB].contains(electricalElementIndexA));
        return areConnected;
    }

    inline void AddConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex)
    {
        assert(!mConnectedElectricalElementsBuffer[electricalElementIndex].contains(connectedElectricalElementIndex));
        mConnectedElectricalElementsBuffer[electricalElementIndex].push_back(connectedElectricalElementIndex);

        // If both elements conduct electricity (at this moment), then connect them also electrically
        assert(!mConductingConnectedElectricalElementsBuffer[electricalElementIndex].contains(connectedElectricalElementIndex));
        if (mConductivityBuffer[electricalElementIndex].ConductsElectricity
            && mConductivityBuffer[connectedElectricalElementIndex].ConductsElectricity)
        {
            mConductingConnectedElectricalElementsBuffer[electricalElementIndex].push_back(connectedElectricalElementIndex);
            // Other connection will be done when AddConnectedElectricalElement is invoked on the other
        }

        // Remember that connectivity structure has changed during this step
        mHasConnectivityStructureChangedInCurrentStep = true;
    }

    inline void RemoveConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex,
        bool hasBeenSevered)
    {
        bool found = mConnectedElectricalElementsBuffer[electricalElementIndex].erase_first(connectedElectricalElementIndex);
        assert(found);

        // Unconditionally remove from conducting and connected elements
        // (it's there in case they're both conducting at this moment)
        found = mConductingConnectedElectricalElementsBuffer[electricalElementIndex].erase_first(connectedElectricalElementIndex);
        assert(!mConductivityBuffer[electricalElementIndex].ConductsElectricity
            || !mConductivityBuffer[connectedElectricalElementIndex].ConductsElectricity
            || found);

        // Other connection will be severed when RemoveConnectedElectricalElement is invoked on the other

        // Remember that connectivity structure has changed during this step
        mHasConnectivityStructureChangedInCurrentStep = true;

        if (hasBeenSevered)
        {
            // Remember that power has been severed during this step
            mPowerFailureReasonInCurrentStep = PowerFailureReason::Other;
        }

        (void)found;
    }

    void AddFactoryConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex);

    //
    // Available Light
    //

    /*
     * Gets the fraction of the element's luminiscence that is actually available after current
     * propagation.
     */
    inline float GetAvailableLight(ElementIndex electricalElementIndex) const
    {
        return mAvailableLightBuffer[electricalElementIndex];
    }

    //
    // Lamps
    //

    /*
     * Gets the actual number of lamps.
     */
    inline ElementIndex GetLampCount() const
    {
        return static_cast<ElementIndex >(mLamps.size());
    }

    /*
     * Gets the number of lamps rounded up to the number of vectorization elements,
     * which is the number of elements in the lamp buffers.
     */
    inline ElementIndex GetBufferLampCount() const
    {
        return mBufferLampCount;
    }

    /*
     * Gets the lamp's light distance coefficient, net of the lamp's available light.
     */
    inline float GetLampRawDistanceCoefficient(ElementIndex lampElementIndex) const
    {
        return mLampRawDistanceCoefficientBuffer[lampElementIndex];
    }

    /*
     * Gets the spread coefficients for all lamps as a buffer; size is
     * BufferLampCount.
     */
    float * GetLampLightSpreadMaxDistanceBufferAsFloat()
    {
        return mLampLightSpreadMaxDistanceBuffer.data();
    }

    /*
     * Gets a work buffer for populating lamp positions.
     *
     * Size is BufferLampCount, padded to vectorization float count.
     */
    Buffer<vec2f> & GetLampPositionWorkBuffer()
    {
        return mLampPositionWorkBuffer;
    }

    /*
     * Gets a work buffer for populating lamp plane IDs.
     *
     * Size is BufferLampCount, padded to vectorization float count.
     */
    Buffer<PlaneId> & GetLampPlaneIdWorkBuffer()
    {
        return mLampPlaneIdWorkBuffer;
    }

    /*
     * Gets a work buffer for populating complete distance coefficients.
     *
     * Size is BufferLampCount, padded to vectorization float count.
     */
    Buffer<float> & GetLampDistanceCoefficientWorkBuffer()
    {
        return mLampDistanceCoefficientWorkBuffer;
    }

private:

    void InternalSetSwitchState(
        ElementIndex elementIndex,
        ElectricalState switchState,
        Points & points,
        GameParameters const & gameParameters);

    void InternalChangeConductivity(
        ElementIndex elementIndex,
        bool value);

    void UpdateEngineConductivity(
        SequenceNumber newConnectivityVisitSequenceNumber,
        Points const & points,
        Springs const & springs);

    void UpdateAutomaticConductivityToggles(
        float currentSimulationTime,
        Points & points,
        GameParameters const & gameParameters);

    void UpdateSourcesAndPropagation(
        float currentSimulationTime,
        SequenceNumber newConnectivityVisitSequenceNumber,
        Points & points,
        GameParameters const & gameParameters);

    void UpdateSinks(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        SequenceNumber currentConnectivityVisitSequenceNumber,
        Points & points,
        float effectiveAirDensity,
        float effectiveWaterDensity,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters);

    void RunLampStateMachine(
        bool isConnectedToPower,
        std::optional<LampOffSequenceType> const & powerFailureSequenceType,
        ElementIndex elementLampIndex,
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Points & points,
        GameParameters const & gameParameters);

    bool CheckWetFailureTime(
        ElementState::LampState & lamp,
        GameWallClock::time_point currentWallClockTime);

    static inline float CalculateLampLightSpreadMaxDistance(
        float materialLightSpread,
        float lightSpreadAdjustment)
    {
        return materialLightSpread
            * lightSpreadAdjustment
            + 0.5f; // To ensure spread=0 => lamp is lighted
    }

    static inline float CalculateLampRawDistanceCoefficient(
        float materialLuminiscence,
        float luminiscenceAdjustment,
        float lampLightSpreadMaxDistance)
    {
        // This coefficient pre-calculates part of lum*(spread-distance)/spread
        return materialLuminiscence
            * luminiscenceAdjustment
            / lampLightSpreadMaxDistance;
    }

    inline void RecalculateLampCoefficients(ElementIndex elementIndex)
    {
        CalculateLampCoefficients(
            elementIndex,
            mCurrentLightSpreadAdjustment,
            mCurrentLuminiscenceAdjustment);
    }

    inline void CalculateLampCoefficients(
        ElementIndex elementIndex,
        float lightSpreadAdjustment,
        float luminiscenceAdjustment)
    {
        ElementIndex const lampElementIndex = mElementStateBuffer[elementIndex].Lamp.LampElementIndex;

        float const lampLightSpreadMaxDistance = CalculateLampLightSpreadMaxDistance(
            mMaterialLightSpreadBuffer[elementIndex],
            lightSpreadAdjustment);

        mLampRawDistanceCoefficientBuffer[lampElementIndex] = CalculateLampRawDistanceCoefficient(
            mMaterialLuminiscenceBuffer[elementIndex],
            luminiscenceAdjustment,
            lampLightSpreadMaxDistance);

        mLampLightSpreadMaxDistanceBuffer[lampElementIndex] = lampLightSpreadMaxDistance;
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    // Deletion
    Buffer<bool> mIsDeletedBuffer;

    // Point
    Buffer<ElementIndex> mPointIndexBuffer;

    // Material
    Buffer<ElectricalMaterial const *> mMaterialBuffer;
    Buffer<ElectricalMaterial::ElectricalElementType> mMaterialTypeBuffer;

    // Conductivity
    Buffer<Conductivity> mConductivityBuffer;

    // Heat
    Buffer<float> mMaterialHeatGeneratedBuffer;
    Buffer<OperatingTemperatures> mMaterialOperatingTemperaturesBuffer;

    // Light
    Buffer<float> mMaterialLuminiscenceBuffer;
    Buffer<vec4f> mMaterialLightColorBuffer;
    Buffer<float> mMaterialLightSpreadBuffer;

    // Currently-connected elements - changes with structural changes
    Buffer<FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint>> mConnectedElectricalElementsBuffer;

    // Currently-conducting and connected elements - changes with structural changes and toggles
    Buffer<FixedSizeVector<ElementIndex, GameParameters::MaxSpringsPerPoint>> mConductingConnectedElectricalElementsBuffer;

    // Element state
    Buffer<ElementState> mElementStateBuffer;

    // Engine groups - member only for allocation efficiency;
    // dimensioned at end of each engine connectivity update,
    // as number of engine groups + 1
    std::vector<EngineGroupState> mEngineGroupStates;

    // Available light (from lamps)
    Buffer<float> mAvailableLightBuffer;

    // Connectivity detection visit sequence number
    Buffer<SequenceNumber> mCurrentConnectivityVisitSequenceNumberBuffer;

    // Instance info's - one for each element
    std::vector<InstanceInfo> mInstanceInfos;

    //////////////////////////////////////////////////////////
    // Lamps
    //////////////////////////////////////////////////////////

    ElementIndex const mBufferLampCount;

    Buffer<float> mLampRawDistanceCoefficientBuffer; // EffectiveLampLight / LampLightSpreadMaxDistance
    Buffer<float> mLampLightSpreadMaxDistanceBuffer;
    Buffer<vec2f> mLampPositionWorkBuffer;
    Buffer<PlaneId> mLampPlaneIdWorkBuffer;
    Buffer<float> mLampDistanceCoefficientWorkBuffer;

    //////////////////////////////////////////////////////////
    // Container
    //////////////////////////////////////////////////////////

    ShipId const mShipId;
    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;
    IShipPhysicsHandler * mShipPhysicsHandler;

    // Indices of specific types in this container - just a shortcut
    std::vector<ElementIndex> mAutomaticConductivityTogglingElements;
    std::vector<ElementIndex> mSources;
    std::vector<ElementIndex> mSinks;
    std::vector<ElementIndex> mLamps;
    std::vector<ElementIndex> mEngineControllers;
    std::vector<ElementIndex> mEngines;

    // Subset of mEngines index vector, only for jets *and* sorted by particle Plane ID;
    // never changes size, only order
    std::vector<ElementIndex> mJetEnginesSortedByPlaneId;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentLightSpreadAdjustment;
    float mCurrentLuminiscenceAdjustment;

    // Flag indicating that the connectivity structure has changed during
    // the current simulation step; triggers re-calculations that only need
    // to happen at these changes
    bool mHasConnectivityStructureChangedInCurrentStep;

    // Flag indicating the cause of a power failure during the current
    // simulation step; cleared at the end of sinks' update.
    // Set only when there's been a failure; not set if power disappears
    // because of a user action (e.g. switch toggle).
    // Used to distinguish outage due to failures from simple switch toggles.
    // A bit of a hack, as the sink that gets a power state toggle
    // won't know for sure if that's because of a switch toggle,
    // but the real problem is in practice also ambiguous, and
    // this is good enough.
    std::optional<PowerFailureReason> mPowerFailureReasonInCurrentStep;
};

}
