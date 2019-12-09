/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Materials.h"

#include <GameCore/Buffer.h>
#include <GameCore/ElementContainer.h>
#include <GameCore/GameWallClock.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>

using namespace std::chrono_literals;

namespace Physics
{

class ElectricalElements : public ElementContainer
{
private:

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
        };

        struct OtherSinkState
        {
            bool IsPowered;

            OtherSinkState(bool isPowered)
                : IsPowered(isPowered)
            {}
        };

        CableState Cable;
        GeneratorState Generator;
        LampState Lamp;
        OtherSinkState OtherSink;

        ElementState(CableState cable)
            : Cable(cable)
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
    };

public:

    ElectricalElements(
        ElementCount allElementCount,
        ElementCount lampElementCount,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        GameParameters const & gameParameters)
        : ElementContainer(allElementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        , mPointIndexBuffer(mBufferElementCount, mElementCount, NoneElementIndex)
        , mMaterialTypeBuffer(mBufferElementCount, mElementCount, ElectricalMaterial::ElectricalElementType::Cable)
        , mMaterialHeatGeneratedBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialOperatingTemperaturesBuffer(mBufferElementCount, mElementCount, OperatingTemperatures(0.0f, 0.0f))
        , mMaterialLuminiscenceBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mMaterialLightColorBuffer(mBufferElementCount, mElementCount, vec4f::zero())
        , mMaterialLightSpreadBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mConnectedElectricalElementsBuffer(mBufferElementCount, mElementCount, FixedSizeVector<ElementIndex, 8U>())
        , mElementStateBuffer(mBufferElementCount, mElementCount, ElementState::CableState())
        , mAvailableLightBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, mElementCount, SequenceNumber())
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
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(nullptr)
        , mSources()
        , mSinks()
        , mLamps()
        , mCurrentLightSpreadAdjustment(gameParameters.LightSpreadAdjustment)
        , mCurrentLuminiscenceAdjustment(gameParameters.LuminiscenceAdjustment)
    {
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
        ElectricalMaterial const & electricalMaterial);

    void Destroy(ElementIndex electricalElementIndex);

    void UpdateForGameParameters(GameParameters const & gameParameters);

    void UpdateSourcesAndPropagation(
        SequenceNumber newConnectivityVisitSequenceNumber,
        Points & points,
        GameParameters const & gameParameters);

    void UpdateSinks(
        GameWallClock::time_point currentWallclockTime,
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

    inline auto const & GetConnectedElectricalElements(ElementIndex electricalElementIndex) const
    {
        return mConnectedElectricalElementsBuffer[electricalElementIndex];
    }

    inline void AddConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex)
    {
        assert(connectedElectricalElementIndex < mElementCount);

        mConnectedElectricalElementsBuffer[electricalElementIndex].push_back(connectedElectricalElementIndex);
    }

    inline void RemoveConnectedElectricalElement(
        ElementIndex electricalElementIndex,
        ElementIndex connectedElectricalElementIndex)
    {
        assert(connectedElectricalElementIndex < mElementCount);

        bool found = mConnectedElectricalElementsBuffer[electricalElementIndex].erase_first(connectedElectricalElementIndex);

        assert(found);
        (void)found;
    }

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

    // Type
    Buffer<ElectricalMaterial::ElectricalElementType> mMaterialTypeBuffer;

    // Properties
    Buffer<float> mMaterialHeatGeneratedBuffer;
    Buffer<OperatingTemperatures> mMaterialOperatingTemperaturesBuffer;

    // Light
    Buffer<float> mMaterialLuminiscenceBuffer;
    Buffer<vec4f> mMaterialLightColorBuffer;
    Buffer<float> mMaterialLightSpreadBuffer;

    // Connected elements
    Buffer<FixedSizeVector<ElementIndex, 8U>> mConnectedElectricalElementsBuffer;

    // Element state
    Buffer<ElementState> mElementStateBuffer;

    // Available light (from lamps)
    Buffer<float> mAvailableLightBuffer;

    // Connectivity detection visit sequence number
    Buffer<SequenceNumber> mCurrentConnectivityVisitSequenceNumberBuffer;

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

    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;
    IShipPhysicsHandler * mShipPhysicsHandler;

    // Indices of specific types in this container - just a shortcut
    std::vector<ElementIndex> mSources;
    std::vector<ElementIndex> mSinks;
    std::vector<ElementIndex> mLamps;

    // The game parameter values that we are current with; changes
    // in the values of these parameters will trigger a re-calculation
    // of pre-calculated coefficients
    float mCurrentLightSpreadAdjustment;
    float mCurrentLuminiscenceAdjustment;
};

}
