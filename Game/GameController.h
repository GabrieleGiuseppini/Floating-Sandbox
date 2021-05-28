/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FishSpeciesDatabase.h"
#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "IGameController.h"
#include "IGameControllerSettings.h"
#include "IGameControllerSettingsOptions.h"
#include "IGameEventHandlers.h"
#include "MaterialDatabase.h"
#include "NotificationLayer.h"
#include "PerfStats.h"
#include "Physics.h"
#include "RenderContext.h"
#include "RenderDeviceProperties.h"
#include "ResourceLocator.h"
#include "ShipMetadata.h"
#include "ShipTexturizer.h"

#include <GameCore/Colors.h>
#include <GameCore/GameChronometer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageData.h>
#include <GameCore/ImageSize.h>
#include <GameCore/ParameterSmoother.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/StrongTypeDef.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/*
 * This class is responsible for managing the game, from its lifetime to the user
 * interactions.
 */
class GameController final
    : public IGameController
    , public IGameControllerSettings
    , public IGameControllerSettingsOptions
    , public ILifecycleGameEventHandler
    , public IWavePhenomenaGameEventHandler
{
public:

    static std::unique_ptr<GameController> Create(
        RenderDeviceProperties const & renderDeviceProperties,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

public:

    /////////////////////////////////////////////////////////
    // IGameController
    /////////////////////////////////////////////////////////

    void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterLifecycleEventHandler(handler);
    }

    void RegisterStructuralEventHandler(IStructuralGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStructuralEventHandler(handler);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterWavePhenomenaEventHandler(handler);
    }

    void RegisterCombustionEventHandler(ICombustionGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterCombustionEventHandler(handler);
    }

    void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterStatisticsEventHandler(handler);
    }

    void RegisterAtmosphereEventHandler(IAtmosphereGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterAtmosphereEventHandler(handler);
    }

    void RegisterElectricalElementEventHandler(IElectricalElementGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterElectricalElementEventHandler(handler);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterGenericEventHandler(handler);
    }

    void RebindOpenGLContext();

    ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath) override;
    ShipMetadata ResetAndReloadShip(std::filesystem::path const & shipDefinitionFilepath) override;
    ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath) override;

    RgbImageData TakeScreenshot() override;

    void RunGameIteration() override;
    void LowFrequencyUpdate() override;

    void PulseUpdateAtNextGameIteration() override
    {
        mIsPulseUpdateSet = true;
    }

    void StartRecordingEvents(std::function<void(uint32_t, RecordedEvent const &)> onEventCallback) override;
    RecordedEvents StopRecordingEvents() override;
    void ReplayRecordedEvent(RecordedEvent const & event) override;

    //
    // Game Control and notifications
    //

    void SetPaused(bool isPaused) override;
    void SetMoveToolEngaged(bool isEngaged) override;
    void DisplaySettingsLoadedNotification() override;

    bool GetShowStatusText() const override;
    void SetShowStatusText(bool value) override;
    bool GetShowExtendedStatusText() const override;
    void SetShowExtendedStatusText(bool value) override;

    void NotifySoundMuted(bool isSoundMuted) override;

    //
    // World probing
    //

    float GetCurrentSimulationTime() const override { return mWorld->GetCurrentSimulationTime(); }
    float GetEffectiveAmbientLightIntensity() const override { return mRenderContext->GetEffectiveAmbientLightIntensity(); }
    bool IsUnderwater(LogicalPixelCoordinates const & screenCoordinates) const override { return mWorld->IsUnderwater(ScreenToWorld(screenCoordinates)); }
    bool IsUnderwater(ElementId elementId) const override { return mWorld->IsUnderwater(elementId); }

    //
    // Interactions
    //

    void ScareFish(LogicalPixelCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) override;
    void AttractFish(LogicalPixelCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) override;

    void PickObjectToMove(LogicalPixelCoordinates const & screenCoordinates, std::optional<ElementId> & elementId) override;
    void PickObjectToMove(LogicalPixelCoordinates const & screenCoordinates, std::optional<ShipId> & shipId) override;
    void MoveBy(ElementId elementId, LogicalPixelSize const & screenOffset, LogicalPixelSize const & inertialScreenOffset) override;
    void MoveBy(ShipId shipId, LogicalPixelSize const & screenOffset, LogicalPixelSize const & inertialScreenOffset) override;
    void RotateBy(ElementId elementId, float screenDeltaY, LogicalPixelCoordinates const & screenCenter, float inertialScreenDeltaY) override;
    void RotateBy(ShipId shipId, float screenDeltaY, LogicalPixelCoordinates const & screenCenter, float intertialScreenDeltaY) override;
    std::optional<ElementId> PickObjectForPickAndPull(LogicalPixelCoordinates const & screenCoordinates) override;
    void Pull(ElementId elementId, LogicalPixelCoordinates const & screenTarget) override;
    void DestroyAt(LogicalPixelCoordinates const & screenCoordinates, float radiusMultiplier) override;
    void RepairAt(LogicalPixelCoordinates const & screenCoordinates, float radiusMultiplier, SequenceNumber repairStepId) override;
    void SawThrough(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates, bool isFirstSegment) override;
    bool ApplyHeatBlasterAt(LogicalPixelCoordinates const & screenCoordinates, HeatBlasterActionType action) override;
    bool ExtinguishFireAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void ApplyBlastAt(LogicalPixelCoordinates const & screenCoordinates, float radiusMultiplier, float forceMultiplier, float renderProgress, float personalitySeed) override;
    bool ApplyElectricSparkAt(LogicalPixelCoordinates const & screenCoordinates, float progress) override;
    void DrawTo(LogicalPixelCoordinates const & screenCoordinates, float strengthFraction) override;
    void SwirlAt(LogicalPixelCoordinates const & screenCoordinates, float strengthFraction) override;
    void TogglePinAt(LogicalPixelCoordinates const & screenCoordinates) override;
    bool InjectBubblesAt(LogicalPixelCoordinates const & screenCoordinates) override;
    bool FloodAt(LogicalPixelCoordinates const & screenCoordinates, float waterQuantityMultiplier) override;
    void ToggleAntiMatterBombAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void ToggleImpactBombAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void TogglePhysicsProbeAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void ToggleRCBombAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void ToggleTimerBombAt(LogicalPixelCoordinates const & screenCoordinates) override;
    void DetonateRCBombs() override;
    void DetonateAntiMatterBombs() override;
    void AdjustOceanSurfaceTo(std::optional<LogicalPixelCoordinates> const & screenCoordinates) override;
    std::optional<bool> AdjustOceanFloorTo(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) override;
    bool ScrubThrough(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) override;
    bool RotThrough(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) override;
    void ApplyThanosSnapAt(LogicalPixelCoordinates const & screenCoordinates) override;
    std::optional<ElementId> GetNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const override;
    void QueryNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const override;

    void TriggerTsunami() override;
    void TriggerRogueWave() override;
    void TriggerStorm() override;
    void TriggerLightning() override;

    void HighlightElectricalElement(ElectricalElementId electricalElementId) override;

    void SetSwitchState(
        ElectricalElementId electricalElementId,
        ElectricalState switchState) override;

    void SetEngineControllerState(
        ElectricalElementId electricalElementId,
        int telegraphValue) override;

    bool DestroyTriangle(ElementId triangleId) override;
    bool RestoreTriangle(ElementId triangleId) override;

    //
    // Render controls
    //

    void SetCanvasSize(LogicalPixelSize const & canvasSize) override;
    void Pan(LogicalPixelSize const & screenOffset) override;
    void PanImmediate(LogicalPixelSize const & screenOffset) override;
    void PanToWorldEnd(int side) override;
    void ResetPan() override;
    void AdjustZoom(float amount) override;
    void ResetZoom() override;
    vec2f ScreenToWorld(LogicalPixelCoordinates const & screenCoordinates) const override;
    vec2f ScreenOffsetToWorldOffset(LogicalPixelSize const & screenOffset) const override;

    //
    // Interaction parameters
    //

    bool GetDoShowTsunamiNotifications() const override { return mDoShowTsunamiNotifications; }
    void SetDoShowTsunamiNotifications(bool value) override { mDoShowTsunamiNotifications = value; }

    bool GetDoShowElectricalNotifications() const override { return mGameParameters.DoShowElectricalNotifications; }
    void SetDoShowElectricalNotifications(bool value) override { mGameParameters.DoShowElectricalNotifications = value; }

    bool GetDoAutoZoomOnShipLoad() const override { return mDoAutoZoomOnShipLoad; }
    void SetDoAutoZoomOnShipLoad(bool value) override { mDoAutoZoomOnShipLoad = value; }

    ShipAutoTexturizationSettings const & GetShipAutoTexturizationDefaultSettings() const override { return mShipTexturizer.GetDefaultSettings(); }
    ShipAutoTexturizationSettings & GetShipAutoTexturizationDefaultSettings() override { return mShipTexturizer.GetDefaultSettings(); }
    void SetShipAutoTexturizationDefaultSettings(ShipAutoTexturizationSettings const & value) override { mShipTexturizer.SetDefaultSettings(value); }

    bool GetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings() const override { return mShipTexturizer.GetDoForceDefaultSettingsOntoShipSettings(); }
    void SetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings(bool value) override { mShipTexturizer.SetDoForceDefaultSettingsOntoShipSettings(value); }

    /////////////////////////////////////////////////////////
    // IGameControllerSettings and IGameControllerSettingsOptions
    /////////////////////////////////////////////////////////

    //
    // Game parameters
    //

    float GetSimulationStepTimeDuration() const override { return GameParameters::SimulationStepTimeDuration<float>; }

    float GetNumMechanicalDynamicsIterationsAdjustment() const override { return mGameParameters.NumMechanicalDynamicsIterationsAdjustment; }
    void SetNumMechanicalDynamicsIterationsAdjustment(float value) override { mGameParameters.NumMechanicalDynamicsIterationsAdjustment = value; }
    float GetMinNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MinNumMechanicalDynamicsIterationsAdjustment; }
    float GetMaxNumMechanicalDynamicsIterationsAdjustment() const override { return GameParameters::MaxNumMechanicalDynamicsIterationsAdjustment; }

    float GetSpringStiffnessAdjustment() const override { return mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStiffnessAdjustment(float value) override { mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStiffnessAdjustment() const override { return GameParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const override { return GameParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const override { return mGameParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value) override { mGameParameters.SpringDampingAdjustment = value; }
    float GetMinSpringDampingAdjustment() const override { return GameParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const override { return GameParameters::MaxSpringDampingAdjustment; }

    float GetSpringStrengthAdjustment() const override { return mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStrengthAdjustment(float value) override { mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStrengthAdjustment() const override { return GameParameters::MinSpringStrengthAdjustment; }
    float GetMaxSpringStrengthAdjustment() const override { return GameParameters::MaxSpringStrengthAdjustment; }

    float GetGlobalDampingAdjustment() const override { return mGameParameters.GlobalDampingAdjustment; }
    void SetGlobalDampingAdjustment(float value) override { mGameParameters.GlobalDampingAdjustment = value; }
    float GetMinGlobalDampingAdjustment() const override { return GameParameters::MinGlobalDampingAdjustment; }
    float GetMaxGlobalDampingAdjustment() const override { return GameParameters::MaxGlobalDampingAdjustment; }

    float GetRotAcceler8r() const override { return mGameParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) override { mGameParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const override { return GameParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const override { return GameParameters::MaxRotAcceler8r; }

    float GetAirFrictionDragAdjustment() const override { return mGameParameters.AirFrictionDragAdjustment; }
    void SetAirFrictionDragAdjustment(float value) override { mGameParameters.AirFrictionDragAdjustment = value; }
    float GetMinAirFrictionDragAdjustment() const override { return GameParameters::MinAirFrictionDragAdjustment; }
    float GetMaxAirFrictionDragAdjustment() const override { return GameParameters::MaxAirFrictionDragAdjustment; }

    float GetAirPressureDragAdjustment() const override { return mGameParameters.AirPressureDragAdjustment; }
    void SetAirPressureDragAdjustment(float value) override { mGameParameters.AirPressureDragAdjustment = value; }
    float GetMinAirPressureDragAdjustment() const override { return GameParameters::MinAirPressureDragAdjustment; }
    float GetMaxAirPressureDragAdjustment() const override { return GameParameters::MaxAirPressureDragAdjustment; }

    float GetWaterDensityAdjustment() const override { return mGameParameters.WaterDensityAdjustment; }
    void SetWaterDensityAdjustment(float value) override { mGameParameters.WaterDensityAdjustment = value; }
    float GetMinWaterDensityAdjustment() const override { return GameParameters::MinWaterDensityAdjustment; }
    float GetMaxWaterDensityAdjustment() const override { return GameParameters::MaxWaterDensityAdjustment; }

    float GetWaterFrictionDragAdjustment() const override { return mGameParameters.WaterFrictionDragAdjustment; }
    void SetWaterFrictionDragAdjustment(float value) override { mGameParameters.WaterFrictionDragAdjustment = value; }
    float GetMinWaterFrictionDragAdjustment() const override { return GameParameters::MinWaterFrictionDragAdjustment; }
    float GetMaxWaterFrictionDragAdjustment() const override { return GameParameters::MaxWaterFrictionDragAdjustment; }

    float GetWaterPressureDragAdjustment() const override { return mGameParameters.WaterPressureDragAdjustment; }
    void SetWaterPressureDragAdjustment(float value) override { mGameParameters.WaterPressureDragAdjustment = value; }
    float GetMinWaterPressureDragAdjustment() const override { return GameParameters::MinWaterPressureDragAdjustment; }
    float GetMaxWaterPressureDragAdjustment() const override { return GameParameters::MaxWaterPressureDragAdjustment; }

    float GetHydrostaticPressureAdjustment() const override { return mGameParameters.HydrostaticPressureAdjustment; }
    void SetHydrostaticPressureAdjustment(float value) override { mGameParameters.HydrostaticPressureAdjustment = value; }
    float GetMinHydrostaticPressureAdjustment() const override { return GameParameters::MinHydrostaticPressureAdjustment; }
    float GetMaxHydrostaticPressureAdjustment() const override { return GameParameters::MaxHydrostaticPressureAdjustment; }

    float GetWaterIntakeAdjustment() const override { return mGameParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) override { mGameParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const override { return GameParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const override { return GameParameters::MaxWaterIntakeAdjustment; }

    float GetWaterDiffusionSpeedAdjustment() const override { return mGameParameters.WaterDiffusionSpeedAdjustment; }
    void SetWaterDiffusionSpeedAdjustment(float value) override { mGameParameters.WaterDiffusionSpeedAdjustment = value; }
    float GetMinWaterDiffusionSpeedAdjustment() const override { return GameParameters::MinWaterDiffusionSpeedAdjustment; }
    float GetMaxWaterDiffusionSpeedAdjustment() const override { return GameParameters::MaxWaterDiffusionSpeedAdjustment; }

    float GetWaterCrazyness() const override { return mGameParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) override { mGameParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const override { return GameParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const override { return GameParameters::MaxWaterCrazyness; }

    float GetSmokeEmissionDensityAdjustment() const override { return mGameParameters.SmokeEmissionDensityAdjustment; }
    void SetSmokeEmissionDensityAdjustment(float value) override { mGameParameters.SmokeEmissionDensityAdjustment = value; }
    float GetMinSmokeEmissionDensityAdjustment() const override { return GameParameters::MinSmokeEmissionDensityAdjustment; }
    float GetMaxSmokeEmissionDensityAdjustment() const override { return GameParameters::MaxSmokeEmissionDensityAdjustment; }

    float GetSmokeParticleLifetimeAdjustment() const override { return mGameParameters.SmokeParticleLifetimeAdjustment; }
    void SetSmokeParticleLifetimeAdjustment(float value) override { mGameParameters.SmokeParticleLifetimeAdjustment = value; }
    float GetMinSmokeParticleLifetimeAdjustment() const override { return GameParameters::MinSmokeParticleLifetimeAdjustment; }
    float GetMaxSmokeParticleLifetimeAdjustment() const override { return GameParameters::MaxSmokeParticleLifetimeAdjustment; }

    bool GetDoModulateWind() const override { return mGameParameters.DoModulateWind; }
    void SetDoModulateWind(bool value) override { mGameParameters.DoModulateWind = value; }

    float GetWindSpeedBase() const override { return mGameParameters.WindSpeedBase; }
    void SetWindSpeedBase(float value) override { mGameParameters.WindSpeedBase = value; }
    float GetMinWindSpeedBase() const override { return GameParameters::MinWindSpeedBase; }
    float GetMaxWindSpeedBase() const override { return GameParameters::MaxWindSpeedBase; }

    float GetWindSpeedMaxFactor() const override { return mGameParameters.WindSpeedMaxFactor; }
    void SetWindSpeedMaxFactor(float value) override { mGameParameters.WindSpeedMaxFactor = value; }
    float GetMinWindSpeedMaxFactor() const override { return GameParameters::MinWindSpeedMaxFactor; }
    float GetMaxWindSpeedMaxFactor() const override { return GameParameters::MaxWindSpeedMaxFactor; }

    float GetBasalWaveHeightAdjustment() const override { return mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].GetValue(); }
    void SetBasalWaveHeightAdjustment(float value) override { mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].SetValue(value); }
    float GetMinBasalWaveHeightAdjustment() const override { return GameParameters::MinBasalWaveHeightAdjustment; }
    float GetMaxBasalWaveHeightAdjustment() const override { return GameParameters::MaxBasalWaveHeightAdjustment; }

    float GetBasalWaveLengthAdjustment() const override { return mGameParameters.BasalWaveLengthAdjustment; }
    void SetBasalWaveLengthAdjustment(float value) override { mGameParameters.BasalWaveLengthAdjustment = value; }
    float GetMinBasalWaveLengthAdjustment() const override { return GameParameters::MinBasalWaveLengthAdjustment; }
    float GetMaxBasalWaveLengthAdjustment() const override { return GameParameters::MaxBasalWaveLengthAdjustment; }

    float GetBasalWaveSpeedAdjustment() const override { return mGameParameters.BasalWaveSpeedAdjustment; }
    void SetBasalWaveSpeedAdjustment(float value) override { mGameParameters.BasalWaveSpeedAdjustment = value; }
    float GetMinBasalWaveSpeedAdjustment() const override { return GameParameters::MinBasalWaveSpeedAdjustment; }
    float GetMaxBasalWaveSpeedAdjustment() const override { return GameParameters::MaxBasalWaveSpeedAdjustment; }

    std::chrono::minutes GetTsunamiRate() const override { return mGameParameters.TsunamiRate; }
    void SetTsunamiRate(std::chrono::minutes value) override { mGameParameters.TsunamiRate = value; }
    std::chrono::minutes GetMinTsunamiRate() const override { return GameParameters::MinTsunamiRate; }
    std::chrono::minutes GetMaxTsunamiRate() const override { return GameParameters::MaxTsunamiRate; }

    std::chrono::seconds GetRogueWaveRate() const override { return mGameParameters.RogueWaveRate; }
    void SetRogueWaveRate(std::chrono::seconds value) override { mGameParameters.RogueWaveRate = value; }
    std::chrono::seconds GetMinRogueWaveRate() const override { return GameParameters::MinRogueWaveRate; }
    std::chrono::seconds GetMaxRogueWaveRate() const override { return GameParameters::MaxRogueWaveRate; }

    bool GetDoDisplaceWater() const override { return mGameParameters.DoDisplaceWater; }
    void SetDoDisplaceWater(bool value) override { mGameParameters.DoDisplaceWater = value; }

    float GetWaterDisplacementWaveHeightAdjustment() const override { return mGameParameters.WaterDisplacementWaveHeightAdjustment; }
    void SetWaterDisplacementWaveHeightAdjustment(float value) override { mGameParameters.WaterDisplacementWaveHeightAdjustment = value; }
    float GetMinWaterDisplacementWaveHeightAdjustment() const override { return GameParameters::MinWaterDisplacementWaveHeightAdjustment; }
    float GetMaxWaterDisplacementWaveHeightAdjustment() const override { return GameParameters::MaxWaterDisplacementWaveHeightAdjustment; }

    float GetWaveSmoothnessAdjustment() const override { return mGameParameters.WaveSmoothnessAdjustment; }
    void SetWaveSmoothnessAdjustment(float value) override { mGameParameters.WaveSmoothnessAdjustment = value; }
    float GetMinWaveSmoothnessAdjustment() const override { return GameParameters::MinWaveSmoothnessAdjustment; }
    float GetMaxWaveSmoothnessAdjustment() const override { return GameParameters::MaxWaveSmoothnessAdjustment; }

    // Storm

    std::chrono::minutes GetStormRate() const override { return mGameParameters.StormRate; }
    void SetStormRate(std::chrono::minutes value) override { mGameParameters.StormRate = value; }
    std::chrono::minutes GetMinStormRate() const override { return GameParameters::MinStormRate; }
    std::chrono::minutes GetMaxStormRate() const override { return GameParameters::MaxStormRate; }

    std::chrono::seconds GetStormDuration() const override { return mGameParameters.StormDuration; }
    void SetStormDuration(std::chrono::seconds value) override { mGameParameters.StormDuration = value; }
    std::chrono::seconds GetMinStormDuration() const override { return GameParameters::MinStormDuration; }
    std::chrono::seconds GetMaxStormDuration() const override { return GameParameters::MaxStormDuration; }

    float GetStormStrengthAdjustment() const override { return mGameParameters.StormStrengthAdjustment; }
    void SetStormStrengthAdjustment(float value) override { mGameParameters.StormStrengthAdjustment = value; }
    float GetMinStormStrengthAdjustment() const override { return GameParameters::MinStormStrengthAdjustment; }
    float GetMaxStormStrengthAdjustment() const override { return GameParameters::MaxStormStrengthAdjustment; }

    bool GetDoRainWithStorm() const override { return mGameParameters.DoRainWithStorm; }
    void SetDoRainWithStorm(bool value) override { mGameParameters.DoRainWithStorm = value; }

    float GetRainFloodAdjustment() const override { return mGameParameters.RainFloodAdjustment; }
    void SetRainFloodAdjustment(float value) override { mGameParameters.RainFloodAdjustment = value; }
    float GetMinRainFloodAdjustment() const override { return GameParameters::MinRainFloodAdjustment; }
    float GetMaxRainFloodAdjustment() const override { return GameParameters::MaxRainFloodAdjustment; }

    // Heat

    float GetAirTemperature() const override { return mGameParameters.AirTemperature; }
    void SetAirTemperature(float value) override { mGameParameters.AirTemperature = value; }
    float GetMinAirTemperature() const override { return GameParameters::MinAirTemperature; }
    float GetMaxAirTemperature() const override { return GameParameters::MaxAirTemperature; }

    float GetWaterTemperature() const override { return mGameParameters.WaterTemperature; }
    void SetWaterTemperature(float value) override { mGameParameters.WaterTemperature = value; }
    float GetMinWaterTemperature() const override { return GameParameters::MinWaterTemperature; }
    float GetMaxWaterTemperature() const override { return GameParameters::MaxWaterTemperature; }

    unsigned int GetMaxBurningParticles() const override { return mGameParameters.MaxBurningParticles; }
    void SetMaxBurningParticles(unsigned int value) override { mGameParameters.MaxBurningParticles = value; }
    unsigned int GetMinMaxBurningParticles() const override { return GameParameters::MinMaxBurningParticles; }
    unsigned int GetMaxMaxBurningParticles() const override { return GameParameters::MaxMaxBurningParticles; }

    float GetThermalConductivityAdjustment() const override { return mGameParameters.ThermalConductivityAdjustment; }
    void SetThermalConductivityAdjustment(float value) override { mGameParameters.ThermalConductivityAdjustment = value; }
    float GetMinThermalConductivityAdjustment() const override { return GameParameters::MinThermalConductivityAdjustment; }
    float GetMaxThermalConductivityAdjustment() const override { return GameParameters::MaxThermalConductivityAdjustment; }

    float GetHeatDissipationAdjustment() const override { return mGameParameters.HeatDissipationAdjustment; }
    void SetHeatDissipationAdjustment(float value) override { mGameParameters.HeatDissipationAdjustment = value; }
    float GetMinHeatDissipationAdjustment() const override { return GameParameters::MinHeatDissipationAdjustment; }
    float GetMaxHeatDissipationAdjustment() const override { return GameParameters::MaxHeatDissipationAdjustment; }

    float GetIgnitionTemperatureAdjustment() const override { return mGameParameters.IgnitionTemperatureAdjustment; }
    void SetIgnitionTemperatureAdjustment(float value) override { mGameParameters.IgnitionTemperatureAdjustment = value; }
    float GetMinIgnitionTemperatureAdjustment() const override { return GameParameters::MinIgnitionTemperatureAdjustment; }
    float GetMaxIgnitionTemperatureAdjustment() const override { return GameParameters::MaxIgnitionTemperatureAdjustment; }

    float GetMeltingTemperatureAdjustment() const override { return mGameParameters.MeltingTemperatureAdjustment; }
    void SetMeltingTemperatureAdjustment(float value) override { mGameParameters.MeltingTemperatureAdjustment = value; }
    float GetMinMeltingTemperatureAdjustment() const override { return GameParameters::MinMeltingTemperatureAdjustment; }
    float GetMaxMeltingTemperatureAdjustment() const override { return GameParameters::MaxMeltingTemperatureAdjustment; }

    float GetCombustionSpeedAdjustment() const override { return mGameParameters.CombustionSpeedAdjustment; }
    void SetCombustionSpeedAdjustment(float value) override { mGameParameters.CombustionSpeedAdjustment = value; }
    float GetMinCombustionSpeedAdjustment() const override { return GameParameters::MinCombustionSpeedAdjustment; }
    float GetMaxCombustionSpeedAdjustment() const override { return GameParameters::MaxCombustionSpeedAdjustment; }

    float GetCombustionHeatAdjustment() const override { return mGameParameters.CombustionHeatAdjustment; }
    void SetCombustionHeatAdjustment(float value) override { mGameParameters.CombustionHeatAdjustment = value; }
    float GetMinCombustionHeatAdjustment() const override { return GameParameters::MinCombustionHeatAdjustment; }
    float GetMaxCombustionHeatAdjustment() const override { return GameParameters::MaxCombustionHeatAdjustment; }

    float GetHeatBlasterHeatFlow() const override { return mGameParameters.HeatBlasterHeatFlow; }
    void SetHeatBlasterHeatFlow(float value) override { mGameParameters.HeatBlasterHeatFlow = value; }
    float GetMinHeatBlasterHeatFlow() const override { return GameParameters::MinHeatBlasterHeatFlow; }
    float GetMaxHeatBlasterHeatFlow() const override { return GameParameters::MaxHeatBlasterHeatFlow; }

    float GetHeatBlasterRadius() const override { return mGameParameters.HeatBlasterRadius; }
    void SetHeatBlasterRadius(float value) override { mGameParameters.HeatBlasterRadius = value; }
    float GetMinHeatBlasterRadius() const override { return GameParameters::MinHeatBlasterRadius; }
    float GetMaxHeatBlasterRadius() const override { return GameParameters::MaxHeatBlasterRadius; }

    float GetElectricalElementHeatProducedAdjustment() const override { return mGameParameters.ElectricalElementHeatProducedAdjustment; }
    void SetElectricalElementHeatProducedAdjustment(float value) override { mGameParameters.ElectricalElementHeatProducedAdjustment = value; }
    float GetMinElectricalElementHeatProducedAdjustment() const override { return GameParameters::MinElectricalElementHeatProducedAdjustment; }
    float GetMaxElectricalElementHeatProducedAdjustment() const override { return GameParameters::MaxElectricalElementHeatProducedAdjustment; }

    float GetEngineThrustAdjustment() const override { return mGameParameters.EngineThrustAdjustment; }
    void SetEngineThrustAdjustment(float value) override { mGameParameters.EngineThrustAdjustment = value; }
    float GetMinEngineThrustAdjustment() const override { return GameParameters::MinEngineThrustAdjustment; }
    float GetMaxEngineThrustAdjustment() const override { return GameParameters::MaxEngineThrustAdjustment; }

    float GetWaterPumpPowerAdjustment() const override { return mGameParameters.WaterPumpPowerAdjustment; }
    void SetWaterPumpPowerAdjustment(float value) override { mGameParameters.WaterPumpPowerAdjustment = value; }
    float GetMinWaterPumpPowerAdjustment() const override { return GameParameters::MinWaterPumpPowerAdjustment; }
    float GetMaxWaterPumpPowerAdjustment() const override { return GameParameters::MaxWaterPumpPowerAdjustment; }

    // Fishes

    unsigned int GetNumberOfFishes() const override { return mGameParameters.NumberOfFishes; }
    void SetNumberOfFishes(unsigned int value) override { mGameParameters.NumberOfFishes = value; }
    unsigned int GetMinNumberOfFishes() const override { return GameParameters::MinNumberOfFishes; }
    unsigned int GetMaxNumberOfFishes() const override { return GameParameters::MaxNumberOfFishes; }

    float GetFishSizeMultiplier() const override { return mFloatParameterSmoothers[FishSizeMultiplierParameterSmoother].GetValue(); }
    void SetFishSizeMultiplier(float value) override { mFloatParameterSmoothers[FishSizeMultiplierParameterSmoother].SetValue(value); }
    float GetMinFishSizeMultiplier() const override { return GameParameters::MinFishSizeMultiplier; }
    float GetMaxFishSizeMultiplier() const override { return GameParameters::MaxFishSizeMultiplier; }

    float GetFishSpeedAdjustment() const override { return mGameParameters.FishSpeedAdjustment; }
    void SetFishSpeedAdjustment(float value) override { mGameParameters.FishSpeedAdjustment = value; }
    float GetMinFishSpeedAdjustment() const override { return GameParameters::MinFishSpeedAdjustment; }
    float GetMaxFishSpeedAdjustment() const override { return GameParameters::MaxFishSpeedAdjustment; }

    bool GetDoFishShoaling() const override { return mGameParameters.DoFishShoaling; }
    void SetDoFishShoaling(bool value) override { mGameParameters.DoFishShoaling = value; }

    float GetFishShoalRadiusAdjustment() const override { return mGameParameters.FishShoalRadiusAdjustment; }
    void SetFishShoalRadiusAdjustment(float value) override { mGameParameters.FishShoalRadiusAdjustment = value; }
    float GetMinFishShoalRadiusAdjustment() const override { return GameParameters::MinFishShoalRadiusAdjustment; }
    float GetMaxFishShoalRadiusAdjustment() const override { return GameParameters::MaxFishShoalRadiusAdjustment; }

    // Misc

    OceanFloorTerrain const & GetOceanFloorTerrain() const override { return mWorld->GetOceanFloorTerrain(); }
    void SetOceanFloorTerrain(OceanFloorTerrain const & value) override { mWorld->SetOceanFloorTerrain(value); }

    float GetSeaDepth() const override { return mFloatParameterSmoothers[SeaDepthParameterSmoother].GetValue(); }
    void SetSeaDepth(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValue(value); }
    void SetSeaDepthImmediate(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValueImmediate(value); }
    float GetMinSeaDepth() const override { return GameParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const override { return GameParameters::MaxSeaDepth; }

    float GetOceanFloorBumpiness() const override { return mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].GetValue(); }
    void SetOceanFloorBumpiness(float value) override { mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].SetValue(value); }
    float GetMinOceanFloorBumpiness() const override { return GameParameters::MinOceanFloorBumpiness; }
    float GetMaxOceanFloorBumpiness() const override { return GameParameters::MaxOceanFloorBumpiness; }

    float GetOceanFloorDetailAmplification() const override { return mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].GetValue(); }
    void SetOceanFloorDetailAmplification(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValue(value); }
    void SetOceanFloorDetailAmplificationImmediate(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValueImmediate(value); }
    float GetMinOceanFloorDetailAmplification() const override { return GameParameters::MinOceanFloorDetailAmplification; }
    float GetMaxOceanFloorDetailAmplification() const override { return GameParameters::MaxOceanFloorDetailAmplification; }

    float GetOceanFloorElasticity() const override { return mGameParameters.OceanFloorElasticity; }
    void SetOceanFloorElasticity(float value) override { mGameParameters.OceanFloorElasticity = value; }
    float GetMinOceanFloorElasticity() const override { return GameParameters::MinOceanFloorElasticity; }
    float GetMaxOceanFloorElasticity() const override { return GameParameters::MaxOceanFloorElasticity; }

    float GetOceanFloorFriction() const override { return mGameParameters.OceanFloorFriction; }
    void SetOceanFloorFriction(float value) override { mGameParameters.OceanFloorFriction = value; }
    float GetMinOceanFloorFriction() const override { return GameParameters::MinOceanFloorFriction; }
    float GetMaxOceanFloorFriction() const override { return GameParameters::MaxOceanFloorFriction; }

    float GetDestroyRadius() const override { return mGameParameters.DestroyRadius; }
    void SetDestroyRadius(float value) override { mGameParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const override { return GameParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const override { return GameParameters::MaxDestroyRadius; }

    float GetRepairRadius() const override { return mGameParameters.RepairRadius; }
    void SetRepairRadius(float value) override { mGameParameters.RepairRadius = value; }
    float GetMinRepairRadius() const override { return GameParameters::MinRepairRadius; }
    float GetMaxRepairRadius() const override { return GameParameters::MaxRepairRadius; }

    float GetRepairSpeedAdjustment() const override { return mGameParameters.RepairSpeedAdjustment; }
    void SetRepairSpeedAdjustment(float value) override { mGameParameters.RepairSpeedAdjustment = value; }
    float GetMinRepairSpeedAdjustment() const override { return GameParameters::MinRepairSpeedAdjustment; }
    float GetMaxRepairSpeedAdjustment() const override { return GameParameters::MaxRepairSpeedAdjustment; }

    float GetBombBlastRadius() const override { return mGameParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) override { mGameParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const override { return GameParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const override { return GameParameters::MaxBombBlastRadius; }

    float GetBombBlastForceAdjustment() const override { return mGameParameters.BombBlastForceAdjustment; }
    void SetBombBlastForceAdjustment(float value) override { mGameParameters.BombBlastForceAdjustment = value; }
    float GetMinBombBlastForceAdjustment() const override { return GameParameters::MinBombBlastForceAdjustment; }
    float GetMaxBombBlastForceAdjustment() const override { return GameParameters::MaxBombBlastForceAdjustment; }

    float GetBombBlastHeat() const override { return mGameParameters.BombBlastHeat; }
    void SetBombBlastHeat(float value) override { mGameParameters.BombBlastHeat = value; }
    float GetMinBombBlastHeat() const override { return GameParameters::MinBombBlastHeat; }
    float GetMaxBombBlastHeat() const override { return GameParameters::MaxBombBlastHeat; }

    float GetAntiMatterBombImplosionStrength() const override { return mGameParameters.AntiMatterBombImplosionStrength; }
    void SetAntiMatterBombImplosionStrength(float value) override { mGameParameters.AntiMatterBombImplosionStrength = value; }
    float GetMinAntiMatterBombImplosionStrength() const override { return GameParameters::MinAntiMatterBombImplosionStrength; }
    float GetMaxAntiMatterBombImplosionStrength() const override { return GameParameters::MaxAntiMatterBombImplosionStrength; }

    float GetFloodRadius() const override { return mGameParameters.FloodRadius; }
    void SetFloodRadius(float value) override { mGameParameters.FloodRadius = value; }
    float GetMinFloodRadius() const override { return GameParameters::MinFloodRadius; }
    float GetMaxFloodRadius() const override { return GameParameters::MaxFloodRadius; }

    float GetFloodQuantity() const override { return mGameParameters.FloodQuantity; }
    void SetFloodQuantity(float value) override { mGameParameters.FloodQuantity = value; }
    float GetMinFloodQuantity() const override { return GameParameters::MinFloodQuantity; }
    float GetMaxFloodQuantity() const override { return GameParameters::MaxFloodQuantity; }

    float GetBlastToolRadius() const override { return mGameParameters.BlastToolRadius; }
    void SetBlastToolRadius(float value) override { mGameParameters.BlastToolRadius = value; }
    float GetMinBlastToolRadius() const override { return GameParameters::MinBlastToolRadius; }
    float GetMaxBlastToolRadius() const override { return GameParameters::MaxBlastToolRadius; }

    float GetBlastToolForceAdjustment() const override { return mGameParameters.BlastToolForceAdjustment; }
    void SetBlastToolForceAdjustment(float value) override { mGameParameters.BlastToolForceAdjustment = value; }
    float GetMinBlastToolForceAdjustment() const override { return GameParameters::MinBlastToolForceAdjustment; }
    float GetMaxBlastToolForceAdjustment() const override { return GameParameters::MaxBlastToolForceAdjustment; }

    float GetScrubRotRadius() const override { return mGameParameters.ScrubRotRadius; }
    void SetScrubRotRadius(float value) override { mGameParameters.ScrubRotRadius = value; }
    float GetMinScrubRotRadius() const override { return GameParameters::MinScrubRotRadius; }
    float GetMaxScrubRotRadius() const override { return GameParameters::MaxScrubRotRadius; }

    float GetLuminiscenceAdjustment() const override { return mGameParameters.LuminiscenceAdjustment; }
    void SetLuminiscenceAdjustment(float value) override { mGameParameters.LuminiscenceAdjustment = value; }
    float GetMinLuminiscenceAdjustment() const override { return GameParameters::MinLuminiscenceAdjustment; }
    float GetMaxLuminiscenceAdjustment() const override { return GameParameters::MaxLuminiscenceAdjustment; }

    float GetLightSpreadAdjustment() const override { return mGameParameters.LightSpreadAdjustment; }
    void SetLightSpreadAdjustment(float value) override { mGameParameters.LightSpreadAdjustment = value; }
    float GetMinLightSpreadAdjustment() const override { return GameParameters::MinLightSpreadAdjustment; }
    float GetMaxLightSpreadAdjustment() const override { return GameParameters::MaxLightSpreadAdjustment; }

    bool GetUltraViolentMode() const override { return mGameParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) override { mGameParameters.IsUltraViolentMode = value; mNotificationLayer.SetUltraViolentModeIndicator(value); }

    bool GetDoGenerateDebris() const override { return mGameParameters.DoGenerateDebris; }
    void SetDoGenerateDebris(bool value) override { mGameParameters.DoGenerateDebris = value; }

    bool GetDoGenerateSparklesForCuts() const override { return mGameParameters.DoGenerateSparklesForCuts; }
    void SetDoGenerateSparklesForCuts(bool value) override { mGameParameters.DoGenerateSparklesForCuts = value; }

    float GetAirBubblesDensity() const override { return mGameParameters.AirBubblesDensity; }
    void SetAirBubblesDensity(float value) override { mGameParameters.AirBubblesDensity = value; }
    float GetMaxAirBubblesDensity() const override { return GameParameters::MaxAirBubblesDensity; }
    float GetMinAirBubblesDensity() const override { return GameParameters::MinAirBubblesDensity; }

    bool GetDoGenerateEngineWakeParticles() const override { return mGameParameters.DoGenerateEngineWakeParticles; }
    void SetDoGenerateEngineWakeParticles(bool value) override { mGameParameters.DoGenerateEngineWakeParticles = value; }

    unsigned int GetNumberOfStars() const override { return mGameParameters.NumberOfStars; }
    void SetNumberOfStars(unsigned int value) override { mGameParameters.NumberOfStars = value; }
    unsigned int GetMinNumberOfStars() const override { return GameParameters::MinNumberOfStars; }
    unsigned int GetMaxNumberOfStars() const override { return GameParameters::MaxNumberOfStars; }

    unsigned int GetNumberOfClouds() const override { return mGameParameters.NumberOfClouds; }
    void SetNumberOfClouds(unsigned int value) override { mGameParameters.NumberOfClouds = value; }
    unsigned int GetMinNumberOfClouds() const override { return GameParameters::MinNumberOfClouds; }
    unsigned int GetMaxNumberOfClouds() const override { return GameParameters::MaxNumberOfClouds; }

    bool GetDoDayLightCycle() const override { return mGameParameters.DoDayLightCycle; }
    void SetDoDayLightCycle(bool value) override;

    std::chrono::minutes GetDayLightCycleDuration() const override { return mGameParameters.DayLightCycleDuration; }
    void SetDayLightCycleDuration(std::chrono::minutes value) override { mGameParameters.DayLightCycleDuration = value; }
    std::chrono::minutes GetMinDayLightCycleDuration() const override { return GameParameters::MinDayLightCycleDuration; }
    std::chrono::minutes GetMaxDayLightCycleDuration() const override { return GameParameters::MaxDayLightCycleDuration; }

    //
    // Render parameters
    //

    rgbColor const & GetFlatSkyColor() const override { return mRenderContext->GetFlatSkyColor(); }
    void SetFlatSkyColor(rgbColor const & color) override { mRenderContext->SetFlatSkyColor(color); }

    float GetAmbientLightIntensity() const override { return mRenderContext->GetAmbientLightIntensity(); }
    void SetAmbientLightIntensity(float value) override { mRenderContext->SetAmbientLightIntensity(value); }

    float GetOceanTransparency() const override { return mRenderContext->GetOceanTransparency(); }
    void SetOceanTransparency(float value) override { mRenderContext->SetOceanTransparency(value); }

    float GetOceanDarkeningRate() const override { return mRenderContext->GetOceanDarkeningRate(); }
    void SetOceanDarkeningRate(float value) override { mRenderContext->SetOceanDarkeningRate(value); }

    rgbColor const & GetFlatLampLightColor() const override { return mRenderContext->GetFlatLampLightColor(); }
    void SetFlatLampLightColor(rgbColor const & color) override { mRenderContext->SetFlatLampLightColor(color); }

    rgbColor const & GetDefaultWaterColor() const override { return mRenderContext->GetShipDefaultWaterColor(); }
    void SetDefaultWaterColor(rgbColor const & color) override { mRenderContext->SetShipDefaultWaterColor(color); }

    float GetWaterContrast() const override { return mRenderContext->GetShipWaterContrast(); }
    void SetWaterContrast(float value) override { mRenderContext->SetShipWaterContrast(value); }

    float GetWaterLevelOfDetail() const override { return mRenderContext->GetShipWaterLevelOfDetail(); }
    void SetWaterLevelOfDetail(float value) override { mRenderContext->SetShipWaterLevelOfDetail(value); }
    float GetMinWaterLevelOfDetail() const override { return Render::RenderContext::MinShipWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const override { return Render::RenderContext::MaxShipWaterLevelOfDetail; }

    bool GetShowShipThroughOcean() const override { return mRenderContext->GetShowShipThroughOcean(); }
    void SetShowShipThroughOcean(bool value) override { mRenderContext->SetShowShipThroughOcean(value); }

    DebugShipRenderModeType GetDebugShipRenderMode() const override { return mRenderContext->GetDebugShipRenderMode(); }
    void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode) override { mRenderContext->SetDebugShipRenderMode(debugShipRenderMode); }

    OceanRenderModeType GetOceanRenderMode() const override { return mRenderContext->GetOceanRenderMode(); }
    void SetOceanRenderMode(OceanRenderModeType oceanRenderMode) override { mRenderContext->SetOceanRenderMode(oceanRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const override { return mRenderContext->GetTextureOceanAvailableThumbnails(); }
    size_t GetTextureOceanTextureIndex() const override { return mRenderContext->GetTextureOceanTextureIndex(); }
    void SetTextureOceanTextureIndex(size_t index) override { mRenderContext->SetTextureOceanTextureIndex(index); }

    rgbColor const & GetDepthOceanColorStart() const override { return mRenderContext->GetDepthOceanColorStart(); }
    void SetDepthOceanColorStart(rgbColor const & color) override { mRenderContext->SetDepthOceanColorStart(color); }

    rgbColor const & GetDepthOceanColorEnd() const override { return mRenderContext->GetDepthOceanColorEnd(); }
    void SetDepthOceanColorEnd(rgbColor const & color) override { mRenderContext->SetDepthOceanColorEnd(color); }

    rgbColor const & GetFlatOceanColor() const override { return mRenderContext->GetFlatOceanColor(); }
    void SetFlatOceanColor(rgbColor const & color) override { mRenderContext->SetFlatOceanColor(color); }

    OceanRenderDetailType GetOceanRenderDetail() const override { return mRenderContext->GetOceanRenderDetail(); }
    void SetOceanRenderDetail(OceanRenderDetailType oceanRenderDetail) override { mRenderContext->SetOceanRenderDetail(oceanRenderDetail); }

    LandRenderModeType GetLandRenderMode() const override { return mRenderContext->GetLandRenderMode(); }
    void SetLandRenderMode(LandRenderModeType landRenderMode) override { mRenderContext->SetLandRenderMode(landRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const override { return mRenderContext->GetTextureLandAvailableThumbnails(); }
    size_t GetTextureLandTextureIndex() const override { return mRenderContext->GetTextureLandTextureIndex(); }
    void SetTextureLandTextureIndex(size_t index) override { mRenderContext->SetTextureLandTextureIndex(index); }

    rgbColor const & GetFlatLandColor() const override { return mRenderContext->GetFlatLandColor(); }
    void SetFlatLandColor(rgbColor const & color) override { mRenderContext->SetFlatLandColor(color); }

    VectorFieldRenderModeType GetVectorFieldRenderMode() const override { return mRenderContext->GetVectorFieldRenderMode(); }
    void SetVectorFieldRenderMode(VectorFieldRenderModeType VectorFieldRenderMode) override { mRenderContext->SetVectorFieldRenderMode(VectorFieldRenderMode); }

    bool GetShowShipStress() const override { return mRenderContext->GetShowStressedSprings(); }
    void SetShowShipStress(bool value) override { mRenderContext->SetShowStressedSprings(value); }

    bool GetShowShipFrontiers() const override { return mRenderContext->GetShowFrontiers(); }
    void SetShowShipFrontiers(bool value) override { mRenderContext->SetShowFrontiers(value); }

    bool GetShowAABBs() const override { return mRenderContext->GetShowAABBs(); }
    void SetShowAABBs(bool value) override { mRenderContext->SetShowAABBs(value); }

    HeatRenderModeType GetHeatRenderMode() const override { return mRenderContext->GetHeatRenderMode(); }
    void SetHeatRenderMode(HeatRenderModeType value) override { mRenderContext->SetHeatRenderMode(value); }

    float GetHeatSensitivity() const override { return mRenderContext->GetHeatSensitivity(); }
    void SetHeatSensitivity(float value) override { mRenderContext->SetHeatSensitivity(value); }

    bool GetDrawExplosions() const override { return mRenderContext->GetDrawExplosions(); }
    void SetDrawExplosions(bool value) override { mRenderContext->SetDrawExplosions(value); }

    bool GetDrawFlames() const override { return mRenderContext->GetDrawFlames(); }
    void SetDrawFlames(bool value) override { mRenderContext->SetDrawFlames(value); }

    float GetShipFlameSizeAdjustment() const override { return mFloatParameterSmoothers[FlameSizeAdjustmentParameterSmoother].GetValue(); }
    void SetShipFlameSizeAdjustment(float value) override { mFloatParameterSmoothers[FlameSizeAdjustmentParameterSmoother].SetValue(value); }
    float GetMinShipFlameSizeAdjustment() const override { return Render::RenderContext::MinShipFlameSizeAdjustment; }
    float GetMaxShipFlameSizeAdjustment() const override { return Render::RenderContext::MaxShipFlameSizeAdjustment; }

    //
    // Interaction parameters
    //

    bool GetDrawHeatBlasterFlame() const override { return mDoDrawHeatBlasterFlame; }
    void SetDrawHeatBlasterFlame(bool value) override { mDoDrawHeatBlasterFlame = value; }

private:

    //
    // Event handlers
    //

    virtual void OnTsunami(float x) override;

    virtual void OnShipRepaired(ShipId shipId) override;

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::unique_ptr<PerfStats> perfStats,
        FishSpeciesDatabase && fishSpeciesDatabase,
        MaterialDatabase && materialDatabase,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

    ShipMetadata ResetAndLoadShip(
        std::filesystem::path const & shipDefinitionFilepath,
        StrongTypedBool<struct DoAutoZoom> doAutoZoom);

    void Reset(std::unique_ptr<Physics::World> newWorld);

    void OnShipAdded(
        ShipId shipId,
        RgbaImageData && textureImage,
        ShipMetadata const & shipMetadata,
        StrongTypedBool<struct DoAutoZoom> doAutoZoom);

    void PublishStats(std::chrono::steady_clock::time_point nowReal);

private:

    //
    // State machines
    //

    struct TsunamiNotificationStateMachine;
    struct TsunamiNotificationStateMachineDeleter { void operator()(TsunamiNotificationStateMachine *) const; };
    std::unique_ptr<TsunamiNotificationStateMachine, TsunamiNotificationStateMachineDeleter> mTsunamiNotificationStateMachine;

    void StartTsunamiNotificationStateMachine(float x);


    struct ThanosSnapStateMachine;
    struct ThanosSnapStateMachineDeleter { void operator()(ThanosSnapStateMachine *) const; };
    std::vector<std::unique_ptr<ThanosSnapStateMachine, ThanosSnapStateMachineDeleter>> mThanosSnapStateMachines;

    void StartThanosSnapStateMachine(float x, float currentSimulationTime);
    bool UpdateThanosSnapStateMachine(ThanosSnapStateMachine & stateMachine, float currentSimulationTime);


    struct DayLightCycleStateMachine;
    struct DayLightCycleStateMachineDeleter { void operator()(DayLightCycleStateMachine *) const; };
    std::unique_ptr<DayLightCycleStateMachine, DayLightCycleStateMachineDeleter> mDayLightCycleStateMachine;

    void StartDayLightCycleStateMachine();
    void StopDayLightCycleStateMachine();
    bool UpdateDayLightCycleStateMachine(DayLightCycleStateMachine & stateMachine, float currentSimulationTime);


    void ResetStateMachines();
    void UpdateStateMachines(float currentSimulationTime);

private:

    //
    // Our current state
    //

    GameParameters mGameParameters;
    bool mIsPaused;
    bool mIsPulseUpdateSet;
    bool mIsMoveToolEngaged;


    //
    // The parameters that we own
    //

    bool mDoShowTsunamiNotifications;
    bool mDoDrawHeatBlasterFlame;
    bool mDoAutoZoomOnShipLoad;


    //
    // The doers
    //

    std::shared_ptr<Render::RenderContext> mRenderContext;
    std::shared_ptr<GameEventDispatcher> mGameEventDispatcher;
    NotificationLayer mNotificationLayer;
    ShipTexturizer mShipTexturizer;
    std::unique_ptr<EventRecorder> mEventRecorder;


    //
    // The world
    //

    FishSpeciesDatabase mFishSpeciesDatabase;
    MaterialDatabase mMaterialDatabase;

    std::unique_ptr<Physics::World> mWorld;


    //
    // Parameter smoothing
    //

    static constexpr size_t SpringStiffnessAdjustmentParameterSmoother = 0;
    static constexpr size_t SpringStrengthAdjustmentParameterSmoother = 1;
    static constexpr size_t SeaDepthParameterSmoother = 2;
    static constexpr size_t OceanFloorBumpinessParameterSmoother = 3;
    static constexpr size_t OceanFloorDetailAmplificationParameterSmoother = 4;
    static constexpr size_t FlameSizeAdjustmentParameterSmoother = 5;
    static constexpr size_t BasalWaveHeightAdjustmentParameterSmoother = 6;
    static constexpr size_t FishSizeMultiplierParameterSmoother = 7;
    std::vector<ParameterSmoother<float>> mFloatParameterSmoothers;

    std::unique_ptr<ParameterSmoother<float>> mZoomParameterSmoother;
    std::unique_ptr<ParameterSmoother<vec2f>> mCameraWorldPositionParameterSmoother;


    //
    // Stats
    //

    std::chrono::steady_clock::time_point mStatsOriginTimestampReal;
    std::chrono::steady_clock::time_point mStatsLastTimestampReal;
    GameWallClock::time_point mOriginTimestampGame;
    std::unique_ptr<PerfStats> mTotalPerfStats;
    PerfStats mLastPublishedTotalPerfStats;
    uint64_t mTotalFrameCount;
    uint64_t mLastPublishedTotalFrameCount;
    int mSkippedFirstStatPublishes;
};
