/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ElementContainer.h"
#include "GameWallClock.h"
#include "Material.h"

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
        std::shared_ptr<IGameEventHandler> gameEventHandler)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        , mIsDeletedBuffer(mBufferElementCount, mElementCount, true)
        , mPointIndexBuffer(mBufferElementCount, mElementCount, NoneElementIndex)
        , mTypeBuffer(mBufferElementCount, mElementCount, Material::ElectricalProperties::ElectricalElementType::Cable)
        , mConnectedElectricalElementsBuffer(mBufferElementCount, mElementCount, FixedSizeVector<ElementIndex, 8u>())
        , mElementStateBuffer(mBufferElementCount, mElementCount, ElementState::CableState())
        , mAvailableCurrentBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mCurrentConnectivityVisitSequenceNumberBuffer(mBufferElementCount, mElementCount, NoneVisitSequenceNumber)
        //////////////////////////////////
        // Container
        //////////////////////////////////
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mDestroyHandler()
        , mGenerators()
        , mLamps()
    {
    }

    ElectricalElements(ElectricalElements && other) = default;

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
        Material::ElectricalProperties::ElectricalElementType elementType,
        bool isSelfPowered);

    void Destroy(ElementIndex electricalElementIndex);

    void Update(
        VisitSequenceNumber currentConnectivityVisitSequenceNumber,
        Points const & points,
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

    inline Material::ElectricalProperties::ElectricalElementType GetType(ElementIndex electricalElementIndex) const
    {
        return mTypeBuffer[electricalElementIndex];
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
    // Available Current
    //

    inline float GetAvailableCurrent(ElementIndex electricalElementIndex) const
    {
        return mAvailableCurrentBuffer[electricalElementIndex];
    }

    //
    // Connectivity detection step sequence number
    //

    inline VisitSequenceNumber GetCurrentConnectivityVisitSequenceNumber(ElementIndex electricalElementIndex) const
    {
        return mCurrentConnectivityVisitSequenceNumberBuffer[electricalElementIndex];
    }

    inline void SetConnectivityVisitSequenceNumber(
        ElementIndex electricalElementIndex,
        VisitSequenceNumber connectivityVisitSequenceNumber)
    {
        mCurrentConnectivityVisitSequenceNumberBuffer[electricalElementIndex] =
            connectivityVisitSequenceNumber;
    }

    //
    // Subsets
    //

    inline auto const & GetGenerators() const
    {
        return mGenerators;
    }

    inline auto const & GetLamps() const
    {
        return mLamps;
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
           
            LampState(bool isSelfPowered)
                : IsSelfPowered(isSelfPowered)
                , State(StateType::Initial)
                , FlickerCounter(0u)
                , NextStateTransitionTimePoint()
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
        VisitSequenceNumber currentConnectivityVisitSequenceNumber,
        Points const & points,
        GameParameters const & gameParameters);

    inline vec2f const & GetPosition(
        ElementIndex elementLampIndex,
        Points const & points) const
    {
        return points.GetPosition(mPointIndexBuffer[elementLampIndex]);
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
    Buffer<Material::ElectricalProperties::ElectricalElementType> mTypeBuffer;

    // Connected elements
    Buffer<FixedSizeVector<ElementIndex, 8U>> mConnectedElectricalElementsBuffer;

    // Element state
    Buffer<ElementState> mElementStateBuffer;

    // Available current (to lamps)
    Buffer<float> mAvailableCurrentBuffer;

    // Connectivity detection step sequence number
    Buffer<VisitSequenceNumber> mCurrentConnectivityVisitSequenceNumberBuffer;

    //////////////////////////////////////////////////////////
    // Container 
    //////////////////////////////////////////////////////////

    World & mParentWorld;
    std::shared_ptr<IGameEventHandler> const mGameEventHandler;

    // The handler registered for electrical element deletions
    DestroyHandler mDestroyHandler;

    // Indices of specific types in this container - just a shortcut
    std::vector<ElementIndex> mGenerators;
    std::vector<ElementIndex> mLamps;
};

}
