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
public:

    using DestroyHandler = std::function<void(ElementIndex)>;

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
        ElementCount elementCount,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
        : ElementContainer(elementCount)
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
        , mConnectedElectricalElementsBuffer(mBufferElementCount, mElementCount, {})
        , mElementStateBuffer(mBufferElementCount, mElementCount, ElementState::CableState())
        , mAvailableLightBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, mElementCount, SequenceNumber())
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mDestroyHandler()
        , mSources()
        , mSinks()
        , mLamps()
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

    /*
     * Sets a (single) handler that is invoked whenever an electrical element is destroyed.
     *
     * The handler is invoked right before the electrical element is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted electrical element might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other electrical elements from it is not supported
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
        ElementIndex pointElementIndex,
        ElectricalMaterial const & electricalMaterial);

    void Destroy(ElementIndex electricalElementIndex);

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
    // Light
    //

    inline float GetMaterialLuminiscence(ElementIndex electricalElementIndex) const
    {
        assert(ElectricalMaterial::ElectricalElementType::Lamp == GetType(electricalElementIndex));
        return mMaterialLuminiscenceBuffer[electricalElementIndex];
    }

    inline float GetMaterialLightSpread(ElementIndex electricalElementIndex) const
    {
        assert(ElectricalMaterial::ElectricalElementType::Lamp == GetType(electricalElementIndex));
        return mMaterialLightSpreadBuffer[electricalElementIndex];
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

    inline float GetAvailableLight(ElementIndex electricalElementIndex) const
    {
        return mAvailableLightBuffer[electricalElementIndex];
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
    // Container
    //////////////////////////////////////////////////////////

    World & mParentWorld;
    std::shared_ptr<GameEventDispatcher> const mGameEventHandler;

    // The handler registered for electrical element deletions
    DestroyHandler mDestroyHandler;

    // Indices of specific types in this container - just a shortcut
    std::vector<ElementIndex> mSources;
    std::vector<ElementIndex> mSinks;
    std::vector<ElementIndex> mLamps;
};

}
