/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

constexpr float LampWetFailureWaterThreshold = 0.1f;

void ElectricalElements::Add(
    ElementIndex pointElementIndex,
    ElectricalMaterial const & electricalMaterial)
{
    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mTypeBuffer.emplace_back(electricalMaterial.ElectricalType);
    mLuminiscenceBuffer.emplace_back(electricalMaterial.Luminiscence);
    mLightColorBuffer.emplace_back(electricalMaterial.LightColor);
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
            mElementStateBuffer.emplace_back(
                ElementState::LampState(
                    electricalMaterial.IsSelfPowered,
                    electricalMaterial.WetFailureRate));
            break;
        }
    }

    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();
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
    Points & points,
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
    Points & points,
    GameParameters const & /*gameParameters*/)
{
    //
    // Normal lamp, only on if visited of self-powered, and controlled by flicker state machine
    //

    auto const pointIndex = GetPointIndex(elementLampIndex);

    auto & lamp = mElementStateBuffer[elementLampIndex].Lamp;

    switch (lamp.State)
    {
        case ElementState::LampState::StateType::Initial:
        {
            // Transition to ON - if we have current or if we're self-powered
            if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
            {
                mAvailableCurrentBuffer[elementLampIndex] = 1.f;
                lamp.State = ElementState::LampState::StateType::LightOn;
                lamp.NextWetFailureCheckTimePoint = currentWallclockTime + std::chrono::seconds(1);
            }
            else
            {
                // Transition to OFF
                mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                lamp.State = ElementState::LampState::StateType::LightOff;
            }

            break;
        }

        case ElementState::LampState::StateType::LightOn:
        {
            // Check whether we still have current, or we're wet and it's time to fail
            if ((   currentConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                    && !lamp.IsSelfPowered
                ) ||
                (   points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold)
                    && CheckWetFailureTime(lamp, currentWallclockTime)
                ))
            {
                //
                // Start flicker state machine
                //

                mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                // Transition state, choose whether to A or B
                lamp.FlickerCounter = 0u;
                lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerStartInterval;
                if (GameRandomEngine::GetInstance().Choose(2) == 0)
                    lamp.State = ElementState::LampState::StateType::FlickerA;
                else
                    lamp.State = ElementState::LampState::StateType::FlickerB;
            }

            break;
        }

        case ElementState::LampState::StateType::FlickerA:
        {
            // 0-1-0-1-Off

            // Check if we should become ON again
            if ((currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold))
            {
                mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                // Transition state
                lamp.State = ElementState::LampState::StateType::LightOn;
            }
            else if (currentWallclockTime > lamp.NextStateTransitionTimePoint)
            {
                ++lamp.FlickerCounter;

                if (1 == lamp.FlickerCounter
                    || 3 == lamp.FlickerCounter)
                {
                    // Flicker to on, for a short time

                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    mGameEventHandler->OnLightFlicker(
                        DurationShortLongType::Short,
                        mParentWorld.IsUnderwater(points.GetPosition(pointIndex)),
                        1);

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                }
                else if (2 == lamp.FlickerCounter)
                {
                    // Flicker to off, for a short time

                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                }
                else
                {
                    assert(4 == lamp.FlickerCounter);

                    // Transition to off for good
                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                    lamp.State = ElementState::LampState::StateType::LightOff;
                }
            }

            break;
        }

        case ElementState::LampState::StateType::FlickerB:
        {
            // 0-1-0-1--0-1-Off

            // Check if we should become ON again
            if ((currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold))
            {
                mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                // Transition state
                lamp.State = ElementState::LampState::StateType::LightOn;
            }
            else if (currentWallclockTime > lamp.NextStateTransitionTimePoint)
            {
                ++lamp.FlickerCounter;

                if (1 == lamp.FlickerCounter
                    || 5 == lamp.FlickerCounter)
                {
                    // Flicker to on, for a short time

                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    mGameEventHandler->OnLightFlicker(
                        DurationShortLongType::Short,
                        mParentWorld.IsUnderwater(points.GetPosition(pointIndex)),
                        1);

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerBInterval;
                }
                else if (2 == lamp.FlickerCounter
                        || 4 == lamp.FlickerCounter)
                {
                    // Flicker to off, for a short time

                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerBInterval;
                }
                else if (3 == lamp.FlickerCounter)
                {
                    // Flicker to on, for a longer time

                    mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                    mGameEventHandler->OnLightFlicker(
                        DurationShortLongType::Long,
                        mParentWorld.IsUnderwater(points.GetPosition(pointIndex)),
                        1);

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + 2 * ElementState::LampState::FlickerBInterval;
                }
                else
                {
                    assert(6 == lamp.FlickerCounter);

                    // Transition to off for good
                    mAvailableCurrentBuffer[elementLampIndex] = 0.f;
                    lamp.State = ElementState::LampState::StateType::LightOff;
                }
            }

            break;
        }

        case ElementState::LampState::StateType::LightOff:
        {
            assert(mAvailableCurrentBuffer[elementLampIndex] == 0.f);

            // Check if we should become ON again
            if ((currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold))
            {
                mAvailableCurrentBuffer[elementLampIndex] = 1.f;

                mGameEventHandler->OnLightFlicker(
                    DurationShortLongType::Short,
                    mParentWorld.IsUnderwater(points.GetPosition(pointIndex)),
                    1);

                // Transition state
                lamp.State = ElementState::LampState::StateType::LightOn;
            }

            break;
        }
    }
}

bool ElectricalElements::CheckWetFailureTime(
    ElementState::LampState & lamp,
    GameWallClock::time_point currentWallclockTime)
{
    bool isFailure = false;

    if (currentWallclockTime >= lamp.NextWetFailureCheckTimePoint)
    {
        // Sample the CDF
       isFailure =
            GameRandomEngine::GetInstance().GenerateRandomNormalizedReal()
            < lamp.WetFailureRateCdf;

        // Schedule next check
        lamp.NextWetFailureCheckTimePoint = currentWallclockTime + std::chrono::seconds(1);
    }

    return isFailure;
}

}