/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameGeometry.h>
#include <GameCore/GameRandomEngine.h>

#include <queue>

namespace Physics {

float constexpr LampWetFailureWaterThreshold = 0.1f;

void ElectricalElements::Add(
    ElementIndex pointElementIndex,
    ElectricalElementInstanceIndex instanceIndex,
    std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata,
    ElectricalMaterial const & electricalMaterial)
{
    ElementIndex const elementIndex = static_cast<ElementIndex>(mIsDeletedBuffer.GetCurrentPopulatedSize());

    mIsDeletedBuffer.emplace_back(false);
    mPointIndexBuffer.emplace_back(pointElementIndex);
    mMaterialBuffer.emplace_back(&electricalMaterial);
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

        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            // State
            mElementStateBuffer.emplace_back(
                ElementState::EngineState(
                    electricalMaterial.EnginePower * 746.0f, // HP => N*m/s (which we use as N)
                    electricalMaterial.EngineResponsiveness));

            // Indices
            mEngineSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::EngineController:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::EngineControllerState(0, false));

            // Indices
            mSinks.emplace_back(elementIndex);

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
            mElementStateBuffer.emplace_back(ElementState::OtherSinkState(false));

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::PowerMonitor:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::PowerMonitorState(false));

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::ShipSound:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::ShipSoundState(electricalMaterial.IsSelfPowered, false));

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::SmokeEmitterState(electricalMaterial.ParticleEmissionRate, false));

            // Indices
            mSinks.emplace_back(elementIndex);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            // State
            mElementStateBuffer.emplace_back(ElementState::WaterPumpState(electricalMaterial.WaterPumpNominalForce));

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
            case ElectricalMaterial::ElectricalElementType::Engine:
            {
                // Announce engine as EngineMonitor
                mGameEventHandler->OnEngineMonitorCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    *mMaterialBuffer[elementIndex],
                    mElementStateBuffer[elementIndex].Engine.CurrentThrustMagnitude,
                    mElementStateBuffer[elementIndex].Engine.CurrentRpm,
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::EngineController:
            {
                mGameEventHandler->OnEngineControllerCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::Generator:
            {
                // Announce instanced generators as power probes
                if (mInstanceInfos[elementIndex].InstanceIndex != NoneElectricalElementInstanceIndex)
                {
                    mGameEventHandler->OnPowerProbeCreated(
                        ElectricalElementId(mShipId, elementIndex),
                        mInstanceInfos[elementIndex].InstanceIndex,
                        PowerProbeType::Generator,
                        static_cast<ElectricalState>(mElementStateBuffer[elementIndex].Generator.IsProducingCurrent),
                        mInstanceInfos[elementIndex].PanelElementMetadata);
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
            {
                SwitchType switchType = SwitchType::AutomaticSwitch; // Arbitrary, for MSVC
                switch (mMaterialBuffer[elementIndex]->InteractiveSwitchType)
                {
                    case ElectricalMaterial::InteractiveSwitchElementType::Push:
                    {
                        switchType = SwitchType::InteractivePushSwitch;
                        break;
                    }

                    case ElectricalMaterial::InteractiveSwitchElementType::Toggle:
                    {
                        switchType = SwitchType::InteractiveToggleSwitch;
                        break;
                    }
                }

                mGameEventHandler->OnSwitchCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    switchType,
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
                    static_cast<ElectricalState>(mElementStateBuffer[elementIndex].PowerMonitor.IsPowered),
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::ShipSound:
            {
                // Ships sounds announce themselves as switches
                mGameEventHandler->OnSwitchCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    SwitchType::ShipSoundSwitch,
                    static_cast<ElectricalState>(mConductivityBuffer[elementIndex].ConductsElectricity),
                    mInstanceInfos[elementIndex].PanelElementMetadata);

                break;
            }

            case ElectricalMaterial::ElectricalElementType::WaterPump:
            {
                mGameEventHandler->OnWaterPumpCreated(
                    ElectricalElementId(mShipId, elementIndex),
                    mInstanceInfos[elementIndex].InstanceIndex,
                    *mMaterialBuffer[elementIndex],
                    mElementStateBuffer[elementIndex].WaterPump.CurrentNormalizedForce,
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

            case ElectricalMaterial::ElectricalElementType::Cable:
            case ElectricalMaterial::ElectricalElementType::Lamp:
            case ElectricalMaterial::ElectricalElementType::OtherSink:
            case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
            {
                // Nothing to announce for these
                break;
            }
        }
    }

    mGameEventHandler->OnElectricalElementAnnouncementsEnd();
}

void ElectricalElements::HighlightElectricalElement(
    ElementIndex elementIndex,
    Points & points)
{
    rgbColor constexpr EngineOnHighlightColor = rgbColor(0xfc, 0xff, 0xa6);
    rgbColor constexpr EngineOffHighlightColor = rgbColor(0xc4, 0xb7, 0x02);

    rgbColor constexpr PowerOnHighlightColor = rgbColor(0x02, 0x5e, 0x1e);
    rgbColor constexpr PowerOffHighlightColor = rgbColor(0x91, 0x00, 0x00);

    rgbColor constexpr SoundOnHighlightColor = rgbColor(0xe0, 0xe0, 0xe0);
    rgbColor constexpr SoundOffHighlightColor = rgbColor(0x75, 0x75, 0x75);

    rgbColor constexpr SwitchOnHighlightColor = rgbColor(0x00, 0xab, 0x00);
    rgbColor constexpr SwitchOffHighlightColor = rgbColor(0xde, 0x00, 0x00);

    rgbColor constexpr WaterPumpOnHighlightColor = rgbColor(0x47, 0x60, 0xff);
    rgbColor constexpr WaterPumpOffHighlightColor = rgbColor(0x1b, 0x28, 0x80);

    // Switch state as appropriate
    switch (GetMaterialType(elementIndex))
    {
        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mElementStateBuffer[elementIndex].Engine.LastHighlightedRpm != 0.0f? EngineOnHighlightColor : EngineOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mElementStateBuffer[elementIndex].Generator.IsProducingCurrent ? PowerOnHighlightColor : PowerOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
        case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mConductivityBuffer[elementIndex].ConductsElectricity ? SwitchOnHighlightColor : SwitchOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::PowerMonitor:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mElementStateBuffer[elementIndex].PowerMonitor.IsPowered ? PowerOnHighlightColor : PowerOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::ShipSound:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mElementStateBuffer[elementIndex].ShipSound.IsPlaying  ? SoundOnHighlightColor : SoundOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            points.StartPointHighlight(
                GetPointIndex(elementIndex),
                mElementStateBuffer[elementIndex].WaterPump.TargetNormalizedForce != 0.0f ? WaterPumpOnHighlightColor : WaterPumpOffHighlightColor,
                GameWallClock::GetInstance().NowAsFloat());

            break;
        }

        default:
        {
            // Shouldn't be invoked for non-highlightable elements
            assert(false);
        }
    }
}

void ElectricalElements::SetSwitchState(
    ElectricalElementId electricalElementId,
    ElectricalState switchState,
    Points & points,
    GameParameters const & gameParameters)
{
    assert(electricalElementId.GetShipId() == mShipId);

    InternalSetSwitchState(
        electricalElementId.GetLocalObjectId(),
        switchState,
        points,
        gameParameters);
}

void ElectricalElements::SetEngineControllerState(
    ElectricalElementId electricalElementId,
    int telegraphValue,
    GameParameters const & /*gameParameters*/)
{
    assert(electricalElementId.GetShipId() == mShipId);
    auto const elementIndex = electricalElementId.GetLocalObjectId();

    assert(GetMaterialType(elementIndex) == ElectricalMaterial::ElectricalElementType::EngineController);
    auto & state = mElementStateBuffer[elementIndex].EngineController;

    assert(telegraphValue >= -static_cast<int>(GameParameters::EngineTelegraphDegreesOfFreedom / 2)
        && telegraphValue <= static_cast<int>(GameParameters::EngineTelegraphDegreesOfFreedom / 2));

    // Make sure it's a state change
    if (telegraphValue != state.CurrentTelegraphValue)
    {
        // Change current value
        state.CurrentTelegraphValue = telegraphValue;

        // Notify
        mGameEventHandler->OnEngineControllerUpdated(
            electricalElementId,
            telegraphValue);
    }
}

void ElectricalElements::Destroy(ElementIndex electricalElementIndex)
{
    // Connectivity is taken care by ship destroy handler, as usual

    assert(!IsDeleted(electricalElementIndex));

    // Zero out our light
    mAvailableLightBuffer[electricalElementIndex] = 0.0f;

    // Switch state as appropriate
    switch (GetMaterialType(electricalElementIndex))
    {
        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            // Publish state change, if necessary
            if (mElementStateBuffer[electricalElementIndex].Engine.LastPublishedRpm != 0.0f
                || mElementStateBuffer[electricalElementIndex].Engine.LastPublishedThrustMagnitude != 0.0f)
            {
                mGameEventHandler->OnEngineMonitorUpdated(
                    ElectricalElementId(mShipId, electricalElementIndex),
                    0.0f,
                    0.0f);
            }

            break;
        }

        case ElectricalMaterial::ElectricalElementType::EngineController:
        {
            mElementStateBuffer[electricalElementIndex].EngineController.IsPowered = false;

            // Publish disable
            mGameEventHandler->OnEngineControllerEnabled(
                ElectricalElementId(
                    mShipId,
                    electricalElementIndex),
                false);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            if (mElementStateBuffer[electricalElementIndex].Generator.IsProducingCurrent)
            {
                mElementStateBuffer[electricalElementIndex].Generator.IsProducingCurrent = false;

                // See whether we need to publish a power probe change
                if (mInstanceInfos[electricalElementIndex].InstanceIndex != NoneElectricalElementInstanceIndex)
                {
                    mGameEventHandler->OnPowerProbeToggled(
                        ElectricalElementId(mShipId, electricalElementIndex),
                        ElectricalState::Off);
                }
            }

            break;
        }

        case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
        case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
        {
            // Publish disable
            mGameEventHandler->OnSwitchEnabled(
                ElectricalElementId(
                    mShipId,
                    electricalElementIndex),
                false);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::PowerMonitor:
        {
            if (mElementStateBuffer[electricalElementIndex].PowerMonitor.IsPowered)
            {
                mElementStateBuffer[electricalElementIndex].PowerMonitor.IsPowered = false;

                mGameEventHandler->OnPowerProbeToggled(
                    ElectricalElementId(mShipId, electricalElementIndex),
                    ElectricalState::Off);
            }

            break;
        }

        case ElectricalMaterial::ElectricalElementType::ShipSound:
        {
            // Publish state change, if necessary
            if (mElementStateBuffer[electricalElementIndex].ShipSound.IsPlaying)
            {
                mElementStateBuffer[electricalElementIndex].ShipSound.IsPlaying = false;

                // Publish state change
                mGameEventHandler->OnShipSoundUpdated(
                    ElectricalElementId(mShipId, electricalElementIndex),
                    *mMaterialBuffer[electricalElementIndex],
                    false,
                    false); // Irrelevant
            }

            // Publish disable
            mGameEventHandler->OnSwitchEnabled(ElectricalElementId(mShipId, electricalElementIndex), false);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            mElementStateBuffer[electricalElementIndex].WaterPump.TargetNormalizedForce = 0.0f;

            // Publish disable
            mGameEventHandler->OnWaterPumpEnabled(
                ElectricalElementId(
                    mShipId,
                    electricalElementIndex),
                false);

            break;
        }

        default:
        {
            break;
        }
    }

    // Invoke destroy handler
    assert(nullptr != mShipPhysicsHandler);
    mShipPhysicsHandler->HandleElectricalElementDestroy(electricalElementIndex);

    // Remember that power has been severed during this step
    mHasPowerBeenSeveredInCurrentStep = true;

    // Flag ourselves as deleted
    mIsDeletedBuffer[electricalElementIndex] = true;
}

void ElectricalElements::Restore(ElementIndex electricalElementIndex)
{
    // Connectivity is taken care by ship destroy handler, as usual

    assert(IsDeleted(electricalElementIndex));

    // Clear the deleted flag
    mIsDeletedBuffer[electricalElementIndex] = false;

    // Switch state as appropriate
    switch (GetMaterialType(electricalElementIndex))
    {
        case ElectricalMaterial::ElectricalElementType::Engine:
        {
            mElementStateBuffer[electricalElementIndex].Engine.Reset();

            break;
        }

        case ElectricalMaterial::ElectricalElementType::EngineController:
        {
            // Notify enabling
            mGameEventHandler->OnEngineControllerEnabled(ElectricalElementId(mShipId, electricalElementIndex), true);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Generator:
        {
            // Nothing to do: at the next UpdateSources() that makes this generator work, the generator will start
            // producing current again and it will announce it

            assert(!mElementStateBuffer[electricalElementIndex].Generator.IsProducingCurrent);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::Lamp:
        {
            mElementStateBuffer[electricalElementIndex].Lamp.Reset();

            break;
        }

        case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
        case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
        {
            // Notify enabling
            mGameEventHandler->OnSwitchEnabled(ElectricalElementId(mShipId, electricalElementIndex), true);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::PowerMonitor:
        {
            // Nothing to do: at the next UpdateSources() that makes this monitor work, there will be a state change
            // and the monitor will announce it

            assert(!mElementStateBuffer[electricalElementIndex].PowerMonitor.IsPowered);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::ShipSound:
        {
            // Notify enabling
            mGameEventHandler->OnSwitchEnabled(ElectricalElementId(mShipId, electricalElementIndex), true);

            // Nothing else to do: at the next UpdateSinks() that makes this sound work, there will be a state change

            assert(!mElementStateBuffer[electricalElementIndex].ShipSound.IsPlaying);

            break;
        }

        case ElectricalMaterial::ElectricalElementType::WaterPump:
        {
            // Notify enabling
            mGameEventHandler->OnWaterPumpEnabled(ElectricalElementId(mShipId, electricalElementIndex), true);

            // Nothing else to do: at the next UpdateSinks() that makes this pump work, there will be a state change

            assert(mElementStateBuffer[electricalElementIndex].WaterPump.TargetNormalizedForce == 0.0f);

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
    // Visit all non-deleted elements that change their conductivity automatically,
    // and eventually change their conductivity
    //

    for (auto elementIndex : mAutomaticConductivityTogglingElements)
    {
        // Do not visit deleted elements
        if (!IsDeleted(elementIndex))
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

    for (auto sourceElementIndex : mSources)
    {
        // Do not visit deleted sources
        if (!IsDeleted(sourceElementIndex))
        {
            // Make sure we haven't visited it already
            if (newConnectivityVisitSequenceNumber != mCurrentConnectivityVisitSequenceNumberBuffer[sourceElementIndex])
            {
                // Mark it as visited
                mCurrentConnectivityVisitSequenceNumberBuffer[sourceElementIndex] = newConnectivityVisitSequenceNumber;

                //
                // Check pre-conditions that need to be satisfied before visiting the connectivity graph
                //

                auto const sourcePointIndex = GetPointIndex(sourceElementIndex);

                bool preconditionsSatisfied = false;

                switch (GetMaterialType(sourceElementIndex))
                {
                    case ElectricalMaterial::ElectricalElementType::Generator:
                    {
                        //
                        // Preconditions to produce current:
                        // - Not too wet
                        // - Temperature within operating temperature
                        //

                        bool isProducingCurrent;
                        if (mElementStateBuffer[sourceElementIndex].Generator.IsProducingCurrent)
                        {
                            if (points.IsWet(sourcePointIndex, 0.3f)
                                || !mMaterialOperatingTemperaturesBuffer[sourceElementIndex].IsInRange(points.GetTemperature(sourcePointIndex)))
                            {
                                isProducingCurrent = false;
                            }
                            else
                            {
                                isProducingCurrent = true;
                            }
                        }
                        else
                        {
                            if (!points.IsWet(sourcePointIndex, 0.3f)
                                && mMaterialOperatingTemperaturesBuffer[sourceElementIndex].IsBackInRange(points.GetTemperature(sourcePointIndex)))
                            {
                                isProducingCurrent = true;
                            }
                            else
                            {
                                isProducingCurrent = false;
                            }
                        }

                        preconditionsSatisfied = isProducingCurrent;

                        //
                        // Check if it's a state change
                        //

                        if (mElementStateBuffer[sourceElementIndex].Generator.IsProducingCurrent != isProducingCurrent)
                        {
                            // Change state
                            mElementStateBuffer[sourceElementIndex].Generator.IsProducingCurrent = isProducingCurrent;

                            // See whether we need to publish a power probe change
                            if (mInstanceInfos[sourceElementIndex].InstanceIndex != NoneElectricalElementInstanceIndex)
                            {
                                // Notify
                                mGameEventHandler->OnPowerProbeToggled(
                                    ElectricalElementId(mShipId, sourceElementIndex),
                                    static_cast<ElectricalState>(isProducingCurrent));

                                // Show notifications
                                if (gameParameters.DoShowElectricalNotifications)
                                {
                                    HighlightElectricalElement(sourceElementIndex, points);
                                }
                            }

                            // Remember that power has been severed, in case we're turning off
                            if (!isProducingCurrent)
                            {
                                mHasPowerBeenSeveredInCurrentStep = true;
                            }
                        }

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
                    electricalElementsToVisit.push(sourceElementIndex);

                    // Visit all electrical elements electrically reachable from this source
                    while (!electricalElementsToVisit.empty())
                    {
                        auto const e = electricalElementsToVisit.front();
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
                        mMaterialHeatGeneratedBuffer[sourceElementIndex]
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
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    //
    // Visit all sinks and run their state machine
    //
    // Also visit deleted elements, as some types have
    // post-deletion wind-down state machines
    //

    float const effectiveAugmentedAirTemperature =
        gameParameters.AirTemperature
        + stormParameters.AirTemperatureDelta
        + 200.0f; // To ensure buoyancy

    for (auto sinkElementIndex : mSinks)
    {
        //
        // Update state machine
        //

        bool const isConnectedToPower =
            (currentConnectivityVisitSequenceNumber == mCurrentConnectivityVisitSequenceNumberBuffer[sinkElementIndex]);

        bool isProducingHeat = false;

        switch (GetMaterialType(sinkElementIndex))
        {
            case ElectricalMaterial::ElectricalElementType::EngineController:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    auto & controllerState = mElementStateBuffer[sinkElementIndex].EngineController;

                    // Check whether it's powered
                    bool isPowered = false;
                    if (isConnectedToPower)
                    {
                        if (controllerState.IsPowered
                            && mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsInRange(points.GetTemperature(GetPointIndex(sinkElementIndex))))
                        {
                            isPowered = true;
                        }
                        else if (!controllerState.IsPowered
                                && mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsBackInRange(points.GetTemperature(GetPointIndex(sinkElementIndex))))
                        {
                            isPowered = true;
                        }
                    }

                    if (isPowered)
                    {
                        //
                        // Visit all (non-deleted) connected engines and add force to each
                        //

                        vec2f const & controllerPosition = points.GetPosition(GetPointIndex(sinkElementIndex));

                        for (auto const & connectedEngine : controllerState.ConnectedEngines)
                        {
                            auto const engineElectricalElementIndex = connectedEngine.EngineElectricalElementIndex;
                            if (!IsDeleted(engineElectricalElementIndex))
                            {
                                //
                                // Calculate thrust dir, rpm, and thrust magnitude exherted by this controller
                                //  - ThrustDir = f(controller->engine angle)
                                //  - RPM = f(EngineControllerState::CurrentTelegraphValue)
                                //  - ThrustMagnitude = f(EngineControllerState::CurrentTelegraphValue)
                                //

                                auto const enginePointIndex = GetPointIndex(engineElectricalElementIndex);

                                // Thrust direction (normalized vector)

                                vec2f const engineToControllerDir =
                                    (controllerPosition - points.GetPosition(enginePointIndex))
                                    .normalise();

                                vec2f const controllerEngineThrustDir = vec2f(
                                    connectedEngine.CosEngineCWAngle * engineToControllerDir.x
                                    + connectedEngine.SinEngineCWAngle * engineToControllerDir.y
                                    ,
                                    -connectedEngine.SinEngineCWAngle * engineToControllerDir.x
                                    + connectedEngine.CosEngineCWAngle * engineToControllerDir.y
                                );

                                // RPM: 0, 1/N, 1/N->1

                                float constexpr TelegraphCoeff =
                                    1.0f
                                    / static_cast<float>(GameParameters::EngineTelegraphDegreesOfFreedom / 2 - 1);

                                int const absTelegraphValue = std::abs(controllerState.CurrentTelegraphValue);
                                float controllerEngineRpm;
                                if (absTelegraphValue == 0)
                                    controllerEngineRpm = 0.0f;
                                else if (absTelegraphValue == 1)
                                    controllerEngineRpm = TelegraphCoeff;
                                else
                                    controllerEngineRpm = static_cast<float>(absTelegraphValue - 1) * TelegraphCoeff;

                                // Thrust magnitude: 0, 0, 1/N->1

                                float controllerEngineThrustMagnitude;
                                if (controllerState.CurrentTelegraphValue >= 0)
                                {
                                    if (controllerState.CurrentTelegraphValue <= 1)
                                        controllerEngineThrustMagnitude = 0.0f;
                                    else
                                        controllerEngineThrustMagnitude = static_cast<float>(controllerState.CurrentTelegraphValue - 1) * TelegraphCoeff;
                                }
                                else
                                {
                                    if (controllerState.CurrentTelegraphValue >= -1)
                                        controllerEngineThrustMagnitude = 0.0f;
                                    else
                                        controllerEngineThrustMagnitude = static_cast<float>(controllerState.CurrentTelegraphValue + 1) * TelegraphCoeff;
                                }

                                //
                                // Add to engine
                                //  - Engine has been reset at end of previous iteration
                                //

                                mElementStateBuffer[engineElectricalElementIndex].Engine.TargetThrustDir +=
                                    controllerEngineThrustDir;

                                mElementStateBuffer[engineElectricalElementIndex].Engine.TargetRpm = std::max(
                                    mElementStateBuffer[engineElectricalElementIndex].Engine.TargetRpm,
                                    controllerEngineRpm);

                                mElementStateBuffer[engineElectricalElementIndex].Engine.TargetThrustMagnitude +=
                                    controllerEngineThrustMagnitude;
                            }
                        }
                    }

                    // Remember state
                    mElementStateBuffer[sinkElementIndex].EngineController.IsPowered = isPowered;
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::Lamp:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    // Update state machine
                    RunLampStateMachine(
                        isConnectedToPower,
                        sinkElementIndex,
                        currentWallclockTime,
                        points,
                        gameParameters);

                    isProducingHeat = (GetAvailableLight(sinkElementIndex) > 0.0f);
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::OtherSink:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    // Update state machine
                    if (mElementStateBuffer[sinkElementIndex].OtherSink.IsPowered)
                    {
                        if (!isConnectedToPower
                            || !mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsInRange(points.GetTemperature(GetPointIndex(sinkElementIndex))))
                        {
                            mElementStateBuffer[sinkElementIndex].OtherSink.IsPowered = false;
                        }
                    }
                    else
                    {
                        if (isConnectedToPower
                            && mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsBackInRange(points.GetTemperature(GetPointIndex(sinkElementIndex))))
                        {
                            mElementStateBuffer[sinkElementIndex].OtherSink.IsPowered = true;
                        }
                    }

                    isProducingHeat = mElementStateBuffer[sinkElementIndex].OtherSink.IsPowered;
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::PowerMonitor:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    // Update state machine
                    if (mElementStateBuffer[sinkElementIndex].PowerMonitor.IsPowered)
                    {
                        if (!isConnectedToPower)
                        {
                            //
                            // Toggle state ON->OFF
                            //

                            mElementStateBuffer[sinkElementIndex].PowerMonitor.IsPowered = false;

                            // Notify
                            mGameEventHandler->OnPowerProbeToggled(
                                ElectricalElementId(mShipId, sinkElementIndex),
                                ElectricalState::Off);

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                    }
                    else
                    {
                        if (isConnectedToPower)
                        {
                            //
                            // Toggle state OFF->ON
                            //

                            mElementStateBuffer[sinkElementIndex].PowerMonitor.IsPowered = true;

                            // Notify
                            mGameEventHandler->OnPowerProbeToggled(
                                ElectricalElementId(mShipId, sinkElementIndex),
                                ElectricalState::On);

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                    }
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::ShipSound:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    auto & state = mElementStateBuffer[sinkElementIndex].ShipSound;

                    // Update state machine
                    if (state.IsPlaying)
                    {
                        if ((!state.IsSelfPowered && !isConnectedToPower)
                            || !mConductivityBuffer[sinkElementIndex].ConductsElectricity)
                        {
                            //
                            // Toggle state ON->OFF
                            //

                            state.IsPlaying = false;

                            // Notify sound
                            mGameEventHandler->OnShipSoundUpdated(
                                ElectricalElementId(mShipId, sinkElementIndex),
                                *(mMaterialBuffer[sinkElementIndex]),
                                false,
                                false); // Irrelevant

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                    }
                    else
                    {
                        if ((state.IsSelfPowered || isConnectedToPower)
                            && mConductivityBuffer[sinkElementIndex].ConductsElectricity)
                        {
                            //
                            // Toggle state OFF->ON
                            //

                            state.IsPlaying = true;

                            // Notify sound
                            mGameEventHandler->OnShipSoundUpdated(
                                ElectricalElementId(mShipId, sinkElementIndex),
                                *(mMaterialBuffer[sinkElementIndex]),
                                true,
                                mParentWorld.IsUnderwater(points.GetPosition(GetPointIndex(sinkElementIndex))));

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                    }
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
            {
                if (!IsDeleted(sinkElementIndex))
                {
                    // Update state machine
                    if (mElementStateBuffer[sinkElementIndex].SmokeEmitter.IsOperating)
                    {
                        if (!isConnectedToPower
                            || mParentWorld.IsUnderwater(points.GetPosition(GetPointIndex(sinkElementIndex))))
                        {
                            // Stop operating
                            mElementStateBuffer[sinkElementIndex].SmokeEmitter.IsOperating = false;
                        }
                    }
                    else
                    {
                        if (isConnectedToPower
                            && !mParentWorld.IsUnderwater(points.GetPosition(GetPointIndex(sinkElementIndex))))
                        {
                            // Start operating
                            mElementStateBuffer[sinkElementIndex].SmokeEmitter.IsOperating = true;

                            // Make sure we calculate the next emission timestamp
                            mElementStateBuffer[sinkElementIndex].SmokeEmitter.NextEmissionSimulationTimestamp = 0.0f;
                        }
                    }

                    if (mElementStateBuffer[sinkElementIndex].SmokeEmitter.IsOperating)
                    {
                        // See if we need to calculate the next emission timestamp
                        if (mElementStateBuffer[sinkElementIndex].SmokeEmitter.NextEmissionSimulationTimestamp == 0.0f)
                        {
                            mElementStateBuffer[sinkElementIndex].SmokeEmitter.NextEmissionSimulationTimestamp =
                                currentSimulationTime
                                + GameRandomEngine::GetInstance().GenerateExponentialReal(
                                    gameParameters.SmokeEmissionDensityAdjustment
                                    / mElementStateBuffer[sinkElementIndex].SmokeEmitter.EmissionRate);
                        }

                        // See if it's time to emit smoke
                        if (currentSimulationTime >= mElementStateBuffer[sinkElementIndex].SmokeEmitter.NextEmissionSimulationTimestamp)
                        {
                            //
                            // Emit smoke
                            //

                            auto const emitterPointIndex = GetPointIndex(sinkElementIndex);

                            // Choose temperature: highest of emitter's and current air + something (to ensure buoyancy)
                            float const smokeTemperature = std::max(
                                points.GetTemperature(emitterPointIndex),
                                effectiveAugmentedAirTemperature);

                            // Generate particle
                            points.CreateEphemeralParticleLightSmoke(
                                points.GetPosition(emitterPointIndex),
                                smokeTemperature,
                                currentSimulationTime,
                                points.GetPlaneId(emitterPointIndex),
                                gameParameters);

                            // Make sure we re-calculate the next emission timestamp
                            mElementStateBuffer[sinkElementIndex].SmokeEmitter.NextEmissionSimulationTimestamp = 0.0f;
                        }
                    }
                }

                break;
            }

            case ElectricalMaterial::ElectricalElementType::WaterPump:
            {
                auto const pointIndex = GetPointIndex(sinkElementIndex);

                auto & waterPumpState = mElementStateBuffer[sinkElementIndex].WaterPump;

                //
                // 1) If not deleted, run operating state machine (connectivity, operating temperature)
                //    in order to come up with TargetForce
                //

                if (!IsDeleted(sinkElementIndex))
                {
                    if (waterPumpState.TargetNormalizedForce != 0.0f)
                    {
                        // Currently it's powered...
                        // ...see if it stops being powered
                        if (!isConnectedToPower
                            || !mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsInRange(points.GetTemperature(pointIndex)))
                        {
                            // State change: stop operating
                            waterPumpState.TargetNormalizedForce = 0.0f;

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                        else
                        {
                            // Operating, thus producing heat
                            isProducingHeat = true;
                        }
                    }
                    else
                    {
                        // Currently it's not powered...
                        // ...see if it becomes powered
                        if (isConnectedToPower
                            && mMaterialOperatingTemperaturesBuffer[sinkElementIndex].IsBackInRange(points.GetTemperature(pointIndex)))
                        {
                            // State change: start operating
                            waterPumpState.TargetNormalizedForce = 1.0f;

                            // Operating, thus producing heat
                            isProducingHeat = true;

                            // Show notifications
                            if (gameParameters.DoShowElectricalNotifications)
                            {
                                HighlightElectricalElement(sinkElementIndex, points);
                            }
                        }
                    }
                }

                //
                // 2) Smooth CurrentForce towards TargetForce and eventually act on particle
                //
                // We run this also when deleted, as it's part of our wind-down state machine
                //

                // Smooth current force
                waterPumpState.CurrentNormalizedForce +=
                    (waterPumpState.TargetNormalizedForce - waterPumpState.CurrentNormalizedForce)
                    * 0.025f; // Convergence rate, magic number

                // Apply force to point
                points.GetLeakingComposite(pointIndex).LeakingSources.WaterPumpForce =
                    waterPumpState.CurrentNormalizedForce
                    * waterPumpState.NominalForce;

                // Eventually publish force change notification
                if (waterPumpState.CurrentNormalizedForce != waterPumpState.LastPublishedNormalizedForce)
                {
                    // Notify
                    mGameEventHandler->OnWaterPumpUpdated(
                        ElectricalElementId(mShipId, sinkElementIndex),
                        waterPumpState.CurrentNormalizedForce);

                    // Remember last-published value
                    waterPumpState.LastPublishedNormalizedForce = waterPumpState.CurrentNormalizedForce;
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

        if (isProducingHeat)
        {
            points.AddHeat(GetPointIndex(sinkElementIndex),
                mMaterialHeatGeneratedBuffer[sinkElementIndex]
                * gameParameters.ElectricalElementHeatProducedAdjustment
                * GameParameters::SimulationStepTimeDuration<float>);
        }
    }

    //
    // Visit all engines and run their state machine
    //

    for (auto engineSinkElementIndex : mEngineSinks)
    {
        if (!IsDeleted(engineSinkElementIndex))
        {
            assert(GetMaterialType(engineSinkElementIndex) == ElectricalMaterial::ElectricalElementType::Engine);
            auto & engineState = mElementStateBuffer[engineSinkElementIndex].Engine;

            auto const enginePointIndex = GetPointIndex(engineSinkElementIndex);

            //
            // Apply engine thrust
            //

            // Adjust targets based off point's water
            //  e^(-0.5*x + 5)/(5 + e^(-0.5*x + 5))
            float targetDamping;
            auto const engineWater = points.GetWater(enginePointIndex);
            if (engineWater != 0.0f)
            {
                float const expCoeff = std::exp(-engineWater * 0.5f + 5.0f);
                targetDamping = expCoeff / (5.0f + expCoeff);
            }
            else
            {
                targetDamping = 1.0f;
            }


            // Update current values to match targets (via responsiveness)

            engineState.CurrentRpm =
                engineState.CurrentRpm
                + (engineState.TargetRpm * targetDamping - engineState.CurrentRpm) * engineState.Responsiveness;

            engineState.CurrentThrustMagnitude =
                engineState.CurrentThrustMagnitude
                + (engineState.TargetThrustMagnitude * targetDamping - engineState.CurrentThrustMagnitude) * engineState.Responsiveness;

            // Calculate force vector
            vec2f const thrustForce =
                engineState.TargetThrustDir
                * engineState.CurrentThrustMagnitude
                * engineState.ThrustCapacity
                * gameParameters.EngineThrustAdjustment;

            // Apply force to point
            points.GetNonSpringForce(enginePointIndex) += thrustForce;

            //
            // Publish
            //

            // Eventually publish power change notification
            if (engineState.CurrentThrustMagnitude != engineState.LastPublishedThrustMagnitude
                || engineState.CurrentRpm != engineState.LastPublishedRpm)
            {
                // Notify
                mGameEventHandler->OnEngineMonitorUpdated(
                    ElectricalElementId(mShipId, engineSinkElementIndex),
                    engineState.CurrentThrustMagnitude,
                    engineState.CurrentRpm);

                // Remember last-published values
                engineState.LastPublishedThrustMagnitude = engineState.CurrentThrustMagnitude;
                engineState.LastPublishedRpm = engineState.CurrentRpm;
            }

            // Eventually show notifications - only if moving between zero and non-zero RPM
            if (gameParameters.DoShowElectricalNotifications)
            {
                if ((engineState.TargetRpm != 0.0f && engineState.LastHighlightedRpm == 0.0)
                    || (engineState.TargetRpm == 0.0f && engineState.LastHighlightedRpm != 0.0))
                {
                    engineState.LastHighlightedRpm = engineState.TargetRpm;

                    HighlightElectricalElement(engineSinkElementIndex, points);
                }
            }

            //
            // Generate heat if running
            //

            points.AddHeat(enginePointIndex,
                mMaterialHeatGeneratedBuffer[engineSinkElementIndex]
                * engineState.CurrentRpm
                * gameParameters.ElectricalElementHeatProducedAdjustment
                * GameParameters::SimulationStepTimeDuration<float>);

            //
            // Generate wake if running and underwater
            //

            if (gameParameters.DoGenerateEngineWakeParticles)
            {
                vec2f const enginePosition = points.GetPosition(enginePointIndex);

                if (float const absThrustMagnitude = std::abs(engineState.CurrentThrustMagnitude);
                    absThrustMagnitude > 0.1f // Magic number
                    && mParentWorld.IsUnderwater(enginePosition))
                {
                    auto const planeId = points.GetPlaneId(enginePointIndex);

                    for (int i = 0; i < std::round(absThrustMagnitude * 4.0f); ++i)
                    {
                        float constexpr HalfFanOutAngle = Pi<float> / 14.0f; // Magic number

                        float const angle = Clamp(
                            0.15f * GameRandomEngine::GetInstance().GenerateNormalizedNormalReal(),
                            -HalfFanOutAngle,
                            HalfFanOutAngle);

                        vec2f const wakeVelocity =
                            -engineState.TargetThrustDir.rotate(angle)
                            * (engineState.CurrentThrustMagnitude < 0.0f ? -1.0f : 1.0f)
                            * 20.0f; // Magic number

                        points.CreateEphemeralParticleWakeBubble(
                            enginePosition,
                            wakeVelocity,
                            currentSimulationTime,
                            planeId,
                            gameParameters);
                    }
                }
            }

            //
            // Update engine conductivity
            //

            InternalChangeConductivity(
                engineSinkElementIndex,
                (engineState.CurrentRpm > 0.15f)); // Magic number

            //
            // Reset this engine's targets, we'll re-update them at the next iteration
            //

            engineState.ClearTargets();
        }
    }

    //
    // Clear "power severed" dirtyness
    //

    mHasPowerBeenSeveredInCurrentStep = false;
}

void ElectricalElements::AddFactoryConnectedElectricalElement(
    ElementIndex electricalElementIndex,
    ElementIndex connectedElectricalElementIndex,
    Octant octant)
{
    // Add element
    AddConnectedElectricalElement(
        electricalElementIndex,
        connectedElectricalElementIndex);

    // Store connected engine if this is an EngineController->Engine connection
    if (GetMaterialType(electricalElementIndex) == ElectricalMaterial::ElectricalElementType::EngineController
        && GetMaterialType(connectedElectricalElementIndex) == ElectricalMaterial::ElectricalElementType::Engine)
    {
        float engineCWAngle =
            (2.0f * Pi<float> - mMaterialBuffer[connectedElectricalElementIndex]->EngineCCWDirection)
            - OctantToCWAngle(OppositeOctant(octant));

        // Normalize
        if (engineCWAngle < 0.0f)
            engineCWAngle += 2.0f * Pi<float>;

        mElementStateBuffer[electricalElementIndex].EngineController.ConnectedEngines.emplace_back(
            connectedElectricalElementIndex,
            engineCWAngle);
    }
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
        // Update conductivity graph (circuit)
        InternalChangeConductivity(elementIndex, static_cast<bool>(switchState));

        // Notify switch toggled
        mGameEventHandler->OnSwitchToggled(
            ElectricalElementId(mShipId, elementIndex),
            switchState);

        // Show notifications - for interactive switches only
        if (gameParameters.DoShowElectricalNotifications
            && mMaterialTypeBuffer[elementIndex] == ElectricalMaterial::ElectricalElementType::InteractiveSwitch)
        {
            HighlightElectricalElement(elementIndex, points);
        }
    }
}

void ElectricalElements::InternalChangeConductivity(
    ElementIndex elementIndex,
    bool value)
{
    // Update conductive connectivity
    if (mConductivityBuffer[elementIndex].ConductsElectricity == false && value == true)
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
    else if (mConductivityBuffer[elementIndex].ConductsElectricity == true && value == false)
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

    // Change current value
    mConductivityBuffer[elementIndex].ConductsElectricity = value;
}

void ElectricalElements::RunLampStateMachine(
    bool isConnectedToPower,
    ElementIndex elementLampIndex,
    GameWallClock::time_point currentWallclockTime,
    Points & points,
    GameParameters const & /*gameParameters*/)
{
    //
    // Lamp is only on if visited or self-powered and within operating temperature;
    // actual light depends on flicker state machine
    //

    auto const pointIndex = GetPointIndex(elementLampIndex);

    assert(GetMaterialType(elementLampIndex) == ElectricalMaterial::ElectricalElementType::Lamp);
    auto & lamp = mElementStateBuffer[elementLampIndex].Lamp;

    switch (lamp.State)
    {
        case ElementState::LampState::StateType::Initial:
        {
            // Transition to ON - if we have current or if we're self-powered AND if within operating temperature
            if ((isConnectedToPower || lamp.IsSelfPowered)
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
            if ((   !isConnectedToPower
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
                if (mHasPowerBeenSeveredInCurrentStep)
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
                else
                {
                    //
                    // Turn off gracefully
                    //

                    lamp.State = ElementState::LampState::StateType::LightOff;
                }
            }

            break;
        }

        case ElementState::LampState::StateType::FlickerA:
        {
            // 0-1-0-1-Off

            // Check if we should become ON again
            if ((isConnectedToPower || lamp.IsSelfPowered)
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
            if ((isConnectedToPower || lamp.IsSelfPowered)
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
            if ((isConnectedToPower || lamp.IsSelfPowered)
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