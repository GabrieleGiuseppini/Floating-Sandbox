/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

#include <queue>

namespace Physics {

constexpr float LampWetFailureWaterThreshold = 0.1f;

void ElectricalElements::Add(
    ElementIndex pointElementIndex,
    ElectricalMaterial const & electricalMaterial)
{
    ElementIndex const elementIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mMaterialTypeBuffer.emplace_back(electricalMaterial.ElectricalType);
    mMaterialHeatGeneratedBuffer.emplace_back(electricalMaterial.HeatGenerated);
    mMaterialOperatingTemperaturesBuffer.emplace_back(
        electricalMaterial.MinimumOperatingTemperature,
        electricalMaterial.MaximumOperatingTemperature);
    mMaterialLuminiscenceBuffer.emplace_back(electricalMaterial.Luminiscence);
    mMaterialLightColorBuffer.emplace_back(electricalMaterial.LightColor);
    mMaterialLightSpreadBuffer.emplace_back(electricalMaterial.LightSpread);
    mConnectedElectricalElementsBuffer.emplace_back();
    mAvailableLightBuffer.emplace_back(0.f);

    switch (electricalMaterial.ElectricalType)
    {
        case ElectricalMaterial::ElectricalElementType::Cable:
        {
            mElementStateBuffer.emplace_back(ElementState::CableState());
            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            mSources.emplace_back(elementIndex);
            mElementStateBuffer.emplace_back(ElementState::GeneratorState(true));
            break;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            mSinks.emplace_back(elementIndex);
            mLamps.emplace_back(elementIndex);
            mElementStateBuffer.emplace_back(
                ElementState::LampState(
                    electricalMaterial.IsSelfPowered,
                    electricalMaterial.WetFailureRate));
            break;
        }

        case ElectricalMaterial::ElectricalElementType::OtherSink:
        {
            mSinks.emplace_back(elementIndex);
            mElementStateBuffer.emplace_back(ElementState::OtherSinkState(false));
            break;
        }
    }

    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();

    //
    // Lamp
    //

    if (ElectricalMaterial::ElectricalElementType::Lamp == electricalMaterial.ElectricalType)
    {
        float const lampLightSpreadMaxDistance = CalculateLampLightSpreadMaxDistance(
            electricalMaterial.LightSpread,
            mCurrentLightSpreadAdjustment);

        mLampRawDistanceCoefficientBuffer.emplace_back(
            CalculateLampRawDistanceCoefficient(
                electricalMaterial.Luminiscence,
                mCurrentLuminiscenceAdjustment,
                lampLightSpreadMaxDistance));

        mLampLightSpreadMaxDistanceBuffer.emplace_back(lampLightSpreadMaxDistance);
    }
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    assert(electricalElementIndex < mElementCount);
    assert(!IsDeleted(electricalElementIndex));

    // Zero out our light
    mAvailableLightBuffer[electricalElementIndex] = 0.0f;

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

void ElectricalElements::UpdateForGameParameters(GameParameters const & gameParameters)
{
    //
    // Recalculate lamp coefficients, if needed
    //

    if (gameParameters.LightSpreadAdjustment != mCurrentLightSpreadAdjustment
        || gameParameters.LuminiscenceAdjustment != mCurrentLuminiscenceAdjustment)
    {
        for (size_t l = 0; l < mLamps.size(); ++l)
        {
            float const lampLightSpreadMaxDistance = CalculateLampLightSpreadMaxDistance(
                mMaterialLightSpreadBuffer[l],
                gameParameters.LightSpreadAdjustment);

            mLampRawDistanceCoefficientBuffer[l] = CalculateLampRawDistanceCoefficient(
                mMaterialLuminiscenceBuffer[l],
                gameParameters.LuminiscenceAdjustment,
                lampLightSpreadMaxDistance);

            mLampLightSpreadMaxDistanceBuffer[l] = lampLightSpreadMaxDistance;
        }

        // Remember new parameters
        mCurrentLightSpreadAdjustment = gameParameters.LightSpreadAdjustment;
        mCurrentLuminiscenceAdjustment = gameParameters.LuminiscenceAdjustment;
    }
}

void ElectricalElements::UpdateSourcesAndPropagation(
    SequenceNumber newConnectivityVisitSequenceNumber,
    Points & points,
    GameParameters const & gameParameters)
{
    //
    // Visit electrical graph starting from sources, and propagate connectivity state
    // by means of visit sequence number
    //

    std::queue<ElementIndex> electricalElementsToVisit;

    for (auto sourceIndex : mSources)
    {
        // Do not visit deleted sources
        if (!IsDeleted(sourceIndex))
        {
            // Make sure we haven't visited it already
            if (newConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[sourceIndex])
            {
                // Mark it as visited
                mCurrentConnectivityVisitSequenceNumberBuffer[sourceIndex] = newConnectivityVisitSequenceNumber;

                //
                // Check pre-conditions that need to be satisfied before visiting the connectivity graph
                //

                auto const sourcePointIndex = GetPointIndex(sourceIndex);

                bool preconditionsSatisfied = false;

                switch (GetMaterialType(sourceIndex))
                {
                    case ElectricalMaterial::ElectricalElementType::Generator:
                    {
                        //
                        // Preconditions to produce current:
                        // - Not too wet
                        // - Temperature within operating temperature
                        //

                        if (mElementStateBuffer[sourceIndex].Generator.IsProducingCurrent)
                        {
                            if (points.IsWet(sourcePointIndex, 0.3f)
                                || !mMaterialOperatingTemperaturesBuffer[sourceIndex].IsInRange(points.GetTemperature(sourcePointIndex)))
                            {
                                mElementStateBuffer[sourceIndex].Generator.IsProducingCurrent = false;
                            }
                        }
                        else
                        {
                            if (!points.IsWet(sourcePointIndex, 0.3f)
                                && mMaterialOperatingTemperaturesBuffer[sourceIndex].IsBackInRange(points.GetTemperature(sourcePointIndex)))
                            {
                                mElementStateBuffer[sourceIndex].Generator.IsProducingCurrent = true;
                            }
                        }

                        preconditionsSatisfied = mElementStateBuffer[sourceIndex].Generator.IsProducingCurrent;

                        break;
                    }

                    default:
                    {
                        assert(false); // At the moment our only sources are generators
                        break;
                    }
                }

                if (preconditionsSatisfied)
                {
                    //
                    // Flood graph
                    //

                    // Add source to queue
                    assert(electricalElementsToVisit.empty());
                    electricalElementsToVisit.push(sourceIndex);

                    // Visit all electrical elements reachable from this source
                    while (!electricalElementsToVisit.empty())
                    {
                        auto e = electricalElementsToVisit.front();
                        electricalElementsToVisit.pop();

                        // Already marked as visited
                        assert(newConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[e]);

                        for (auto reachableElectricalElementIndex : GetConnectedElectricalElements(e))
                        {
                            assert(!IsDeleted(reachableElectricalElementIndex));

                            // Make sure not visited already
                            if (newConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[reachableElectricalElementIndex])
                            {
                                // Add to queue
                                electricalElementsToVisit.push(reachableElectricalElementIndex);

                                // Mark it as visited
                                mCurrentConnectivityVisitSequenceNumberBuffer[reachableElectricalElementIndex] = newConnectivityVisitSequenceNumber;
                            }
                        }
                    }


                    //
                    // Generate heat
                    //

                    points.AddHeat(sourcePointIndex,
                        mMaterialHeatGeneratedBuffer[sourceIndex]
                        * gameParameters.ElectricalElementHeatProducedAdjustment
                        * GameParameters::SimulationStepTimeDuration<float>);
                }
            }
        }
    }
}

void ElectricalElements::UpdateSinks(
    GameWallClock::time_point currentWallclockTime,
    SequenceNumber currentConnectivityVisitSequenceNumber,
    Points & points,
    GameParameters const & gameParameters)
{
    //
    // Visit all sinks and run their state machine
    //

    for (auto sinkIndex : mSinks)
    {
        if (!IsDeleted(sinkIndex))
        {
            //
            // Update state machine
            //

            bool isOperating = false;

            switch (GetMaterialType(sinkIndex))
            {
                case ElectricalMaterial::ElectricalElementType::Lamp:
                {
                    // Update state machine
                    RunLampStateMachine(
                        sinkIndex,
                        currentWallclockTime,
                        currentConnectivityVisitSequenceNumber,
                        points,
                        gameParameters);

                    isOperating = (GetAvailableLight(sinkIndex) > 0.0f);

                    break;
                }

                case ElectricalMaterial::ElectricalElementType::OtherSink:
                {
                    // Update state machine
                    if (mElementStateBuffer[sinkIndex].OtherSink.IsPowered)
                    {
                        if (currentConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[sinkIndex]
                            || !mMaterialOperatingTemperaturesBuffer[sinkIndex].IsInRange(points.GetTemperature(GetPointIndex(sinkIndex))))
                        {
                            mElementStateBuffer[sinkIndex].OtherSink.IsPowered = false;
                        }
                    }
                    else
                    {
                        if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[sinkIndex]
                            && mMaterialOperatingTemperaturesBuffer[sinkIndex].IsBackInRange(points.GetTemperature(GetPointIndex(sinkIndex))))
                        {
                            mElementStateBuffer[sinkIndex].OtherSink.IsPowered = true;
                        }
                    }

                    isOperating = mElementStateBuffer[sinkIndex].OtherSink.IsPowered;

                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }


            //
            // Generate heat if sink is working
            //

            if (isOperating)
            {
                points.AddHeat(GetPointIndex(sinkIndex),
                    mMaterialHeatGeneratedBuffer[sinkIndex]
                    * gameParameters.ElectricalElementHeatProducedAdjustment
                    * GameParameters::SimulationStepTimeDuration<float>);
            }
        }
    }
}

void ElectricalElements::RunLampStateMachine(
    ElementIndex elementLampIndex,
    GameWallClock::time_point currentWallclockTime,
    SequenceNumber currentConnectivityVisitSequenceNumber,
    Points & points,
    GameParameters const & /*gameParameters*/)
{
    //
    // Lamp is only on if visited or self-powered and within operating temperature;
    // actual lights depends on flicker state machine
    //

    auto const pointIndex = GetPointIndex(elementLampIndex);

    auto & lamp = mElementStateBuffer[elementLampIndex].Lamp;

    switch (lamp.State)
    {
        case ElementState::LampState::StateType::Initial:
        {
            // Transition to ON - if we have current or if we're self-powered AND if within operating temperature
            if ((currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
                && mMaterialOperatingTemperaturesBuffer[elementLampIndex].IsInRange(points.GetTemperature(pointIndex)))
            {
                mAvailableLightBuffer[elementLampIndex] = 1.f;
                lamp.State = ElementState::LampState::StateType::LightOn;
                lamp.NextWetFailureCheckTimePoint = currentWallclockTime + std::chrono::seconds(1);
            }
            else
            {
                // Transition to OFF
                mAvailableLightBuffer[elementLampIndex] = 0.f;
                lamp.State = ElementState::LampState::StateType::LightOff;
            }

            break;
        }

        case ElementState::LampState::StateType::LightOn:
        {
            // Check whether we still have current, or we're wet and it's time to fail,
            // or whether we are outside of the operating temperature range
            if ((   currentConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                    && !lamp.IsSelfPowered
                ) ||
                (   points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold)
                    && CheckWetFailureTime(lamp, currentWallclockTime)
                ) ||
                (
                    !mMaterialOperatingTemperaturesBuffer[elementLampIndex].IsInRange(points.GetTemperature(pointIndex))
                ))
            {
                //
                // Start flicker state machine
                //

                mAvailableLightBuffer[elementLampIndex] = 0.f;

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
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold)
                && mMaterialOperatingTemperaturesBuffer[elementLampIndex].IsBackInRange(points.GetTemperature(pointIndex)))
            {
                mAvailableLightBuffer[elementLampIndex] = 1.f;

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

                    mAvailableLightBuffer[elementLampIndex] = 1.f;

                    mGameEventHandler->OnLightFlicker(
                        DurationShortLongType::Short,
                        mParentWorld.IsUnderwater(points.GetPosition(pointIndex)),
                        1);

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                }
                else if (2 == lamp.FlickerCounter)
                {
                    // Flicker to off, for a short time

                    mAvailableLightBuffer[elementLampIndex] = 0.f;

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerAInterval;
                }
                else
                {
                    assert(4 == lamp.FlickerCounter);

                    // Transition to off for good
                    mAvailableLightBuffer[elementLampIndex] = 0.f;
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
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold)
                && mMaterialOperatingTemperaturesBuffer[elementLampIndex].IsBackInRange(points.GetTemperature(pointIndex)))
            {
                mAvailableLightBuffer[elementLampIndex] = 1.f;

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

                    mAvailableLightBuffer[elementLampIndex] = 1.f;

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

                    mAvailableLightBuffer[elementLampIndex] = 0.f;

                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerBInterval;
                }
                else if (3 == lamp.FlickerCounter)
                {
                    // Flicker to on, for a longer time

                    mAvailableLightBuffer[elementLampIndex] = 1.f;

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
                    mAvailableLightBuffer[elementLampIndex] = 0.f;
                    lamp.State = ElementState::LampState::StateType::LightOff;
                }
            }

            break;
        }

        case ElementState::LampState::StateType::LightOff:
        {
            assert(mAvailableLightBuffer[elementLampIndex] == 0.f);

            // Check if we should become ON again
            if ((currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[elementLampIndex]
                || lamp.IsSelfPowered)
                && !points.IsWet(GetPointIndex(elementLampIndex), LampWetFailureWaterThreshold)
                && mMaterialOperatingTemperaturesBuffer[elementLampIndex].IsBackInRange(points.GetTemperature(pointIndex)))
            {
                mAvailableLightBuffer[elementLampIndex] = 1.f;

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
            GameRandomEngine::GetInstance().GenerateNormalizedUniformReal()
            < lamp.WetFailureRateCdf;

        // Schedule next check
        lamp.NextWetFailureCheckTimePoint = currentWallclockTime + std::chrono::seconds(1);
    }

    return isFailure;
}

}