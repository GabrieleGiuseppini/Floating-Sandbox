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
    ElectricalElementInstanceIndex instanceIndex,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata,
    ElectricalMaterial const & electricalMaterial)
{
    ElementIndex const elementIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mMaterialTypeBuffer.emplace_back(electricalMaterial.ElectricalType);
    mConductivityBuffer.emplace_back(electricalMaterial.ConductsElectricity);
    mMaterialHeatGeneratedBuffer.emplace_back(electricalMaterial.HeatGenerated);
    mMaterialOperatingTemperaturesBuffer.emplace_back(
        electricalMaterial.MinimumOperatingTemperature,
        electricalMaterial.MaximumOperatingTemperature);
    mMaterialLuminiscenceBuffer.emplace_back(electricalMaterial.Luminiscence);
    mMaterialLightColorBuffer.emplace_back(electricalMaterial.LightColor);
    mMaterialLightSpreadBuffer.emplace_back(electricalMaterial.LightSpread);
    mConnectedElectricalElementsBuffer.emplace_back(); // Will be populated later
    mConductingConnectedElectricalElementsBuffer.emplace_back(); // Will be populated later
    mAvailableLightBuffer.emplace_back(0.f);

    //
    // Per-type initialization
    //

    switch (electricalMaterial.ElectricalType)
    {
        case ElectricalMaterial::ElectricalElementType::Cable:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::CableState());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::GeneratorState(true));

            // Indices
            mSources.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            // State
            mElementStateBuffer.emplace_back(
                ElementState::LampState(
                    electricalMaterial.IsSelfPowered,
                    electricalMaterial.WetFailureRate));

            // Indices
            mSinks.emplace_back(elementIndex);
            mLamps.emplace_back(elementIndex);

            // Lighting

            float const lampLightSpreadMaxDistance = CalculateLampLightSpreadMaxDistance(
                electricalMaterial.LightSpread,
                mCurrentLightSpreadAdjustment);

            mLampRawDistanceCoefficientBuffer.emplace_back(
                CalculateLampRawDistanceCoefficient(
                    electricalMaterial.Luminiscence,
                    mCurrentLuminiscenceAdjustment,
                    lampLightSpreadMaxDistance));

            mLampLightSpreadMaxDistanceBuffer.emplace_back(lampLightSpreadMaxDistance);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::OtherSink:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::OtherSinkState());

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::SmokeEmitterState(electricalMaterial.ParticleEmissionRate));

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::DummyState());

            // Indices
            mAutomaticConductivityTogglingElements.emplace_back(elementIndex);

            break;
        }

        default:
        {
            // State - dummy
            mElementStateBuffer.emplace_back(ElementState::DummyState());

            break;
        }
    }

    mCurrentConnectivityVisitSequenceNumberBuffer.emplace_back();

    mInstanceInfos.emplace_back(instanceIndex, panelElementMetadata);
}


void ElectricalElements::AnnounceInstancedElements()
{
    mGameEventHandler->OnElectricalElementAnnouncementsBegin();

    for (auto elementIndex : *this)
    {
        assert(elementIndex < mInstanceInfos.size());

        switch (GetMaterialType(elementIndex))
        {
            case ElectricalMaterial::ElectricalElementType::InteractivePushSwitch:
            {
                mGameEventHandler->OnSwitchCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    SwitchType::InteractivePushSwitch,
                    static_cast<ElectricalState>(mConductivityBuffer[elementIndex].ConductsElectricity),
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::InteractiveToggleSwitch:
            {
                mGameEventHandler->OnSwitchCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    SwitchType::InteractiveToggleSwitch,
                    static_cast<ElectricalState>(mConductivityBuffer[elementIndex].ConductsElectricity),
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::PowerMonitor:
            {
                mGameEventHandler->OnPowerProbeCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    PowerProbeType::PowerMonitor,
                    ElectricalState::Off, // We start with off; we'll figure out actual state at the next update
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
            {
                mGameEventHandler->OnSwitchCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    SwitchType::AutomaticSwitch,
                    static_cast<ElectricalState>(mConductivityBuffer[elementIndex].ConductsElectricity),
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }
        }
    }

    mGameEventHandler->OnElectricalElementAnnouncementsEnd();
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    // Connectivity is taken care by ship destroy handler, as usual

    assert(!IsDeleted(electricalElementIndex));

    // Zero out our light
    mAvailableLightBuffer[electricalElementIndex] = 0.0f;

    // Notify switch disabling
    auto const electricalMaterialType = GetMaterialType(electricalElementIndex);
    if (electricalMaterialType == ElectricalMaterial::ElectricalElementType::InteractivePushSwitch
        || electricalMaterialType == ElectricalMaterial::ElectricalElementType::InteractiveToggleSwitch
        || electricalMaterialType == ElectricalMaterial::ElectricalElementType::WaterSensingSwitch)
    {
        mGameEventHandler->OnSwitchEnabled(ElectricalElementId(mShipId, electricalElementIndex), false);
    }

    // Invoke destroy handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleElectricalElementDestroy(electricalElementIndex);

    // Flag ourselves as deleted
    mIsDeletedBuffer[electricalElementIndex] = true;
}

void ElectricalElements::Restore(ElementIndex electricalElementIndex)
{
    // Connectivity is taken care by ship destroy handler, as usual

    assert(IsDeleted(electricalElementIndex));

    // Clear the deleted flag
    mIsDeletedBuffer[electricalElementIndex] = false;

    // Reset our state machine
    switch (GetMaterialType(electricalElementIndex))
    {
        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            mElementStateBuffer[electricalElementIndex].Lamp.Reset();
            break;
        }

        default:
        {
            // These types do not have a state machine that needs to be reset
            break;
        }
    }

    // Invoke restore handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleElectricalElementRestore(electricalElementIndex);

    // Notify switch enabling
    auto const electricalMaterialType = GetMaterialType(electricalElementIndex);
    if (electricalMaterialType == ElectricalMaterial::ElectricalElementType::InteractivePushSwitch
        || electricalMaterialType == ElectricalMaterial::ElectricalElementType::InteractiveToggleSwitch
        || electricalMaterialType == ElectricalMaterial::ElectricalElementType::WaterSensingSwitch)
    {
        mGameEventHandler->OnSwitchEnabled(ElectricalElementId(mShipId, electricalElementIndex), true);
    }
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

void ElectricalElements::UpdateAutomaticConductivityToggles(
    Points & points,
    GameParameters const & gameParameters)
{
    //
    // Visit all elements that change their conductivity automatically,
    // and eventually change their conductivity
    //

    for (auto elementIndex : mAutomaticConductivityTogglingElements)
    {
        switch (GetMaterialType(elementIndex))
        {
            case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
            {
                // When higher than watermark: conductivity state toggles to opposite than material's
                // When lower than watermark: conductivity state toggles to same as material's

                float constexpr WaterLowWatermark = 0.15f;
                float constexpr WaterHighWatermark = 0.45f;

                if (mConductivityBuffer[elementIndex].ConductsElectricity == mConductivityBuffer[elementIndex].MaterialConductsElectricity
                    && points.GetWater(GetPointIndex(elementIndex)) >= WaterHighWatermark)
                {
                    InternalSetSwitchState(
                        elementIndex,
                        static_cast<ElectricalState>(!mConductivityBuffer[elementIndex].MaterialConductsElectricity),
                        points,
                        gameParameters);
                }
                else if (mConductivityBuffer[elementIndex].ConductsElectricity != mConductivityBuffer[elementIndex].MaterialConductsElectricity
                    && points.GetWater(GetPointIndex(elementIndex)) <= WaterLowWatermark)
                {
                    InternalSetSwitchState(
                        elementIndex,
                        static_cast<ElectricalState>(mConductivityBuffer[elementIndex].MaterialConductsElectricity),
                        points,
                        gameParameters);
                }

                break;
            }

            default:
            {
                // Shouldn't be here - all automatically-toggling elements should have been handled
                assert(false);
            }
        }
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

                    // Visit all electrical elements electrically reachable from this source
                    while (!electricalElementsToVisit.empty())
                    {
                        auto e = electricalElementsToVisit.front();
                        electricalElementsToVisit.pop();

                        // Already marked as visited
                        assert(newConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[e]);

                        for (auto conductingConnectedElectricalElementIndex : mConductingConnectedElectricalElementsBuffer[e])
                        {
                            assert(!IsDeleted(conductingConnectedElectricalElementIndex));

                            // Make sure not visited already
                            if (newConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[conductingConnectedElectricalElementIndex])
                            {
                                // Add to queue
                                electricalElementsToVisit.push(conductingConnectedElectricalElementIndex);

                                // Mark it as visited
                                mCurrentConnectivityVisitSequenceNumberBuffer[conductingConnectedElectricalElementIndex] = newConnectivityVisitSequenceNumber;
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
    float currentSimulationTime,
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

                case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
                {
                    // Update state machine
                    if (mElementStateBuffer[sinkIndex].SmokeEmitter.IsOperating)
                    {
                        if (currentConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[sinkIndex]
                            || mParentWorld.IsUnderwater(points.GetPosition(GetPointIndex(sinkIndex))))
                        {
                            // Stop operating
                            mElementStateBuffer[sinkIndex].SmokeEmitter.IsOperating = false;
                        }
                    }
                    else
                    {
                        if (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[sinkIndex]
                            && !mParentWorld.IsUnderwater(points.GetPosition(GetPointIndex(sinkIndex))))
                        {
                            // Start operating
                            mElementStateBuffer[sinkIndex].SmokeEmitter.IsOperating = true;

                            // Make sure we calculate the next emission timestamp
                            mElementStateBuffer[sinkIndex].SmokeEmitter.NextEmissionSimulationTimestamp = 0.0f;
                        }
                    }

                    if (mElementStateBuffer[sinkIndex].SmokeEmitter.IsOperating)
                    {
                        // See if we need to calculate the next emission timestamp
                        if (mElementStateBuffer[sinkIndex].SmokeEmitter.NextEmissionSimulationTimestamp == 0.0f)
                        {
                            mElementStateBuffer[sinkIndex].SmokeEmitter.NextEmissionSimulationTimestamp =
                                currentSimulationTime
                                + GameRandomEngine::GetInstance().GenerateExponentialReal(
                                    gameParameters.SmokeEmissionDensityAdjustment
                                    / mElementStateBuffer[sinkIndex].SmokeEmitter.EmissionRate);
                        }

                        // See if it's time to emit smoke
                        if (currentSimulationTime >= mElementStateBuffer[sinkIndex].SmokeEmitter.NextEmissionSimulationTimestamp)
                        {
                            //
                            // Emit smoke
                            //

                            auto const emitterPointIndex = GetPointIndex(sinkIndex);

                            // Choose temperature: highest of emitter's and current air + something (to ensure buoyancy)
                            float const temperature = std::max(
                                points.GetTemperature(emitterPointIndex),
                                gameParameters.AirTemperature + 200.0f);

                            // Generate particle
                            points.CreateEphemeralParticleLightSmoke(
                                points.GetPosition(emitterPointIndex),
                                temperature,
                                currentSimulationTime,
                                points.GetPlaneId(emitterPointIndex),
                                gameParameters);

                            // Make sure we re-calculate the next emission timestamp
                            mElementStateBuffer[sinkIndex].SmokeEmitter.NextEmissionSimulationTimestamp = 0.0f;
                        }
                    }

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

    //
    // Clear switch toggle dirtyness
    //

    mHasSwitchBeenToggledInStep = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ElectricalElements::InternalSetSwitchState(
    ElementIndex elementIndex,
    ElectricalState switchState,
    Points & points,
    GameParameters const & gameParameters)
{
    // Make sure it's a state change
    if (static_cast<bool>(switchState) != mConductivityBuffer[elementIndex].ConductsElectricity)
    {
        // Change current value
        mConductivityBuffer[elementIndex].ConductsElectricity = static_cast<bool>(switchState);

        // Update conductive connectivity
        if (static_cast<bool>(switchState) == true)
        {
            // OFF->ON

            // For each electrical element connected to this one: if both elements conduct electricity,
            // conduct-connect elements to each other
            for (auto const & otherElementIndex : mConnectedElectricalElementsBuffer[elementIndex])
            {
                assert(!mConductingConnectedElectricalElementsBuffer[elementIndex].contains(otherElementIndex));
                assert(!mConductingConnectedElectricalElementsBuffer[otherElementIndex].contains(elementIndex));
                if (mConductivityBuffer[otherElementIndex].ConductsElectricity)
                {
                    mConductingConnectedElectricalElementsBuffer[elementIndex].push_back(otherElementIndex);
                    mConductingConnectedElectricalElementsBuffer[otherElementIndex].push_back(elementIndex);
                }
            }
        }
        else
        {
            // ON->OFF

            // For each electrical element connected to this one: if the other element conducts electricity,
            // sever conduct-connection to each other
            for (auto const & otherElementIndex : mConnectedElectricalElementsBuffer[elementIndex])
            {
                if (mConductivityBuffer[otherElementIndex].ConductsElectricity)
                {
                    assert(mConductingConnectedElectricalElementsBuffer[elementIndex].contains(otherElementIndex));
                    assert(mConductingConnectedElectricalElementsBuffer[otherElementIndex].contains(elementIndex));

                    mConductingConnectedElectricalElementsBuffer[elementIndex].erase_first(otherElementIndex);
                    mConductingConnectedElectricalElementsBuffer[otherElementIndex].erase_first(elementIndex);
                }
                else
                {
                    assert(!mConductingConnectedElectricalElementsBuffer[elementIndex].contains(otherElementIndex));
                    assert(!mConductingConnectedElectricalElementsBuffer[otherElementIndex].contains(elementIndex));
                }
            }
        }

        // Notify
        mGameEventHandler->OnSwitchToggled(
            ElectricalElementId(mShipId, elementIndex),
            switchState);

        // Show notifications
        if (gameParameters.DoShowElectricalNotifications)
        {
            // Highlight point
            rgbColor constexpr SwitchOnHighlightColor = rgbColor(0x02, 0x5e, 0x1e);
            rgbColor constexpr SwitchOffHighlightColor = rgbColor(0xb5, 0x00, 0x00);
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                switchState == ElectricalState::On ? SwitchOnHighlightColor : SwitchOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());
        }

        // Remember that a switch has been toggled
        mHasSwitchBeenToggledInStep = true;
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
    // actual light depends on flicker state machine
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
                // Transition to ON
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
                // Turn off
                //

                mAvailableLightBuffer[elementLampIndex] = 0.f;

                // Check whether we need to flicker or just turn off gracefully
                if (mHasSwitchBeenToggledInStep)
                {
                    //
                    // Turn off gracefully
                    //

                    lamp.State = ElementState::LampState::StateType::LightOff;
                }
                else
                {
                    //
                    // Start flicker state machine
                    //

                    // Transition state, choose whether to A or B
                    lamp.FlickerCounter = 0u;
                    lamp.NextStateTransitionTimePoint = currentWallclockTime + ElementState::LampState::FlickerStartInterval;
                    if (GameRandomEngine::GetInstance().Choose(2) == 0)
                        lamp.State = ElementState::LampState::StateType::FlickerA;
                    else
                        lamp.State = ElementState::LampState::StateType::FlickerB;
                }
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

                // Notify flicker event, so we play light-on sound
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