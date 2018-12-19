/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "GameRandomEngine.h"

namespace Physics {

void ElectricalElements::Add(
    ElementIndex pointElementIndex,
    ElectricalMaterial const & electricalMaterial)
{
    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mTypeBuffer.emplace_back(electricalMaterial.ElectricalType);
    mLuminiscenceBuffer.emplace_back(electricalMaterial.Luminiscence);
    mLightSpreadBuffer.emplace_back(electricalMaterial.LightSpread);
    mConnectedElectricalElementsBuffer.emplace_back();
    mAvailableCurrentBuffer.emplace_back(0.f);

    switch (electricalMaterial.ElectricalType)
    {
        case ElectricalMaterial::ElectricalElementType::Cable:
        {
            mElementStateBuffer.emplace_back(ElementState::CableState());
            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            mGenerators.emplace_back(static_cast<ElementIndex>(mElementStateBuffer.GetCurrentPopulatedSize()));
            mElementStateBuffer.emplace_back(ElementState::GeneratorState());
            break;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            mLamps.emplace_back(static_cast<ElementIndex>(mElementStateBuffer.GetCurrentPopulatedSize()));
            mElementStateBuffer.emplace_back(ElementState::LampState(electricalMaterial.IsSelfPowered));
            break;
        }
    }

    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back(NoneVisitSequenceNumber);
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    assert(electricalElementIndex < mElementCount);
    assert(!IsDeleted(electricalElementIndex));

    // Zero out our current
    mAvailableCurrentBuffer[electricalElementIndex] = 0.0f;

    // Note: no need to remove self from connected electrical elements, as Ship's PointDestroyHandler,
    // which is the caller of this Destroy(), has already destroyed the point's springs, hence
    // this electrical element has no connected points anymore already and viceversa
    assert(GetConnectedElectricalElements(electricalElementIndex).empty());

    // Invoke destroy handler
    if (!!mDestroyHandler)
    {
        mDestroyHandler(electricalElementIndex);
    }

    // Flag ourselves as deleted
    mIsDeletedBuffer[electricalElementIndex] = true;
}

void ElectricalElements::Update(
    GameWallClock::time_point currentWallclockTime,
    VisitSequenceNumber currentConnectivityVisitSequenceNumber,
    Points const & points,
    GameParameters const & gameParameters)
{
    //
    // Visit all lamps and run their state machine
    //

    for (auto iLamp : Lamps())
    {
        if (!mIsDeletedBuffer[iLamp])
        {
            RunLampStateMachine(
                iLamp,
                currentWallclockTime,
                currentConnectivityVisitSequenceNumber,
                points,
                gameParameters);
        }
        else
        {
            assert(0.0f == mAvailableCurrentBuffer[iLamp]);
        }
    }
}

void ElectricalElements::RunLampStateMachine(
    ElementIndex elementLampIndex,
    GameWallClock::time_point currentWallclockTime,
    VisitSequenceNumber currentConnectivityVisitSequenceNumber,
    Points const & points,
    GameParameters const & /*gameParameters*/)
{
    if (mElementStateBuffer[elementLampIndex].Lamp.IsSelfPowered)
    {
        //
        // Self-powered lamp, always on
        //

        mAvailableCurrentBuffer[elementLampIndex] = 1.0f;
    }
    else
    {
        //
        // Normal lamp, only on if visited, and controlled by flicker state machine
        //

        switch (mElementStateBuffer[elementLampIndex].Lamp.State)
        {
            case ElementState::LampState::StateType::Initial:
            {
                // Check whether we have current
                if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex])
                {
                    // Transition to ON
                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;
                    mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOn;
                }
                else
                {
                    // Transition to OFF
                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                    mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOff;
                }

                break;
            }

            case ElementState::LampState::StateType::LightOn:
            {
                // Check whether we still have current
                if (currentConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex])
                {
                    //
                    // Start flicker state machine
                    //

                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                    // Transition state, choose whether to A or B
                    mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter = 0u;
                    mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerStartInterval;
                    if (GameRandomEngine::GetInstance().Choose(2) == 0)
                        mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::FlickerA;
                    else
                        mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::FlickerB;
                }

                break;
            }

            case ElementState::LampState::StateType::FlickerA:
            {
                // Check if current started flowing again, by any chance
                if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex])
                {
                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    // Transition state
                    mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOn;
                }
                else if (currentWallclockTime > mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint)
                {
                    ++mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter;

                    if (1 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter
                        || 3 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter)
                    {
                        // Flicker to on, for a short time

                        mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                        mGameEventHandler->OnLightFlicker(
                            DurationShortLongType::Short,
                            mParentWorld.IsUnderwater(GetPosition(elementLampIndex, points)),
                            1);

                        mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                    }
                    else if (2 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter)
                    {
                        // Flicker to off, for a short time

                        mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                        mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                    }
                    else
                    {
                        assert(4 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter);

                        // Transition to off for good
                        mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                        mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOff;
                    }
                }

                break;
            }

            case ElementState::LampState::StateType::FlickerB:
            {
                // Check if current started flowing again, by any chance
                if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex])
                {
                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    // Transition state
                    mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOn;
                }
                else if (currentWallclockTime > mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint)
                {
                    ++mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter;

                    if (1 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter
                        || 5 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter)
                    {
                        // Flicker to on, for a short time

                        mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                        mGameEventHandler->OnLightFlicker(
                            DurationShortLongType::Short,
                            mParentWorld.IsUnderwater(GetPosition(elementLampIndex, points)),
                            1);

                        mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerBInterval;
                    }
                    else if (2 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter
                            || 4 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter)
                    {
                        // Flicker to off, for a short time

                        mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                        mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerBInterval;
                    }
                    else if (3 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter)
                    {
                        // Flicker to on, for a longer time

                        mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                        mGameEventHandler->OnLightFlicker(
                            DurationShortLongType::Long,
                            mParentWorld.IsUnderwater(GetPosition(elementLampIndex, points)),
                            1);

                        mElementStateBuffer[elementLampIndex].Lamp.NextStateTransitionTimePoint = currentWallclockTime + 2 * ElementState::LampState::FlickerBInterval;
                    }
                    else
                    {
                        assert(6 == mElementStateBuffer[elementLampIndex].Lamp.FlickerCounter);

                        // Transition to off for good
                        mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                        mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOff;
                    }
                }

                break;
            }

            case ElementState::LampState::StateType::LightOff:
            {
                assert(mAvailableCurrentBuffer[elementLampIndex] == 0.f);

                // Check if current started flowing again, by any chance
                if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex])
                {
                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    mGameEventHandler->OnLightFlicker(
                        DurationShortLongType::Short,
                        mParentWorld.IsUnderwater(GetPosition(elementLampIndex, points)),
                        1);

                    // Transition state
                    mElementStateBuffer[elementLampIndex].Lamp.State = ElementState::LampState::StateType::LightOn;
                }

                break;
            }
        }
    }
}

}