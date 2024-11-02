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
#include "NpcDatabase.h"
#include "PerfStats.h"
#include "Physics.h"
#include "RenderContext.h"
#include "RenderDeviceProperties.h"
#include "ResourceLocator.h"
#include "ShipFactory.h"
#include "ShipLoadSpecifications.h"
#include "ShipMetadata.h"
#include "ShipTexturizer.h"
#include "ViewManager.h"

#include <GameCore/Colors.h>
#include <GameCore/GameChronometer.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ImageData.h>
#include <GameCore/ParameterSmoother.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/ThreadManager.h>
#include <GameCore/Vectors.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
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
    , public INpcGameEventHandler
    , public IWavePhenomenaGameEventHandler
{
public:

    static std::unique_ptr<GameController> Create(
        RenderDeviceProperties const & renderDeviceProperties,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

    ~GameController();

public:

    MaterialDatabase const & GetMaterialDatabase() const
    {
        return mMaterialDatabase;
    }

    ShipTexturizer const & GetShipTexturizer() const
    {
        return mShipTexturizer;
    }

    ShipStrengthRandomizer const & GetShipStrengthRandomizer() const
    {
        return mShipStrengthRandomizer;
    }

    auto GetHumanNpcSubKinds(std::string const & language) const
    {
        return mNpcDatabase.GetHumanSubKinds(language);
    }

    auto GetFurnitureNpcSubKinds(std::string const & language) const
    {
        return mNpcDatabase.GetFurnitureSubKinds(language);
    }

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

    void RegisterNpcEventHandler(INpcGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterNpcEventHandler(handler);
    }

    void RegisterGenericEventHandler(IGenericGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterGenericEventHandler(handler);
    }

    void RegisterControlEventHandler(IControlGameEventHandler * handler) override
    {
        assert(!!mGameEventDispatcher);
        mGameEventDispatcher->RegisterControlEventHandler(handler);
    }

    void RebindOpenGLContext();

    ShipMetadata ResetAndLoadShip(ShipLoadSpecifications const & loadSpecs) override;
    ShipMetadata ResetAndReloadShip(ShipLoadSpecifications const & loadSpecs) override;
    ShipMetadata AddShip(ShipLoadSpecifications const & loadSpecs) override;

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

    void Freeze() override;
    void Thaw() override;
    void SetPaused(bool isPaused) override;
    void SetMoveToolEngaged(bool isEngaged) override;
    void DisplaySettingsLoadedNotification() override;

    bool GetShowStatusText() const override;
    void SetShowStatusText(bool value) override;
    bool GetShowExtendedStatusText() const override;
    void SetShowExtendedStatusText(bool value) override;

    void DisplayEphemeralTextLine(std::string const & text) override;

    void NotifySoundMuted(bool isSoundMuted) override;

    // Not sticky
    void SetLineGuide(
        DisplayLogicalCoordinates const & start,
        DisplayLogicalCoordinates const & end) override;

    //
    // World probing
    //

    float GetCurrentSimulationTime() const override { return mWorld->GetCurrentSimulationTime(); }
    void ToggleToFullDayOrNight() override;
    float GetEffectiveAmbientLightIntensity() const override { return mRenderContext->GetEffectiveAmbientLightIntensity(); }
    bool IsUnderwater(DisplayLogicalCoordinates const & screenCoordinates) const override { return mWorld->GetOceanSurface().IsUnderwater(ScreenToWorld(screenCoordinates)); }
    bool IsUnderwater(GlobalElementId elementId) const override { return mWorld->IsUnderwater(elementId); }
    bool HasNpcs() const override { return mWorld->GetNpcs().HasNpcs(); }

    //
    // Interactions
    //

    void PickObjectToMove(DisplayLogicalCoordinates const & screenCoordinates, std::optional<GlobalConnectedComponentId> & connectedComponentId) override;
    void PickObjectToMove(DisplayLogicalCoordinates const & screenCoordinates, std::optional<ShipId> & shipId) override;
    void MoveBy(GlobalConnectedComponentId const & connectedComponentId, DisplayLogicalSize const & screenOffset, DisplayLogicalSize const & inertialScreenOffset) override;
    void MoveBy(ShipId shipId, DisplayLogicalSize const & screenOffset, DisplayLogicalSize const & inertialScreenOffset) override;
    void RotateBy(GlobalConnectedComponentId const & connectedComponentId, float screenDeltaY, DisplayLogicalCoordinates const & screenCenter, float inertialScreenDeltaY) override;
    void RotateBy(ShipId shipId, float screenDeltaY, DisplayLogicalCoordinates const & screenCenter, float intertialScreenDeltaY) override;
    std::optional<GlobalElementId> PickObjectForPickAndPull(DisplayLogicalCoordinates const & screenCoordinates) override;
    void Pull(GlobalElementId elementId, DisplayLogicalCoordinates const & screenTarget) override;
    void DestroyAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier) override;
    void RepairAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, SequenceNumber repairStepId) override;
    bool SawThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, bool isFirstSegment) override;
    bool ApplyHeatBlasterAt(DisplayLogicalCoordinates const & screenCoordinates, HeatBlasterActionType action) override;
    bool ExtinguishFireAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void ApplyBlastAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, float forceMultiplier, float renderProgress, float personalitySeed) override;
    bool ApplyElectricSparkAt(DisplayLogicalCoordinates const & screenCoordinates, std::uint64_t counter, float lengthMultiplier, float currentSimulationTime) override;
    void ApplyRadialWindFrom(DisplayLogicalCoordinates const & sourcePos, float preFrontSimulationTimeElapsed, float preFrontIntensityMultiplier, float mainFrontSimulationTimeElapsed, float mainFrontIntensityMultiplier) override;
    bool ApplyLaserCannonThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, std::optional<float> strength) override;
    void DrawTo(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) override;
    void SwirlAt(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) override;
    void TogglePinAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void RemoveAllPins() override;
    std::optional<ToolApplicationLocus> InjectPressureAt(DisplayLogicalCoordinates const & screenCoordinates, float pressureQuantityMultiplier) override;
    bool FloodAt(DisplayLogicalCoordinates const & screenCoordinates, float waterQuantityMultiplier) override;
    void ToggleAntiMatterBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void ToggleImpactBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void TogglePhysicsProbeAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void ToggleRCBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void ToggleTimerBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void DetonateRCBombs() override;
    void DetonateAntiMatterBombs() override;
    void AdjustOceanSurfaceTo(DisplayLogicalCoordinates const & screenCoordinates, int screenRadius) override;
    std::optional<bool> AdjustOceanFloorTo(vec2f const & startWorldPosition, vec2f const & endWorldPosition) override;
    bool ScrubThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates) override;
    bool RotThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates) override;
    void ApplyThanosSnapAt(DisplayLogicalCoordinates const & screenCoordinates, bool isSparseMode) override;
    void ScareFish(DisplayLogicalCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) override;
    void AttractFish(DisplayLogicalCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) override;
    void SetLampAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusScreenFraction) override;
    void ResetLamp() override;
    NpcKindType GetNpcKind(NpcId id) override;
    std::optional<PickedNpc> BeginPlaceNewFurnitureNpc(std::optional<NpcSubKindIdType> subKind, DisplayLogicalCoordinates const & screenCoordinates, bool doMoveWholeMesh) override;
    std::optional<PickedNpc> BeginPlaceNewHumanNpc(std::optional<NpcSubKindIdType> subKind, DisplayLogicalCoordinates const & screenCoordinates, bool doMoveWholeMesh) override;
    std::optional<PickedNpc> ProbeNpcAt(DisplayLogicalCoordinates const & screenCoordinates) const override;
    void BeginMoveNpc(NpcId id, int particleOrdinal, bool doMoveWholeMesh) override;
    void MoveNpcTo(NpcId id, DisplayLogicalCoordinates const & screenCoordinates, vec2f const & worldOffset, bool doMoveWholeMesh) override;
    void EndMoveNpc(NpcId id) override;
    void CompleteNewNpc(NpcId id) override;
    void RemoveNpc(NpcId id) override;
    void AbortNewNpc(NpcId id) override;
    void AddNpcGroup(NpcKindType kind) override;
    void TurnaroundNpc(NpcId id) override;
    void SelectNpc(std::optional<NpcId> id) override;
    void SelectNextNpc() override;
    void HighlightNpc(std::optional<NpcId> id) override;
    std::optional<GlobalElementId> GetNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const override;
    void QueryNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const override;

    void TriggerTsunami() override;
    void TriggerRogueWave() override;
    void TriggerStorm() override;
    void TriggerLightning() override;

    void HighlightElectricalElement(GlobalElectricalElementId electricalElementId) override;

    void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState) override;

    void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue) override;

    bool DestroyTriangle(GlobalElementId triangleId) override;
    bool RestoreTriangle(GlobalElementId triangleId) override;

    //
    // Render controls
    //

    void SetCanvasSize(DisplayLogicalSize const & canvasSize) override;
    void Pan(DisplayLogicalSize const & screenOffset) override;
    void PanToWorldEnd(int side) override;
    void AdjustZoom(float amount) override;
    void ResetView() override;
    void FocusOnShips() override;
    vec2f ScreenToWorld(DisplayLogicalCoordinates const & screenCoordinates) const override;
    vec2f ScreenOffsetToWorldOffset(DisplayLogicalSize const & screenOffset) const override;

    float GetCameraSpeedAdjustment() const override { return mViewManager.GetCameraSpeedAdjustment(); }
    void SetCameraSpeedAdjustment(float value) override { mViewManager.SetCameraSpeedAdjustment(value); }
    float GetMinCameraSpeedAdjustment() const override { return ViewManager::GetMinCameraSpeedAdjustment(); }
    float GetMaxCameraSpeedAdjustment() const override { return ViewManager::GetMaxCameraSpeedAdjustment(); }

    bool GetDoAutoFocusOnShipLoad() const override { return mViewManager.GetDoAutoFocusOnShipLoad(); }
    void SetDoAutoFocusOnShipLoad(bool value) override { mViewManager.SetDoAutoFocusOnShipLoad(value); }

    bool GetDoAutoFocusOnNpcPlacement() const override { return mViewManager.GetDoAutoFocusOnNpcPlacement(); }
    void SetDoAutoFocusOnNpcPlacement(bool value) override { mViewManager.SetDoAutoFocusOnNpcPlacement(value); }

    std::optional<AutoFocusTargetKindType> GetAutoFocusTarget() const override;
    void SetAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & autoFocusTarget) override;

    //
    // UI parameters
    //

    bool GetDoShowTsunamiNotifications() const override { return mDoShowTsunamiNotifications; }
    void SetDoShowTsunamiNotifications(bool value) override { mDoShowTsunamiNotifications = value; }

    bool GetDoShowElectricalNotifications() const override { return mGameParameters.DoShowElectricalNotifications; }
    void SetDoShowElectricalNotifications(bool value) override { mGameParameters.DoShowElectricalNotifications = value; }

    bool GetDoShowNpcNotifications() const override { return mDoShowNpcNotifications; }
    void SetDoShowNpcNotifications(bool value) override { mDoShowNpcNotifications = value; }

    UnitsSystem GetDisplayUnitsSystem() const override { return mRenderContext->GetDisplayUnitsSystem(); }
    void SetDisplayUnitsSystem(UnitsSystem value) override { mRenderContext->SetDisplayUnitsSystem(value); mNotificationLayer.SetDisplayUnitsSystem(value); }

    //
    // Ship factory parameters
    //

    ShipAutoTexturizationSettings const & GetShipAutoTexturizationSharedSettings() const override { return mShipTexturizer.GetSharedSettings(); }
    ShipAutoTexturizationSettings & GetShipAutoTexturizationSharedSettings() override { return mShipTexturizer.GetSharedSettings(); }
    void SetShipAutoTexturizationSharedSettings(ShipAutoTexturizationSettings const & value) override { mShipTexturizer.SetSharedSettings(value); }

    bool GetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings() const override { return mShipTexturizer.GetDoForceSharedSettingsOntoShipSettings(); }
    void SetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings(bool value) override { mShipTexturizer.SetDoForceSharedSettingsOntoShipSettings(value); }

    /////////////////////////////////////////////////////////
    // IGameControllerSettings and IGameControllerSettingsOptions
    /////////////////////////////////////////////////////////

    //
    // Game parameters
    //

    float GetSimulationStepTimeDuration() const override { return GameParameters::SimulationStepTimeDuration<float>; }

    unsigned int GetMaxNumSimulationThreads() const override { return static_cast<unsigned int>(mThreadManager.GetSimulationParallelism()); }
    void SetMaxNumSimulationThreads(unsigned int value) override { mThreadManager.SetSimulationParallelism(static_cast<size_t>(value)); }
    unsigned int GetMinMaxNumSimulationThreads() const override { return 1; }
    unsigned int GetMaxMaxNumSimulationThreads() const override { return static_cast<unsigned int>(mThreadManager.GetMaxSimulationParallelism()); }

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

    float GetElasticityAdjustment() const override { return mGameParameters.ElasticityAdjustment; }
    void SetElasticityAdjustment(float value) override { mGameParameters.ElasticityAdjustment = value; }
    float GetMinElasticityAdjustment() const override { return GameParameters::MinElasticityAdjustment; }
    float GetMaxElasticityAdjustment() const override { return GameParameters::MaxElasticityAdjustment; }

    float GetStaticFrictionAdjustment() const override { return mGameParameters.StaticFrictionAdjustment; }
    void SetStaticFrictionAdjustment(float value) override { mGameParameters.StaticFrictionAdjustment = value; }
    float GetMinStaticFrictionAdjustment() const override { return GameParameters::MinStaticFrictionAdjustment; }
    float GetMaxStaticFrictionAdjustment() const override { return GameParameters::MaxStaticFrictionAdjustment; }

    float GetKineticFrictionAdjustment() const override { return mGameParameters.KineticFrictionAdjustment; }
    void SetKineticFrictionAdjustment(float value) override { mGameParameters.KineticFrictionAdjustment = value; }
    float GetMinKineticFrictionAdjustment() const override { return GameParameters::MinKineticFrictionAdjustment; }
    float GetMaxKineticFrictionAdjustment() const override { return GameParameters::MaxKineticFrictionAdjustment; }

    float GetRotAcceler8r() const override { return mGameParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) override { mGameParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const override { return GameParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const override { return GameParameters::MaxRotAcceler8r; }

    float GetStaticPressureForceAdjustment() const override { return mGameParameters.StaticPressureForceAdjustment; }
    void SetStaticPressureForceAdjustment(float value) override { mGameParameters.StaticPressureForceAdjustment = value; }
    float GetMinStaticPressureForceAdjustment() const override { return GameParameters::MinStaticPressureForceAdjustment; }
    float GetMaxStaticPressureForceAdjustment() const override { return GameParameters::MaxStaticPressureForceAdjustment; }

    float GetTimeOfDay() const override { return mTimeOfDay; }
    void SetTimeOfDay(float value) override;

    // Air

    float GetAirDensityAdjustment() const override { return mGameParameters.AirDensityAdjustment; }
    void SetAirDensityAdjustment(float value) override { mGameParameters.AirDensityAdjustment = value; }
    float GetMinAirDensityAdjustment() const override { return GameParameters::MinAirDensityAdjustment; }
    float GetMaxAirDensityAdjustment() const override { return GameParameters::MaxAirDensityAdjustment; }

    float GetAirFrictionDragAdjustment() const override { return mGameParameters.AirFrictionDragAdjustment; }
    void SetAirFrictionDragAdjustment(float value) override { mGameParameters.AirFrictionDragAdjustment = value; }
    float GetMinAirFrictionDragAdjustment() const override { return GameParameters::MinAirFrictionDragAdjustment; }
    float GetMaxAirFrictionDragAdjustment() const override { return GameParameters::MaxAirFrictionDragAdjustment; }

    float GetAirPressureDragAdjustment() const override { return mGameParameters.AirPressureDragAdjustment; }
    void SetAirPressureDragAdjustment(float value) override { mGameParameters.AirPressureDragAdjustment = value; }
    float GetMinAirPressureDragAdjustment() const override { return GameParameters::MinAirPressureDragAdjustment; }
    float GetMaxAirPressureDragAdjustment() const override { return GameParameters::MaxAirPressureDragAdjustment; }

    // Water

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

    float GetWaterImpactForceAdjustment() const override { return mGameParameters.WaterImpactForceAdjustment; }
    void SetWaterImpactForceAdjustment(float value) override { mGameParameters.WaterImpactForceAdjustment = value; }
    float GetMinWaterImpactForceAdjustment() const override { return GameParameters::MinWaterImpactForceAdjustment; }
    float GetMaxWaterImpactForceAdjustment() const override { return GameParameters::MaxWaterImpactForceAdjustment; }

    float GetHydrostaticPressureCounterbalanceAdjustment() const override { return mGameParameters.HydrostaticPressureCounterbalanceAdjustment; }
    void SetHydrostaticPressureCounterbalanceAdjustment(float value) override { mGameParameters.HydrostaticPressureCounterbalanceAdjustment = value; }
    float GetMinHydrostaticPressureCounterbalanceAdjustment() const override { return GameParameters::MinHydrostaticPressureCounterbalanceAdjustment; }
    float GetMaxHydrostaticPressureCounterbalanceAdjustment() const override { return GameParameters::MaxHydrostaticPressureCounterbalanceAdjustment; }

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

    float GetLightningBlastProbability() const override { return mGameParameters.LightningBlastProbability; }
    void SetLightningBlastProbability(float value) override { mGameParameters.LightningBlastProbability = value; }

    // Heat

    float GetAirTemperature() const override { return mGameParameters.AirTemperature; }
    void SetAirTemperature(float value) override { mGameParameters.AirTemperature = value; }
    float GetMinAirTemperature() const override { return GameParameters::MinAirTemperature; }
    float GetMaxAirTemperature() const override { return GameParameters::MaxAirTemperature; }

    float GetWaterTemperature() const override { return mGameParameters.WaterTemperature; }
    void SetWaterTemperature(float value) override { mGameParameters.WaterTemperature = value; }
    float GetMinWaterTemperature() const override { return GameParameters::MinWaterTemperature; }
    float GetMaxWaterTemperature() const override { return GameParameters::MaxWaterTemperature; }

    unsigned int GetMaxBurningParticlesPerShip() const override { return mGameParameters.MaxBurningParticlesPerShip; }
    void SetMaxBurningParticlesPerShip(unsigned int value) override { mGameParameters.MaxBurningParticlesPerShip = value; }
    unsigned int GetMinMaxBurningParticlesPerShip() const override { return GameParameters::MinMaxBurningParticlesPerShip; }
    unsigned int GetMaxMaxBurningParticlesPerShip() const override { return GameParameters::MaxMaxBurningParticlesPerShip; }

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

    float GetLaserRayHeatFlow() const override { return mGameParameters.LaserRayHeatFlow; }
    void SetLaserRayHeatFlow(float value) override { mGameParameters.LaserRayHeatFlow = value; }
    float GetMinLaserRayHeatFlow() const override { return GameParameters::MinLaserRayHeatFlow; }
    float GetMaxLaserRayHeatFlow() const override { return GameParameters::MaxLaserRayHeatFlow; }

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

    // NPCs

    float GetNpcSpringReductionFractionAdjustment() const override { return mGameParameters.NpcSpringReductionFractionAdjustment; }
    void SetNpcSpringReductionFractionAdjustment(float value) override { mGameParameters.NpcSpringReductionFractionAdjustment = value; }
    float GetMinNpcSpringReductionFractionAdjustment() const override { return GameParameters::MinNpcSpringReductionFractionAdjustment; }
    float GetMaxNpcSpringReductionFractionAdjustment() const override { return GameParameters::MaxNpcSpringReductionFractionAdjustment; }

    float GetNpcSpringDampingCoefficientAdjustment() const override { return mGameParameters.NpcSpringDampingCoefficientAdjustment; }
    void SetNpcSpringDampingCoefficientAdjustment(float value) override { mGameParameters.NpcSpringDampingCoefficientAdjustment = value; }
    float GetMinNpcSpringDampingCoefficientAdjustment() const override { return GameParameters::MinNpcSpringDampingCoefficientAdjustment; }
    float GetMaxNpcSpringDampingCoefficientAdjustment() const override { return GameParameters::MaxNpcSpringDampingCoefficientAdjustment; }

    float GetNpcFrictionAdjustment() const override { return mGameParameters.NpcFrictionAdjustment; }
    void SetNpcFrictionAdjustment(float value) override { mGameParameters.NpcFrictionAdjustment = value; }
    float GetMinNpcFrictionAdjustment() const override { return GameParameters::MinNpcFrictionAdjustment; }
    float GetMaxNpcFrictionAdjustment() const override { return GameParameters::MaxNpcFrictionAdjustment; }

    float GetMinNpcWindReceptivityAdjustment() const override { return GameParameters::MinNpcWindReceptivityAdjustment; }
    float GetMaxNpcWindReceptivityAdjustment() const override { return GameParameters::MaxNpcWindReceptivityAdjustment; }

    float GetNpcSizeMultiplier() const override { return mFloatParameterSmoothers[NpcSizeMultiplierParameterSmoother].GetValue(); }
    void SetNpcSizeMultiplier(float value) override { mFloatParameterSmoothers[NpcSizeMultiplierParameterSmoother].SetValue(value); }
    float GetMinNpcSizeMultiplier() const override { return GameParameters::MinNpcSizeMultiplier; }
    float GetMaxNpcSizeMultiplier() const override { return GameParameters::MaxNpcSizeMultiplier; }

    bool GetDoApplyPhysicsToolsToNpcs() const override { return mGameParameters.DoApplyPhysicsToolsToNpcs; }
    void SetDoApplyPhysicsToolsToNpcs(bool value) override { mGameParameters.DoApplyPhysicsToolsToNpcs = value; }

    size_t GetMaxNpcs() const override { return mGameParameters.MaxNpcs; }
    void SetMaxNpcs(size_t value) { mGameParameters.MaxNpcs = value; }
    size_t GetMinMaxNpcs() const { return GameParameters::MinMaxNpcs; }
    size_t GetMaxMaxNpcs() const { return GameParameters::MaxMaxNpcs; }

    size_t GetNpcsPerGroup() const override { return mGameParameters.NpcsPerGroup; }
    void SetNpcsPerGroup(size_t value) { mGameParameters.NpcsPerGroup = value; }
    size_t GetMinNpcsPerGroup() const { return GameParameters::MinNpcsPerGroup; }
    size_t GetMaxNpcsPerGroup() const { return GameParameters::MaxNpcsPerGroup; }

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

    float GetOceanFloorElasticityCoefficient() const override { return mGameParameters.OceanFloorElasticityCoefficient; }
    void SetOceanFloorElasticityCoefficient(float value) override { mGameParameters.OceanFloorElasticityCoefficient = value; }
    float GetMinOceanFloorElasticityCoefficient() const override { return GameParameters::MinOceanFloorElasticityCoefficient; }
    float GetMaxOceanFloorElasticityCoefficient() const override { return GameParameters::MaxOceanFloorElasticityCoefficient; }

    float GetOceanFloorFrictionCoefficient() const override { return mGameParameters.OceanFloorFrictionCoefficient; }
    void SetOceanFloorFrictionCoefficient(float value) override { mGameParameters.OceanFloorFrictionCoefficient = value; }
    float GetMinOceanFloorFrictionCoefficient() const override { return GameParameters::MinOceanFloorFrictionCoefficient; }
    float GetMaxOceanFloorFrictionCoefficient() const override { return GameParameters::MaxOceanFloorFrictionCoefficient; }

    float GetOceanFloorSiltHardness() const override { return mGameParameters.OceanFloorSiltHardness; }
    void SetOceanFloorSiltHardness(float value) override { mGameParameters.OceanFloorSiltHardness = value; }
    float GetMinOceanFloorSiltHardness() const override { return GameParameters::MinOceanFloorSiltHardness; }
    float GetMaxOceanFloorSiltHardness() const override { return GameParameters::MaxOceanFloorSiltHardness; }

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

    float GetInjectPressureQuantity() const override { return mGameParameters.InjectPressureQuantity; }
    void SetInjectPressureQuantity(float value) override { mGameParameters.InjectPressureQuantity = value; }
    float GetMinInjectPressureQuantity() const override { return GameParameters::MinInjectPressureQuantity; }
    float GetMaxInjectPressureQuantity() const override { return GameParameters::MaxInjectPressureQuantity; }

    float GetBlastToolRadius() const override { return mGameParameters.BlastToolRadius; }
    void SetBlastToolRadius(float value) override { mGameParameters.BlastToolRadius = value; }
    float GetMinBlastToolRadius() const override { return GameParameters::MinBlastToolRadius; }
    float GetMaxBlastToolRadius() const override { return GameParameters::MaxBlastToolRadius; }

    float GetBlastToolForceAdjustment() const override { return mGameParameters.BlastToolForceAdjustment; }
    void SetBlastToolForceAdjustment(float value) override { mGameParameters.BlastToolForceAdjustment = value; }
    float GetMinBlastToolForceAdjustment() const override { return GameParameters::MinBlastToolForceAdjustment; }
    float GetMaxBlastToolForceAdjustment() const override { return GameParameters::MaxBlastToolForceAdjustment; }

    float GetScrubRotToolRadius() const override { return mGameParameters.ScrubRotToolRadius; }
    void SetScrubRotToolRadius(float value) override { mGameParameters.ScrubRotToolRadius = value; }
    float GetMinScrubRotToolRadius() const override { return GameParameters::MinScrubRotToolRadius; }
    float GetMaxScrubRotToolRadius() const override { return GameParameters::MaxScrubRotToolRadius; }

    float GetWindMakerToolWindSpeed() const override { return mGameParameters.WindMakerToolWindSpeed; }
    void SetWindMakerToolWindSpeed(float value) override { mGameParameters.WindMakerToolWindSpeed = value; }
    float GetMinWindMakerToolWindSpeed() const override { return GameParameters::MinWindMakerToolWindSpeed; }
    float GetMaxWindMakerToolWindSpeed() const override { return GameParameters::MaxWindMakerToolWindSpeed; }

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

    float GetShipStrengthRandomizationDensityAdjustment() const override { return mShipStrengthRandomizer.GetDensityAdjustment(); }
    void SetShipStrengthRandomizationDensityAdjustment(float value) override { mShipStrengthRandomizer.SetDensityAdjustment(value); }
    float GetMinShipStrengthRandomizationDensityAdjustment() const override { return 0.0f; }
    float GetMaxShipStrengthRandomizationDensityAdjustment() const override { return 10.0f; }

    float GetShipStrengthRandomizationExtent() const override { return mShipStrengthRandomizer.GetRandomizationExtent(); }
    void SetShipStrengthRandomizationExtent(float value) override { mShipStrengthRandomizer.SetRandomizationExtent(value); }
    float GetMinShipStrengthRandomizationExtent() const override { return 0.0f; }
    float GetMaxShipStrengthRandomizationExtent() const override { return 1.0f; }

    //
    // Render parameters
    //

    rgbColor const & GetFlatSkyColor() const override { return mRenderContext->GetFlatSkyColor(); }
    void SetFlatSkyColor(rgbColor const & color) override { mRenderContext->SetFlatSkyColor(color); }

    bool GetDoMoonlight() const override { return mRenderContext->GetDoMoonlight(); }
    void SetDoMoonlight(bool value) override { mRenderContext->SetDoMoonlight(value); }

    rgbColor const & GetMoonlightColor() const override { return mRenderContext->GetMoonlightColor(); }
    void SetMoonlightColor(rgbColor const & color) override { mRenderContext->SetMoonlightColor(color); }

    bool GetDoCrepuscularGradient() const override { return mRenderContext->GetDoCrepuscularGradient(); }
    void SetDoCrepuscularGradient(bool value) override { mRenderContext->SetDoCrepuscularGradient(value); }

    rgbColor const & GetCrepuscularColor() const override { return mRenderContext->GetCrepuscularColor(); }
    void SetCrepuscularColor(rgbColor const & color) override { mRenderContext->SetCrepuscularColor(color); }

    CloudRenderDetailType GetCloudRenderDetail() const override { return mRenderContext->GetCloudRenderDetail(); }
    void SetCloudRenderDetail(CloudRenderDetailType value) override { mRenderContext->SetCloudRenderDetail(value); }

    float GetOceanTransparency() const override { return mRenderContext->GetOceanTransparency(); }
    void SetOceanTransparency(float value) override { mRenderContext->SetOceanTransparency(value); }

    float GetOceanDepthDarkeningRate() const override { return mRenderContext->GetOceanDepthDarkeningRate(); }
    void SetOceanDepthDarkeningRate(float value) override { mRenderContext->SetOceanDepthDarkeningRate(value); }

    float GetShipAmbientLightSensitivity() const override { return mRenderContext->GetShipAmbientLightSensitivity(); }
    void SetShipAmbientLightSensitivity(float value) override { mRenderContext->SetShipAmbientLightSensitivity(value); }

    float GetShipDepthDarkeningSensitivity() const override { return mRenderContext->GetShipDepthDarkeningSensitivity(); }
    void SetShipDepthDarkeningSensitivity(float value) override { mRenderContext->SetShipDepthDarkeningSensitivity(value); }

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
    void SetOceanRenderDetail(OceanRenderDetailType value) override;

    LandRenderModeType GetLandRenderMode() const override { return mRenderContext->GetLandRenderMode(); }
    void SetLandRenderMode(LandRenderModeType landRenderMode) override { mRenderContext->SetLandRenderMode(landRenderMode); }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const override { return mRenderContext->GetTextureLandAvailableThumbnails(); }
    size_t GetTextureLandTextureIndex() const override { return mRenderContext->GetTextureLandTextureIndex(); }
    void SetTextureLandTextureIndex(size_t index) override { mRenderContext->SetTextureLandTextureIndex(index); }

    rgbColor const & GetFlatLandColor() const override { return mRenderContext->GetFlatLandColor(); }
    void SetFlatLandColor(rgbColor const & color) override { mRenderContext->SetFlatLandColor(color); }

    LandRenderDetailType GetLandRenderDetail() const override { return mRenderContext->GetLandRenderDetail(); }
    void SetLandRenderDetail(LandRenderDetailType value) override { mRenderContext->SetLandRenderDetail(value); }

    ShipViewModeType GetShipViewMode() const override { return mRenderContext->GetShipViewMode(); }
    void SetShipViewMode(ShipViewModeType shipViewMode) override { mRenderContext->SetShipViewMode(shipViewMode); }

    DebugShipRenderModeType GetDebugShipRenderMode() const override { return mRenderContext->GetDebugShipRenderMode(); }
    void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode) override { mRenderContext->SetDebugShipRenderMode(debugShipRenderMode); }

    NpcRenderModeType GetNpcRenderMode() const override { return mRenderContext->GetNpcRenderMode(); }
    void SetNpcRenderMode(NpcRenderModeType npcRenderMode) override { mRenderContext->SetNpcRenderMode(npcRenderMode); }

    rgbColor const & GetNpcQuadFlatColor() const override { return mRenderContext->GetNpcQuadFlatColor(); }
    void SetNpcQuadFlatColor(rgbColor const & color) override { mRenderContext->SetNpcQuadFlatColor(color); }

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

    StressRenderModeType GetStressRenderMode() const override { return mRenderContext->GetStressRenderMode(); }
    void SetStressRenderMode(StressRenderModeType value) override { mRenderContext->SetStressRenderMode(value); }

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

    void OnTsunami(float x) override;

    void OnShipRepaired(ShipId shipId) override;

    void OnHumanNpcCountsUpdated(
        size_t insideShipCount,
        size_t outsideShipCount) override;

private:

    GameController(
        std::unique_ptr<Render::RenderContext> renderContext,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        std::unique_ptr<PerfStats> perfStats,
        FishSpeciesDatabase && fishSpeciesDatabase,
        NpcDatabase && npcDatabase,
        MaterialDatabase && materialDatabase,
        ResourceLocator const & resourceLocator,
        ProgressCallback const & progressCallback);

    ShipMetadata InternalResetAndLoadShip(ShipLoadSpecifications const & loadSpecs);

    void Reset(std::unique_ptr<Physics::World> newWorld);

    void InternalAddShip(
        std::unique_ptr<Physics::Ship> ship,
        RgbaImageData && exteriorTextureImage,
        RgbaImageData && interiorViewImage,
        ShipMetadata const & shipMetadata);

    void ResetStats();

    void PublishStats(std::chrono::steady_clock::time_point nowReal);

    void OnBeginPlaceNewNpc(
        NpcId const & npcId,
        bool doAnchorToScreen);

    void NotifyNpcPlacementError(NpcCreationFailureReasonType reason);

    static bool CalculateAreCloudShadowsEnabled(OceanRenderDetailType oceanRenderDetail);

    // Auto-focus

    void UpdateViewOnShipLoad();

    void UpdateAutoFocus();

    void InternalSwitchAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & autoFocusTarget);

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
    void StartThanosSnapStateMachine(float x, bool isSparseMode, float currentSimulationTime);
    bool UpdateThanosSnapStateMachine(ThanosSnapStateMachine & stateMachine, float currentSimulationTime);

    struct DayLightCycleStateMachine;
    struct DayLightCycleStateMachineDeleter { void operator()(DayLightCycleStateMachine *) const; };
    std::unique_ptr<DayLightCycleStateMachine, DayLightCycleStateMachineDeleter> mDayLightCycleStateMachine;
    void StartDayLightCycleStateMachine();
    void StopDayLightCycleStateMachine();

    void ResetAllStateMachines();
    void UpdateAllStateMachines(float currentSimulationTime);

private:

    //
    // The world
    //

    std::unique_ptr<Physics::World> mWorld;

    FishSpeciesDatabase mFishSpeciesDatabase;
    NpcDatabase mNpcDatabase;
    MaterialDatabase mMaterialDatabase;

    //
    // Ship factory
    //

    ShipStrengthRandomizer mShipStrengthRandomizer;
    ShipTexturizer mShipTexturizer;


    //
    // Our current state
    //

    GameParameters mGameParameters;
    bool mIsFrozen;
    bool mIsPaused;
    bool mIsPulseUpdateSet;
    bool mIsMoveToolEngaged;


    //
    // The parameters that we own
    //

    float mTimeOfDay;
    bool mDoShowTsunamiNotifications;
    bool mDoShowNpcNotifications;
    bool mDoDrawHeatBlasterFlame;


    //
    // The doers
    //

    std::shared_ptr<Render::RenderContext> mRenderContext;
    std::shared_ptr<GameEventDispatcher> mGameEventDispatcher;
    NotificationLayer mNotificationLayer;
    ThreadManager mThreadManager;
    ViewManager mViewManager;
    std::unique_ptr<EventRecorder> mEventRecorder;


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
    static constexpr size_t NpcSizeMultiplierParameterSmoother = 8;
    std::vector<ParameterSmoother<float>> mFloatParameterSmoothers;


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
