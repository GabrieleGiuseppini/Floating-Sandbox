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
        , mTypeBuffer(mBufferElementCount, mElementCount, ElectricalMaterial::ElectricalElementType::Cable)
        , mLuminiscenceBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mLightColorBuffer(mBufferElementCount, mElementCount, vec4f::zero())
        , mLightSpreadBuffer(mBufferElementCount, mElementCount, 0.0f)
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
        , mGenerators()
        , mLamps()
    {
    }

    ElectricalElements(ElectricalElements && other) = default;

    /*
     * Returns an iterator for the generator elements only.
     */
    inline auto const & Generators() const
    {
        return mGenerators;
    }

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

    void Update(
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

    inline ElectricalMaterial::ElectricalElementType GetType(ElementIndex electricalElementIndex) const
    {
        return mTypeBuffer[electricalElementIndex];
    }

    //
    // Light
    //

    inline float GetLuminiscence(ElementIndex electricalElementIndex) const
    {
        assert(ElectricalMaterial::ElectricalElementType::Lamp == GetType(electricalElementIndex));
        return mLuminiscenceBuffer[electricalElementIndex];
    }

    inline float GetLightSpread(ElementIndex electricalElementIndex) const
    {
        assert(ElectricalMaterial::ElectricalElementType::Lamp == GetType(electricalElementIndex));
        return mLightSpreadBuffer[electricalElementIndex];
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

    //
    // Connectivity detection visit sequence number
    //

    inline SequenceNumber GetCurrentConnectivityVisitSequenceNumber(ElementIndex electricalElementIndex) const
    {
        return mCurrentConnectivityVisitSequenceNumberBuffer[electricalElementIndex];
    }

    inline void SetConnectivityVisitSequenceNumber(
        ElementIndex electricalElementIndex,
        SequenceNumber connectivityVisitSequenceNumber)
    {
        mCurrentConnectivityVisitSequenceNumberBuffer[electricalElementIndex] =
            connectivityVisitSequenceNumber;
    }

private:

    union ElementState
    {
        struct CableState
        {
        };

        struct GeneratorState
        {
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
                , WetFailureRateCdf(1.0f - exp(-wetFailureRate/60.0f))
                , State(StateType::Initial)
                , FlickerCounter(0u)
                , NextStateTransitionTimePoint()
                , NextWetFailureCheckTimePoint()
            {}
        };

        CableState Cable;
        GeneratorState Generator;
        LampState Lamp;

        ElementState(CableState cable)
            : Cable(cable)
        {}

        ElementState(GeneratorState generator)
            : Generator(generator)
        {}

        ElementState(LampState lamp)
            : Lamp(lamp)
        {}
    };

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
    Buffer<ElectricalMaterial::ElectricalElementType> mTypeBuffer;

    // Light
    Buffer<float> mLuminiscenceBuffer;
    Buffer<vec4f> mLightColorBuffer;
    Buffer<float> mLightSpreadBuffer;

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
    std::vector<ElementIndex> mGenerators;
    std::vector<ElementIndex> mLamps;
};

}
