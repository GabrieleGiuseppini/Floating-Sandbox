/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/Buffer.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/FixedSizeVector.h>
#include <GameCore/GameWallClock.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

using namespace std::chrono_literals;

namespace Physics
{

class ElectricalElements : public ElementContainer
{
private:

    /*
     * The information we maintain with each instanced element.
     */
    struct InstanceInfo
    {
        ElectricalElementInstanceIndex InstanceIndex;
        std::optional<ElectricalPanelElementMetadata> PanelElementMetadata;

        InstanceInfo(
            ElectricalElementInstanceIndex instanceIndex,
            std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata)
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
            float ThrustCapacity;
            float Responsiveness;

            vec2f CurrentThrustVector; // Normalized
            float CurrentRpm;
            float CurrentThrustMagnitude;
            float LastPublishedRpm;
            float LastPublishedThrustMagnitude;

            EngineState(
                float thrustCapacity,
                float responsiveness)
                : ThrustCapacity(thrustCapacity)
                , Responsiveness(responsiveness)
                , LastPublishedRpm(0.0f)
                , LastPublishedThrustMagnitude(0.0f)
            {
                ResetCurrent();
            }

            void ResetCurrent()
            {
                CurrentThrustVector = vec2f::zero();
                CurrentRpm = 0.0f;
                CurrentThrustMagnitude = 0.0f;
            }
        };

        struct EngineControllerState
        {
            struct ConnectedEngine
            {
                ElementIndex EngineElectricalElementIndex;
                float SinEngineCWAngle;
                float CosEngineCWAngle;

                ConnectedEngine()
                    : EngineElectricalElementIndex(NoneElementIndex)
                    , SinEngineCWAngle(0.0f)
                    , CosEngineCWAngle(0.0f)
                {}

                ConnectedEngine(
                    ElementIndex engineElectricalElementIndex,
                    float engineCWAngle) // CW angle between engine direction and engine->controller vector
                    : EngineElectricalElementIndex(engineElectricalElementIndex)
                    , SinEngineCWAngle(std::sin(engineCWAngle))
                    , CosEngineCWAngle(std::cos(engineCWAngle))
                {}
            };

            FixedSizeVector<ConnectedEngine, GameParameters::MaxSpringsPerPoint> ConnectedEngines; // Immutable

            int CurrentTelegraphValue; // Between -Degrees/2 and +Degrees/2
            bool IsPowered;

            EngineControllerState(int telegraphValue, bool isPowered)
                : ConnectedEngines()
                , CurrentTelegraphValue(telegraphValue)
                , IsPowered(isPowered)
            {}
        };

        struct GeneratorState
        {
            bool IsProducingCurrent;

            GeneratorState(bool isProducingCurrent)
                : IsProducingCurrent(isProducingCurrent)
            {}
        };

        struct LampState
        {
            bool IsSelfPowered;

            // Probability that light fails within 1 second
            float WetFailureRateCdf;

            enum class StateType
            {
                Initial, // At ship load
                LightOn,
                FlickerA,
                FlickerB,
                LightOff
            };

            static constexpr auto FlickerStartInterval = 100ms;
            static constexpr auto FlickerAInterval = 150ms;
            static constexpr auto FlickerBInterval = 100ms;

            StateType State;
            std::uint8_t FlickerCounter;
            GameWallClock::time_point NextStateTransitionTimePoint;
            GameWallClock::time_point NextWetFailureCheckTimePoint;

            LampState(
                bool isSelfPowered,
                float wetFailureRate)
                : IsSelfPowered(isSelfPowered)
                , WetFailureRateCdf(1.0f - exp(-wetFailureRate / 60.0f))
                , State(StateType::Initial)
                , FlickerCounter(0u)
                , NextStateTransitionTimePoint()
                , NextWetFailureCheckTimePoint()
            {}

            void Reset()
            {
                State = StateType::Initial;
                FlickerCounter = 0;
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

        struct DummyState
        {};

        CableState Cable;
        EngineState Engine;
        EngineControllerState EngineController;
        GeneratorState Generator;
        LampState Lamp;
        OtherSinkState OtherSink;
        PowerMonitorState PowerMonitor;
        SmokeEmitterState SmokeEmitter;
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

        ElementState(SmokeEmitterState smokeEmitter)
            : SmokeEmitter(smokeEmitter)
        {}

        ElementState(DummyState dummy)
            : Dummy(dummy)
        {}
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
        , mEngineSinks()
        , mCurrentLightSpreadAdjustment(gameParameters.LightSpreadAdjustment)
        , mCurrentLuminiscenceAdjustment(gameParameters.LuminiscenceAdjustment)
        , mHasSwitchBeenToggledInStep(false)
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
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata,
        ElectricalMaterial const & electricalMaterial);

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

    void SetSwitchState(
        ElectricalElementId electricalElementId,
        ElectricalState switchState,
        Points & points,
        GameParameters const & gameParameters);

    void SetEngineControllerState(
        ElectricalElementId electricalElementId,
        int telegraphValue,
        GameParameters const & gameParameters);

    void Destroy(ElementIndex electricalElementIndex);

    void Restore(ElementIndex electricalElementIndex);

    void UpdateForGameParameters(GameParameters const & gameParameters);

    void UpdateAutomaticConductivityToggles(
        Points & points,
        GameParameters const & gameParameters);

    void UpdateSourcesAndPropagation(
        SequenceNumber newConnectivityVisitSequenceNumber,
        Points & points,
        GameParameters const & gameParameters);

    void UpdateSinks(
        GameWallClock::time_point currentWallclockTime,
        float currentSimulationTime,
        SequenceNumber currentConnectivityVisitSequenceNumber,
        Points & points,
        GameParameters const & gameParameters);

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
    }

    inline void RemoveConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex)
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

        (void)found;
    }

    void AddFactoryConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex,
        Octant octant);

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
    inline float GetLampRawDistanceCoefficient(ElementIndex electricalElementIndex) const
    {
        return mLampRawDistanceCoefficientBuffer[electricalElementIndex];
    }

    /*
     * Gets the spread coefficients for all lamps as a buffer; size is
     * BufferLampCount.
     */
    float * restrict GetLampLightSpreadMaxDistanceBufferAsFloat()
    {
        return mLampLightSpreadMaxDistanceBuffer.data();
    }

    /*
     * Gets a work buffer for populating lamp positions; size is
     * BufferLampCount.
     */
    Buffer<vec2f> & GetLampPositionWorkBuffer()
    {
        return mLampPositionWorkBuffer;
    }

    /*
     * Gets a work buffer for populating lamp plane IDs; size is
     * BufferLampCount.
     */
    Buffer<PlaneId> & GetLampPlaneIdWorkBuffer()
    {
        return mLampPlaneIdWorkBuffer;
    }

    /*
     * Gets a work buffer for populating complete distance coefficients; size is
     * BufferLampCount.
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

    void RunLampStateMachine(
        ElementIndex elementLampIndex,
        GameWallClock::time_point currentWallclockTime,
        SequenceNumber currentConnectivityVisitSequenceNumber,
        Points & points,
        GameParameters const & gameParameters);

    bool CheckWetFailureTime(
        ElementState::LampState & lamp,
        GameWallClock::time_point currentWallclockTime);

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
    std::vector<ElementIndex> mEngineSinks;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentLightSpreadAdjustment;
    float mCurrentLuminiscenceAdjustment;

    // Flag indicating whether or not a switch has been toggled
    // during the current simulation step; cleared at the end
    // of sinks' update.
    // Used to distinguish malfunctions from explicit actions.
    // A bit of a hack, as the sink that gets a power state toggle
    // won't know for sure if that's because of a switch toggle,
    // but the real problem is in practice also ambiguous, and
    // this is good enough.
    bool mHasSwitchBeenToggledInStep;
};

}
