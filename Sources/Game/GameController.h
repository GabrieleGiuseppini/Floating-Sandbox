/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameAssetManager.h"
#include "GameEventDispatcher.h"
#include "IGameController.h"
#include "IGameControllerSettings.h"
#include "IGameControllerSettingsOptions.h"
#include "IGameEventHandlers.h"
#include "NotificationLayer.h"
#include "ShipLoadSpecifications.h"
#include "ViewManager.h"

#include <Simulation/FishSpeciesDatabase.h>
#include <Simulation/MaterialDatabase.h>
#include <Simulation/NpcDatabase.h>
#include <Simulation/Physics/Physics.h>
#include <Simulation/ShipMetadata.h>
#include <Simulation/ShipStrengthRandomizer.h>
#include <Simulation/ShipTexturizer.h>
#include <Simulation/SimulationEventDispatcher.h>
#include <Simulation/SimulationParameters.h>

#include <Render/RenderContext.h>
#include <Render/RenderDeviceProperties.h>

#include <Core/PerfStats.h>

#include <Core/Colors.h>
#include <Core/GameChronometer.h>
#include <Core/GameTypes.h>
#include <Core/GameWallClock.h>
#include <Core/ImageData.h>
#include <Core/ParameterSmoother.h>
#include <Core/ProgressCallback.h>
#include <Core/ThreadManager.h>
#include <Core/Vectors.h>

#include <algorithm>
#include <cassert>
#include <chrono>
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
    ///
    , public IGenericShipEventHandler
    , public IWavePhenomenaEventHandler
    , public INpcEventHandler
{
public:

    static std::unique_ptr<GameController> Create(
        RenderDeviceProperties const & renderDeviceProperties,
        ThreadManager & threadManager,
        GameAssetManager const & gameAssetManager,
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

    auto GetHumanNpcSubKinds(
        NpcHumanRoleType role,
        std::string const & language) const
    {
        return mNpcDatabase.GetHumanSubKinds(role, language);
    }

    auto GetFurnitureNpcSubKinds(
        NpcFurnitureRoleType role,
        std::string const & language) const
    {
        return mNpcDatabase.GetFurnitureSubKinds(role, language);
    }

    /////////////////////////////////////////////////////////
    // IGameController
    /////////////////////////////////////////////////////////

    void RegisterStructuralShipEventHandler(IStructuralShipEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterStructuralShipEventHandler(handler);
    }

    void RegisterGenericShipEventHandler(IGenericShipEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterGenericShipEventHandler(handler);
    }

    void RegisterWavePhenomenaEventHandler(IWavePhenomenaEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterWavePhenomenaEventHandler(handler);
    }

    void RegisterCombustionEventHandler(ICombustionEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterCombustionEventHandler(handler);
    }

    void RegisterSimulationStatisticsEventHandler(ISimulationStatisticsEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterSimulationStatisticsEventHandler(handler);
    }

    void RegisterAtmosphereEventHandler(IAtmosphereEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterAtmosphereEventHandler(handler);
    }

    void RegisterElectricalElementEventHandler(IElectricalElementEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterElectricalElementEventHandler(handler);
    }

    void RegisterNpcEventHandler(INpcEventHandler * handler) override
    {
        mSimulationEventDispatcher.RegisterNpcEventHandler(handler);
    }

    void RegisterGameEventHandler(IGameEventHandler * handler) override
    {
        mGameEventDispatcher.RegisterGameEventHandler(handler);
    }

    void RegisterGameStatisticsEventHandler(IGameStatisticsEventHandler * handler) override
    {
        mGameEventDispatcher.RegisterGameStatisticsEventHandler(handler);
    }

    void RebindOpenGLContext();

    ShipMetadata ResetAndLoadShip(ShipLoadSpecifications const & loadSpecs, IAssetManager const & assetManager) override;
    ShipMetadata ResetAndReloadShip(ShipLoadSpecifications const & loadSpecs, IAssetManager const & assetManager) override;
    ShipMetadata AddShip(ShipLoadSpecifications const & loadSpecs, IAssetManager const & assetManager) override;

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

    bool IsShiftOn() const override;
    void SetShiftOn(bool value) override;

    // Not sticky

    void ShowInteractiveToolDashedLine(
        DisplayLogicalCoordinates const & start,
        DisplayLogicalCoordinates const & end) override;

    void ShowInteractiveToolDashedRect(
        DisplayLogicalCoordinates const & corner1,
        DisplayLogicalCoordinates const & corner2) override;

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
    std::tuple<vec2f, float> SetupMoveGrippedBy(DisplayLogicalCoordinates const & screenGripCenter, DisplayLogicalSize const & screenGripOffset) override;
    void MoveGrippedBy(vec2f const & worldGripCenter, float worldGripRadius, DisplayLogicalSize const & screenOffset, DisplayLogicalSize const & inertialScreenOffset) override;
    void RotateGrippedBy(vec2f const & worldGripCenter, float worldGripRadius, float screenDeltaY, float inertialScreenDeltaY) override;
    void EndMoveGrippedBy() override;
    std::optional<GlobalElementId> PickObjectForPickAndPull(DisplayLogicalCoordinates const & screenCoordinates) override;
    void Pull(GlobalElementId elementId, DisplayLogicalCoordinates const & screenTarget) override;
    void DestroyAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, SessionId const & sessionId) override;
    void RepairAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, SequenceNumber repairStepId) override;
    bool SawThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, bool isFirstSegment) override;
    bool ApplyHeatBlasterAt(DisplayLogicalCoordinates const & screenCoordinates, HeatBlasterActionType action) override;
    bool ExtinguishFireAt(DisplayLogicalCoordinates const & screenCoordinates, float strengthMultiplier) override;
    void ApplyBlastAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, float forceMultiplier, float renderProgress, float personalitySeed) override;
    bool ApplyElectricSparkAt(DisplayLogicalCoordinates const & screenCoordinates, std::uint64_t counter, float lengthMultiplier) override;
    void ApplyRadialWindFrom(DisplayLogicalCoordinates const & sourcePos, float preFrontSimulationTimeElapsed, float preFrontIntensityMultiplier, float mainFrontSimulationTimeElapsed, float mainFrontIntensityMultiplier) override;
    bool ApplyLaserCannonThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, std::optional<float> strength) override;
    void DrawTo(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) override;
    void SwirlAt(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) override;
    void TogglePinAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void RemoveAllPins() override;
    std::optional<ToolApplicationLocus> InjectPressureAt(DisplayLogicalCoordinates const & screenCoordinates, float pressureQuantityMultiplier) override;
    bool FloodAt(DisplayLogicalCoordinates const & screenCoordinates, float flowSign) override;
    void ToggleAntiMatterBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
    void ToggleFireExtinguishingBombAt(DisplayLogicalCoordinates const & screenCoordinates) override;
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
    std::vector<NpcId> ProbeNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) const override;
    void BeginMoveNpc(NpcId id, int particleOrdinal, bool doMoveWholeMesh) override;
    void BeginMoveNpcs(std::vector<NpcId> const & ids) override;
    void MoveNpcTo(NpcId id, DisplayLogicalCoordinates const & screenCoordinates, vec2f const & worldOffset, bool doMoveWholeMesh) override;
    void MoveNpcsBy(std::vector<NpcId> const & ids, DisplayLogicalSize const & screenOffset) override;
    void EndMoveNpc(NpcId id) override;
    void CompleteNewNpc(NpcId id) override;
    void RemoveNpc(NpcId id) override;
    void RemoveNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) override;
    void AbortNewNpc(NpcId id) override;
    void AddNpcGroup(NpcKindType kind) override;
    void TurnaroundNpc(NpcId id) override;
    void TurnaroundNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) override;
    std::optional<NpcId> GetCurrentlySelectedNpc() const override;
    void SelectNpc(std::optional<NpcId> id) override;
    void SelectNextNpc() override;
    void HighlightNpcs(std::vector<NpcId> const & ids) override;
    void HighlightNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) override;
    std::optional<GlobalElementId> GetNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const override;
    void QueryNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const override;
    void QueryNearestNpcAt(DisplayLogicalCoordinates const & screenCoordinates) const override;

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
    // NPCs
    //

    size_t GetMaxNpcs() const override { return mSimulationParameters.MaxNpcs; }
    void SetMaxNpcs(size_t value) override { mSimulationParameters.MaxNpcs = value; }
    size_t GetMinMaxNpcs() const override { return SimulationParameters::MinMaxNpcs; }
    size_t GetMaxMaxNpcs() const override { return SimulationParameters::MaxMaxNpcs; }

    size_t GetNpcsPerGroup() const override { return mSimulationParameters.NpcsPerGroup; }
    void SetNpcsPerGroup(size_t value) override { mSimulationParameters.NpcsPerGroup = value; }
    size_t GetMinNpcsPerGroup() const override { return SimulationParameters::MinNpcsPerGroup; }
    size_t GetMaxNpcsPerGroup() const override { return SimulationParameters::MaxNpcsPerGroup; }

    //
    // UI parameters
    //

    bool GetDoShowTsunamiNotifications() const override { return mDoShowTsunamiNotifications; }
    void SetDoShowTsunamiNotifications(bool value) override { mDoShowTsunamiNotifications = value; }

    bool GetDoShowElectricalNotifications() const override { return mSimulationParameters.DoShowElectricalNotifications; }
    void SetDoShowElectricalNotifications(bool value) override { mSimulationParameters.DoShowElectricalNotifications = value; }

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
    // Simulation
    //

    float GetSimulationStepTimeDuration() const override { return SimulationParameters::SimulationStepTimeDuration<float>; }

    unsigned int GetMaxNumSimulationThreads() const override { return static_cast<unsigned int>(mThreadManager.GetSimulationParallelism()); }
    void SetMaxNumSimulationThreads(unsigned int value) override { mThreadManager.SetSimulationParallelism(static_cast<size_t>(value)); }
    unsigned int GetMinMaxNumSimulationThreads() const override { return 1; }
    unsigned int GetMaxMaxNumSimulationThreads() const override { return static_cast<unsigned int>(mThreadManager.GetMaxSimulationParallelism()); }

    float GetNumMechanicalDynamicsIterationsAdjustment() const override { return mSimulationParameters.NumMechanicalDynamicsIterationsAdjustment; }
    void SetNumMechanicalDynamicsIterationsAdjustment(float value) override { mSimulationParameters.NumMechanicalDynamicsIterationsAdjustment = value; }
    float GetMinNumMechanicalDynamicsIterationsAdjustment() const override { return SimulationParameters::MinNumMechanicalDynamicsIterationsAdjustment; }
    float GetMaxNumMechanicalDynamicsIterationsAdjustment() const override { return SimulationParameters::MaxNumMechanicalDynamicsIterationsAdjustment; }

    float GetSpringStiffnessAdjustment() const override { return mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStiffnessAdjustment(float value) override { mFloatParameterSmoothers[SpringStiffnessAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStiffnessAdjustment() const override { return SimulationParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const override { return SimulationParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const override { return mSimulationParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value) override { mSimulationParameters.SpringDampingAdjustment = value; }
    float GetMinSpringDampingAdjustment() const override { return SimulationParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const override { return SimulationParameters::MaxSpringDampingAdjustment; }

    float GetSpringStrengthAdjustment() const override { return mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].GetValue(); }
    void SetSpringStrengthAdjustment(float value) override { mFloatParameterSmoothers[SpringStrengthAdjustmentParameterSmoother].SetValue(value); }
    float GetMinSpringStrengthAdjustment() const override { return SimulationParameters::MinSpringStrengthAdjustment; }
    float GetMaxSpringStrengthAdjustment() const override { return SimulationParameters::MaxSpringStrengthAdjustment; }

    float GetGlobalDampingAdjustment() const override { return mSimulationParameters.GlobalDampingAdjustment; }
    void SetGlobalDampingAdjustment(float value) override { mSimulationParameters.GlobalDampingAdjustment = value; }
    float GetMinGlobalDampingAdjustment() const override { return SimulationParameters::MinGlobalDampingAdjustment; }
    float GetMaxGlobalDampingAdjustment() const override { return SimulationParameters::MaxGlobalDampingAdjustment; }

    float GetElasticityAdjustment() const override { return mSimulationParameters.ElasticityAdjustment; }
    void SetElasticityAdjustment(float value) override { mSimulationParameters.ElasticityAdjustment = value; }
    float GetMinElasticityAdjustment() const override { return SimulationParameters::MinElasticityAdjustment; }
    float GetMaxElasticityAdjustment() const override { return SimulationParameters::MaxElasticityAdjustment; }

    float GetStaticFrictionAdjustment() const override { return mSimulationParameters.StaticFrictionAdjustment; }
    void SetStaticFrictionAdjustment(float value) override { mSimulationParameters.StaticFrictionAdjustment = value; }
    float GetMinStaticFrictionAdjustment() const override { return SimulationParameters::MinStaticFrictionAdjustment; }
    float GetMaxStaticFrictionAdjustment() const override { return SimulationParameters::MaxStaticFrictionAdjustment; }

    float GetKineticFrictionAdjustment() const override { return mSimulationParameters.KineticFrictionAdjustment; }
    void SetKineticFrictionAdjustment(float value) override { mSimulationParameters.KineticFrictionAdjustment = value; }
    float GetMinKineticFrictionAdjustment() const override { return SimulationParameters::MinKineticFrictionAdjustment; }
    float GetMaxKineticFrictionAdjustment() const override { return SimulationParameters::MaxKineticFrictionAdjustment; }

    float GetRotAcceler8r() const override { return mSimulationParameters.RotAcceler8r; }
    void SetRotAcceler8r(float value) override { mSimulationParameters.RotAcceler8r = value; }
    float GetMinRotAcceler8r() const override { return SimulationParameters::MinRotAcceler8r; }
    float GetMaxRotAcceler8r() const override { return SimulationParameters::MaxRotAcceler8r; }

    float GetStaticPressureForceAdjustment() const override { return mSimulationParameters.StaticPressureForceAdjustment; }
    void SetStaticPressureForceAdjustment(float value) override { mSimulationParameters.StaticPressureForceAdjustment = value; }
    float GetMinStaticPressureForceAdjustment() const override { return SimulationParameters::MinStaticPressureForceAdjustment; }
    float GetMaxStaticPressureForceAdjustment() const override { return SimulationParameters::MaxStaticPressureForceAdjustment; }

    float GetTimeOfDay() const override { return mTimeOfDay; }
    void SetTimeOfDay(float value) override;

    // Air

    float GetAirDensityAdjustment() const override { return mSimulationParameters.AirDensityAdjustment; }
    void SetAirDensityAdjustment(float value) override { mSimulationParameters.AirDensityAdjustment = value; }
    float GetMinAirDensityAdjustment() const override { return SimulationParameters::MinAirDensityAdjustment; }
    float GetMaxAirDensityAdjustment() const override { return SimulationParameters::MaxAirDensityAdjustment; }

    float GetAirFrictionDragAdjustment() const override { return mSimulationParameters.AirFrictionDragAdjustment; }
    void SetAirFrictionDragAdjustment(float value) override { mSimulationParameters.AirFrictionDragAdjustment = value; }
    float GetMinAirFrictionDragAdjustment() const override { return SimulationParameters::MinAirFrictionDragAdjustment; }
    float GetMaxAirFrictionDragAdjustment() const override { return SimulationParameters::MaxAirFrictionDragAdjustment; }

    float GetAirPressureDragAdjustment() const override { return mSimulationParameters.AirPressureDragAdjustment; }
    void SetAirPressureDragAdjustment(float value) override { mSimulationParameters.AirPressureDragAdjustment = value; }
    float GetMinAirPressureDragAdjustment() const override { return SimulationParameters::MinAirPressureDragAdjustment; }
    float GetMaxAirPressureDragAdjustment() const override { return SimulationParameters::MaxAirPressureDragAdjustment; }

    // Water

    float GetWaterDensityAdjustment() const override { return mSimulationParameters.WaterDensityAdjustment; }
    void SetWaterDensityAdjustment(float value) override { mSimulationParameters.WaterDensityAdjustment = value; }
    float GetMinWaterDensityAdjustment() const override { return SimulationParameters::MinWaterDensityAdjustment; }
    float GetMaxWaterDensityAdjustment() const override { return SimulationParameters::MaxWaterDensityAdjustment; }

    float GetWaterFrictionDragAdjustment() const override { return mSimulationParameters.WaterFrictionDragAdjustment; }
    void SetWaterFrictionDragAdjustment(float value) override { mSimulationParameters.WaterFrictionDragAdjustment = value; }
    float GetMinWaterFrictionDragAdjustment() const override { return SimulationParameters::MinWaterFrictionDragAdjustment; }
    float GetMaxWaterFrictionDragAdjustment() const override { return SimulationParameters::MaxWaterFrictionDragAdjustment; }

    float GetWaterPressureDragAdjustment() const override { return mSimulationParameters.WaterPressureDragAdjustment; }
    void SetWaterPressureDragAdjustment(float value) override { mSimulationParameters.WaterPressureDragAdjustment = value; }
    float GetMinWaterPressureDragAdjustment() const override { return SimulationParameters::MinWaterPressureDragAdjustment; }
    float GetMaxWaterPressureDragAdjustment() const override { return SimulationParameters::MaxWaterPressureDragAdjustment; }

    float GetWaterImpactForceAdjustment() const override { return mSimulationParameters.WaterImpactForceAdjustment; }
    void SetWaterImpactForceAdjustment(float value) override { mSimulationParameters.WaterImpactForceAdjustment = value; }
    float GetMinWaterImpactForceAdjustment() const override { return SimulationParameters::MinWaterImpactForceAdjustment; }
    float GetMaxWaterImpactForceAdjustment() const override { return SimulationParameters::MaxWaterImpactForceAdjustment; }

    float GetHydrostaticPressureCounterbalanceAdjustment() const override { return mSimulationParameters.HydrostaticPressureCounterbalanceAdjustment; }
    void SetHydrostaticPressureCounterbalanceAdjustment(float value) override { mSimulationParameters.HydrostaticPressureCounterbalanceAdjustment = value; }
    float GetMinHydrostaticPressureCounterbalanceAdjustment() const override { return SimulationParameters::MinHydrostaticPressureCounterbalanceAdjustment; }
    float GetMaxHydrostaticPressureCounterbalanceAdjustment() const override { return SimulationParameters::MaxHydrostaticPressureCounterbalanceAdjustment; }

    float GetWaterIntakeAdjustment() const override { return mSimulationParameters.WaterIntakeAdjustment; }
    void SetWaterIntakeAdjustment(float value) override { mSimulationParameters.WaterIntakeAdjustment = value; }
    float GetMinWaterIntakeAdjustment() const override { return SimulationParameters::MinWaterIntakeAdjustment; }
    float GetMaxWaterIntakeAdjustment() const override { return SimulationParameters::MaxWaterIntakeAdjustment; }

    float GetWaterDiffusionSpeedAdjustment() const override { return mSimulationParameters.WaterDiffusionSpeedAdjustment; }
    void SetWaterDiffusionSpeedAdjustment(float value) override { mSimulationParameters.WaterDiffusionSpeedAdjustment = value; }
    float GetMinWaterDiffusionSpeedAdjustment() const override { return SimulationParameters::MinWaterDiffusionSpeedAdjustment; }
    float GetMaxWaterDiffusionSpeedAdjustment() const override { return SimulationParameters::MaxWaterDiffusionSpeedAdjustment; }

    float GetWaterCrazyness() const override { return mSimulationParameters.WaterCrazyness; }
    void SetWaterCrazyness(float value) override { mSimulationParameters.WaterCrazyness = value; }
    float GetMinWaterCrazyness() const override { return SimulationParameters::MinWaterCrazyness; }
    float GetMaxWaterCrazyness() const override { return SimulationParameters::MaxWaterCrazyness; }

    float GetSmokeEmissionDensityAdjustment() const override { return mSimulationParameters.SmokeEmissionDensityAdjustment; }
    void SetSmokeEmissionDensityAdjustment(float value) override { mSimulationParameters.SmokeEmissionDensityAdjustment = value; }
    float GetMinSmokeEmissionDensityAdjustment() const override { return SimulationParameters::MinSmokeEmissionDensityAdjustment; }
    float GetMaxSmokeEmissionDensityAdjustment() const override { return SimulationParameters::MaxSmokeEmissionDensityAdjustment; }

    float GetSmokeParticleLifetimeAdjustment() const override { return mSimulationParameters.SmokeParticleLifetimeAdjustment; }
    void SetSmokeParticleLifetimeAdjustment(float value) override { mSimulationParameters.SmokeParticleLifetimeAdjustment = value; }
    float GetMinSmokeParticleLifetimeAdjustment() const override { return SimulationParameters::MinSmokeParticleLifetimeAdjustment; }
    float GetMaxSmokeParticleLifetimeAdjustment() const override { return SimulationParameters::MaxSmokeParticleLifetimeAdjustment; }

    bool GetDoModulateWind() const override { return mSimulationParameters.DoModulateWind; }
    void SetDoModulateWind(bool value) override { mSimulationParameters.DoModulateWind = value; }

    float GetWindSpeedBase() const override { return mSimulationParameters.WindSpeedBase; }
    void SetWindSpeedBase(float value) override { mSimulationParameters.WindSpeedBase = value; }
    float GetMinWindSpeedBase() const override { return SimulationParameters::MinWindSpeedBase; }
    float GetMaxWindSpeedBase() const override { return SimulationParameters::MaxWindSpeedBase; }

    float GetWindSpeedMaxFactor() const override { return mSimulationParameters.WindSpeedMaxFactor; }
    void SetWindSpeedMaxFactor(float value) override { mSimulationParameters.WindSpeedMaxFactor = value; }
    float GetMinWindSpeedMaxFactor() const override { return SimulationParameters::MinWindSpeedMaxFactor; }
    float GetMaxWindSpeedMaxFactor() const override { return SimulationParameters::MaxWindSpeedMaxFactor; }

    float GetBasalWaveHeightAdjustment() const override { return mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].GetValue(); }
    void SetBasalWaveHeightAdjustment(float value) override { mFloatParameterSmoothers[BasalWaveHeightAdjustmentParameterSmoother].SetValue(value); }
    float GetMinBasalWaveHeightAdjustment() const override { return SimulationParameters::MinBasalWaveHeightAdjustment; }
    float GetMaxBasalWaveHeightAdjustment() const override { return SimulationParameters::MaxBasalWaveHeightAdjustment; }

    float GetBasalWaveLengthAdjustment() const override { return mSimulationParameters.BasalWaveLengthAdjustment; }
    void SetBasalWaveLengthAdjustment(float value) override { mSimulationParameters.BasalWaveLengthAdjustment = value; }
    float GetMinBasalWaveLengthAdjustment() const override { return SimulationParameters::MinBasalWaveLengthAdjustment; }
    float GetMaxBasalWaveLengthAdjustment() const override { return SimulationParameters::MaxBasalWaveLengthAdjustment; }

    float GetBasalWaveSpeedAdjustment() const override { return mSimulationParameters.BasalWaveSpeedAdjustment; }
    void SetBasalWaveSpeedAdjustment(float value) override { mSimulationParameters.BasalWaveSpeedAdjustment = value; }
    float GetMinBasalWaveSpeedAdjustment() const override { return SimulationParameters::MinBasalWaveSpeedAdjustment; }
    float GetMaxBasalWaveSpeedAdjustment() const override { return SimulationParameters::MaxBasalWaveSpeedAdjustment; }

    std::chrono::minutes GetTsunamiRate() const override { return mSimulationParameters.TsunamiRate; }
    void SetTsunamiRate(std::chrono::minutes value) override { mSimulationParameters.TsunamiRate = value; }
    std::chrono::minutes GetMinTsunamiRate() const override { return SimulationParameters::MinTsunamiRate; }
    std::chrono::minutes GetMaxTsunamiRate() const override { return SimulationParameters::MaxTsunamiRate; }

    std::chrono::seconds GetRogueWaveRate() const override { return mSimulationParameters.RogueWaveRate; }
    void SetRogueWaveRate(std::chrono::seconds value) override { mSimulationParameters.RogueWaveRate = value; }
    std::chrono::seconds GetMinRogueWaveRate() const override { return SimulationParameters::MinRogueWaveRate; }
    std::chrono::seconds GetMaxRogueWaveRate() const override { return SimulationParameters::MaxRogueWaveRate; }

    bool GetDoDisplaceWater() const override { return mSimulationParameters.DoDisplaceWater; }
    void SetDoDisplaceWater(bool value) override { mSimulationParameters.DoDisplaceWater = value; }

    float GetWaterDisplacementWaveHeightAdjustment() const override { return mSimulationParameters.WaterDisplacementWaveHeightAdjustment; }
    void SetWaterDisplacementWaveHeightAdjustment(float value) override { mSimulationParameters.WaterDisplacementWaveHeightAdjustment = value; }
    float GetMinWaterDisplacementWaveHeightAdjustment() const override { return SimulationParameters::MinWaterDisplacementWaveHeightAdjustment; }
    float GetMaxWaterDisplacementWaveHeightAdjustment() const override { return SimulationParameters::MaxWaterDisplacementWaveHeightAdjustment; }

    float GetWaveSmoothnessAdjustment() const override { return mSimulationParameters.WaveSmoothnessAdjustment; }
    void SetWaveSmoothnessAdjustment(float value) override { mSimulationParameters.WaveSmoothnessAdjustment = value; }
    float GetMinWaveSmoothnessAdjustment() const override { return SimulationParameters::MinWaveSmoothnessAdjustment; }
    float GetMaxWaveSmoothnessAdjustment() const override { return SimulationParameters::MaxWaveSmoothnessAdjustment; }

    // Storm

    std::chrono::minutes GetStormRate() const override { return mSimulationParameters.StormRate; }
    void SetStormRate(std::chrono::minutes value) override { mSimulationParameters.StormRate = value; }
    std::chrono::minutes GetMinStormRate() const override { return SimulationParameters::MinStormRate; }
    std::chrono::minutes GetMaxStormRate() const override { return SimulationParameters::MaxStormRate; }

    std::chrono::seconds GetStormDuration() const override { return mSimulationParameters.StormDuration; }
    void SetStormDuration(std::chrono::seconds value) override { mSimulationParameters.StormDuration = value; }
    std::chrono::seconds GetMinStormDuration() const override { return SimulationParameters::MinStormDuration; }
    std::chrono::seconds GetMaxStormDuration() const override { return SimulationParameters::MaxStormDuration; }

    float GetStormStrengthAdjustment() const override { return mSimulationParameters.StormStrengthAdjustment; }
    void SetStormStrengthAdjustment(float value) override { mSimulationParameters.StormStrengthAdjustment = value; }
    float GetMinStormStrengthAdjustment() const override { return SimulationParameters::MinStormStrengthAdjustment; }
    float GetMaxStormStrengthAdjustment() const override { return SimulationParameters::MaxStormStrengthAdjustment; }

    bool GetDoRainWithStorm() const override { return mSimulationParameters.DoRainWithStorm; }
    void SetDoRainWithStorm(bool value) override { mSimulationParameters.DoRainWithStorm = value; }

    float GetRainFloodAdjustment() const override { return mSimulationParameters.RainFloodAdjustment; }
    void SetRainFloodAdjustment(float value) override { mSimulationParameters.RainFloodAdjustment = value; }
    float GetMinRainFloodAdjustment() const override { return SimulationParameters::MinRainFloodAdjustment; }
    float GetMaxRainFloodAdjustment() const override { return SimulationParameters::MaxRainFloodAdjustment; }

    float GetLightningBlastProbability() const override { return mSimulationParameters.LightningBlastProbability; }
    void SetLightningBlastProbability(float value) override { mSimulationParameters.LightningBlastProbability = value; }

    // Heat

    float GetAirTemperature() const override { return mSimulationParameters.AirTemperature; }
    void SetAirTemperature(float value) override { mSimulationParameters.AirTemperature = value; }
    float GetMinAirTemperature() const override { return SimulationParameters::MinAirTemperature; }
    float GetMaxAirTemperature() const override { return SimulationParameters::MaxAirTemperature; }

    float GetWaterTemperature() const override { return mSimulationParameters.WaterTemperature; }
    void SetWaterTemperature(float value) override { mSimulationParameters.WaterTemperature = value; }
    float GetMinWaterTemperature() const override { return SimulationParameters::MinWaterTemperature; }
    float GetMaxWaterTemperature() const override { return SimulationParameters::MaxWaterTemperature; }

    unsigned int GetMaxBurningParticlesPerShip() const override { return mSimulationParameters.MaxBurningParticlesPerShip; }
    void SetMaxBurningParticlesPerShip(unsigned int value) override { mSimulationParameters.MaxBurningParticlesPerShip = value; }
    unsigned int GetMinMaxBurningParticlesPerShip() const override { return SimulationParameters::MinMaxBurningParticlesPerShip; }
    unsigned int GetMaxMaxBurningParticlesPerShip() const override { return SimulationParameters::MaxMaxBurningParticlesPerShip; }

    float GetThermalConductivityAdjustment() const override { return mSimulationParameters.ThermalConductivityAdjustment; }
    void SetThermalConductivityAdjustment(float value) override { mSimulationParameters.ThermalConductivityAdjustment = value; }
    float GetMinThermalConductivityAdjustment() const override { return SimulationParameters::MinThermalConductivityAdjustment; }
    float GetMaxThermalConductivityAdjustment() const override { return SimulationParameters::MaxThermalConductivityAdjustment; }

    float GetHeatDissipationAdjustment() const override { return mSimulationParameters.HeatDissipationAdjustment; }
    void SetHeatDissipationAdjustment(float value) override { mSimulationParameters.HeatDissipationAdjustment = value; }
    float GetMinHeatDissipationAdjustment() const override { return SimulationParameters::MinHeatDissipationAdjustment; }
    float GetMaxHeatDissipationAdjustment() const override { return SimulationParameters::MaxHeatDissipationAdjustment; }

    float GetIgnitionTemperatureAdjustment() const override { return mSimulationParameters.IgnitionTemperatureAdjustment; }
    void SetIgnitionTemperatureAdjustment(float value) override { mSimulationParameters.IgnitionTemperatureAdjustment = value; }
    float GetMinIgnitionTemperatureAdjustment() const override { return SimulationParameters::MinIgnitionTemperatureAdjustment; }
    float GetMaxIgnitionTemperatureAdjustment() const override { return SimulationParameters::MaxIgnitionTemperatureAdjustment; }

    float GetMeltingTemperatureAdjustment() const override { return mSimulationParameters.MeltingTemperatureAdjustment; }
    void SetMeltingTemperatureAdjustment(float value) override { mSimulationParameters.MeltingTemperatureAdjustment = value; }
    float GetMinMeltingTemperatureAdjustment() const override { return SimulationParameters::MinMeltingTemperatureAdjustment; }
    float GetMaxMeltingTemperatureAdjustment() const override { return SimulationParameters::MaxMeltingTemperatureAdjustment; }

    float GetCombustionSpeedAdjustment() const override { return mSimulationParameters.CombustionSpeedAdjustment; }
    void SetCombustionSpeedAdjustment(float value) override { mSimulationParameters.CombustionSpeedAdjustment = value; }
    float GetMinCombustionSpeedAdjustment() const override { return SimulationParameters::MinCombustionSpeedAdjustment; }
    float GetMaxCombustionSpeedAdjustment() const override { return SimulationParameters::MaxCombustionSpeedAdjustment; }

    float GetCombustionHeatAdjustment() const override { return mSimulationParameters.CombustionHeatAdjustment; }
    void SetCombustionHeatAdjustment(float value) override { mSimulationParameters.CombustionHeatAdjustment = value; }
    float GetMinCombustionHeatAdjustment() const override { return SimulationParameters::MinCombustionHeatAdjustment; }
    float GetMaxCombustionHeatAdjustment() const override { return SimulationParameters::MaxCombustionHeatAdjustment; }

    float GetHeatBlasterHeatFlow() const override { return mSimulationParameters.HeatBlasterHeatFlow; }
    void SetHeatBlasterHeatFlow(float value) override { mSimulationParameters.HeatBlasterHeatFlow = value; }
    float GetMinHeatBlasterHeatFlow() const override { return SimulationParameters::MinHeatBlasterHeatFlow; }
    float GetMaxHeatBlasterHeatFlow() const override { return SimulationParameters::MaxHeatBlasterHeatFlow; }

    float GetHeatBlasterRadius() const override { return mSimulationParameters.HeatBlasterRadius; }
    void SetHeatBlasterRadius(float value) override { mSimulationParameters.HeatBlasterRadius = value; }
    float GetMinHeatBlasterRadius() const override { return SimulationParameters::MinHeatBlasterRadius; }
    float GetMaxHeatBlasterRadius() const override { return SimulationParameters::MaxHeatBlasterRadius; }

    float GetLaserRayHeatFlow() const override { return mSimulationParameters.LaserRayHeatFlow; }
    void SetLaserRayHeatFlow(float value) override { mSimulationParameters.LaserRayHeatFlow = value; }
    float GetMinLaserRayHeatFlow() const override { return SimulationParameters::MinLaserRayHeatFlow; }
    float GetMaxLaserRayHeatFlow() const override { return SimulationParameters::MaxLaserRayHeatFlow; }

    float GetElectricalElementHeatProducedAdjustment() const override { return mSimulationParameters.ElectricalElementHeatProducedAdjustment; }
    void SetElectricalElementHeatProducedAdjustment(float value) override { mSimulationParameters.ElectricalElementHeatProducedAdjustment = value; }
    float GetMinElectricalElementHeatProducedAdjustment() const override { return SimulationParameters::MinElectricalElementHeatProducedAdjustment; }
    float GetMaxElectricalElementHeatProducedAdjustment() const override { return SimulationParameters::MaxElectricalElementHeatProducedAdjustment; }

    float GetEngineThrustAdjustment() const override { return mSimulationParameters.EngineThrustAdjustment; }
    void SetEngineThrustAdjustment(float value) override { mSimulationParameters.EngineThrustAdjustment = value; }
    float GetMinEngineThrustAdjustment() const override { return SimulationParameters::MinEngineThrustAdjustment; }
    float GetMaxEngineThrustAdjustment() const override { return SimulationParameters::MaxEngineThrustAdjustment; }

    bool GetDoEnginesWorkAboveWater() const override { return mSimulationParameters.DoEnginesWorkAboveWater; }
    void SetDoEnginesWorkAboveWater(bool value) override { mSimulationParameters.DoEnginesWorkAboveWater = value; }

    float GetWaterPumpPowerAdjustment() const override { return mSimulationParameters.WaterPumpPowerAdjustment; }
    void SetWaterPumpPowerAdjustment(float value) override { mSimulationParameters.WaterPumpPowerAdjustment = value; }
    float GetMinWaterPumpPowerAdjustment() const override { return SimulationParameters::MinWaterPumpPowerAdjustment; }
    float GetMaxWaterPumpPowerAdjustment() const override { return SimulationParameters::MaxWaterPumpPowerAdjustment; }

    // Fishes

    unsigned int GetNumberOfFishes() const override { return mSimulationParameters.NumberOfFishes; }
    void SetNumberOfFishes(unsigned int value) override { mSimulationParameters.NumberOfFishes = value; }
    unsigned int GetMinNumberOfFishes() const override { return SimulationParameters::MinNumberOfFishes; }
    unsigned int GetMaxNumberOfFishes() const override { return SimulationParameters::MaxNumberOfFishes; }

    float GetFishSizeMultiplier() const override { return mFloatParameterSmoothers[FishSizeMultiplierParameterSmoother].GetValue(); }
    void SetFishSizeMultiplier(float value) override { mFloatParameterSmoothers[FishSizeMultiplierParameterSmoother].SetValue(value); }
    float GetMinFishSizeMultiplier() const override { return SimulationParameters::MinFishSizeMultiplier; }
    float GetMaxFishSizeMultiplier() const override { return SimulationParameters::MaxFishSizeMultiplier; }

    float GetFishSpeedAdjustment() const override { return mSimulationParameters.FishSpeedAdjustment; }
    void SetFishSpeedAdjustment(float value) override { mSimulationParameters.FishSpeedAdjustment = value; }
    float GetMinFishSpeedAdjustment() const override { return SimulationParameters::MinFishSpeedAdjustment; }
    float GetMaxFishSpeedAdjustment() const override { return SimulationParameters::MaxFishSpeedAdjustment; }

    bool GetDoFishShoaling() const override { return mSimulationParameters.DoFishShoaling; }
    void SetDoFishShoaling(bool value) override { mSimulationParameters.DoFishShoaling = value; }

    float GetFishShoalRadiusAdjustment() const override { return mSimulationParameters.FishShoalRadiusAdjustment; }
    void SetFishShoalRadiusAdjustment(float value) override { mSimulationParameters.FishShoalRadiusAdjustment = value; }
    float GetMinFishShoalRadiusAdjustment() const override { return SimulationParameters::MinFishShoalRadiusAdjustment; }
    float GetMaxFishShoalRadiusAdjustment() const override { return SimulationParameters::MaxFishShoalRadiusAdjustment; }

    // NPCs

    float GetNpcSpringReductionFractionAdjustment() const override { return mSimulationParameters.NpcSpringReductionFractionAdjustment; }
    void SetNpcSpringReductionFractionAdjustment(float value) override { mSimulationParameters.NpcSpringReductionFractionAdjustment = value; }
    float GetMinNpcSpringReductionFractionAdjustment() const override { return SimulationParameters::MinNpcSpringReductionFractionAdjustment; }
    float GetMaxNpcSpringReductionFractionAdjustment() const override { return SimulationParameters::MaxNpcSpringReductionFractionAdjustment; }

    float GetNpcSpringDampingCoefficientAdjustment() const override { return mSimulationParameters.NpcSpringDampingCoefficientAdjustment; }
    void SetNpcSpringDampingCoefficientAdjustment(float value) override { mSimulationParameters.NpcSpringDampingCoefficientAdjustment = value; }
    float GetMinNpcSpringDampingCoefficientAdjustment() const override { return SimulationParameters::MinNpcSpringDampingCoefficientAdjustment; }
    float GetMaxNpcSpringDampingCoefficientAdjustment() const override { return SimulationParameters::MaxNpcSpringDampingCoefficientAdjustment; }

    float GetNpcFrictionAdjustment() const override { return mSimulationParameters.NpcFrictionAdjustment; }
    void SetNpcFrictionAdjustment(float value) override { mSimulationParameters.NpcFrictionAdjustment = value; }
    float GetMinNpcFrictionAdjustment() const override { return SimulationParameters::MinNpcFrictionAdjustment; }
    float GetMaxNpcFrictionAdjustment() const override { return SimulationParameters::MaxNpcFrictionAdjustment; }

    float GetMinNpcWindReceptivityAdjustment() const override { return SimulationParameters::MinNpcWindReceptivityAdjustment; }
    float GetMaxNpcWindReceptivityAdjustment() const override { return SimulationParameters::MaxNpcWindReceptivityAdjustment; }

    float GetNpcSizeMultiplier() const override { return mFloatParameterSmoothers[NpcSizeMultiplierParameterSmoother].GetValue(); }
    void SetNpcSizeMultiplier(float value) override { mFloatParameterSmoothers[NpcSizeMultiplierParameterSmoother].SetValue(value); }
    float GetMinNpcSizeMultiplier() const override { return SimulationParameters::MinNpcSizeMultiplier; }
    float GetMaxNpcSizeMultiplier() const override { return SimulationParameters::MaxNpcSizeMultiplier; }

    float GetNpcPassiveBlastRadiusAdjustment() const override { return mSimulationParameters.NpcPassiveBlastRadiusAdjustment; }
    void SetNpcPassiveBlastRadiusAdjustment(float value) override { mSimulationParameters.NpcPassiveBlastRadiusAdjustment = value; }
    float GetMinNpcPassiveBlastRadiusAdjustment() const override { return SimulationParameters::MinNpcPassiveBlastRadiusAdjustment; }
    float GetMaxNpcPassiveBlastRadiusAdjustment() const override { return SimulationParameters::MaxNpcPassiveBlastRadiusAdjustment; }

    // Misc

    OceanFloorHeightMap const & GetOceanFloorTerrain() const override { return mWorld->GetOceanFloorHeightMap(); }
    void SetOceanFloorTerrain(OceanFloorHeightMap const & value) override { mWorld->SetOceanFloorHeightMap(value); }

    float GetSeaDepth() const override { return mFloatParameterSmoothers[SeaDepthParameterSmoother].GetValue(); }
    void SetSeaDepth(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValue(value); }
    void SetSeaDepthImmediate(float value) override { mFloatParameterSmoothers[SeaDepthParameterSmoother].SetValueImmediate(value); }
    float GetMinSeaDepth() const override { return SimulationParameters::MinSeaDepth; }
    float GetMaxSeaDepth() const override { return SimulationParameters::MaxSeaDepth; }

    float GetOceanFloorBumpiness() const override { return mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].GetValue(); }
    void SetOceanFloorBumpiness(float value) override { mFloatParameterSmoothers[OceanFloorBumpinessParameterSmoother].SetValue(value); }
    float GetMinOceanFloorBumpiness() const override { return SimulationParameters::MinOceanFloorBumpiness; }
    float GetMaxOceanFloorBumpiness() const override { return SimulationParameters::MaxOceanFloorBumpiness; }

    float GetOceanFloorDetailAmplification() const override { return mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].GetValue(); }
    void SetOceanFloorDetailAmplification(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValue(value); }
    void SetOceanFloorDetailAmplificationImmediate(float value) override { mFloatParameterSmoothers[OceanFloorDetailAmplificationParameterSmoother].SetValueImmediate(value); }
    float GetMinOceanFloorDetailAmplification() const override { return SimulationParameters::MinOceanFloorDetailAmplification; }
    float GetMaxOceanFloorDetailAmplification() const override { return SimulationParameters::MaxOceanFloorDetailAmplification; }

    float GetOceanFloorElasticityCoefficient() const override { return mSimulationParameters.OceanFloorElasticityCoefficient; }
    void SetOceanFloorElasticityCoefficient(float value) override { mSimulationParameters.OceanFloorElasticityCoefficient = value; }
    float GetMinOceanFloorElasticityCoefficient() const override { return SimulationParameters::MinOceanFloorElasticityCoefficient; }
    float GetMaxOceanFloorElasticityCoefficient() const override { return SimulationParameters::MaxOceanFloorElasticityCoefficient; }

    float GetOceanFloorFrictionCoefficient() const override { return mSimulationParameters.OceanFloorFrictionCoefficient; }
    void SetOceanFloorFrictionCoefficient(float value) override { mSimulationParameters.OceanFloorFrictionCoefficient = value; }
    float GetMinOceanFloorFrictionCoefficient() const override { return SimulationParameters::MinOceanFloorFrictionCoefficient; }
    float GetMaxOceanFloorFrictionCoefficient() const override { return SimulationParameters::MaxOceanFloorFrictionCoefficient; }

    float GetOceanFloorSiltHardness() const override { return mSimulationParameters.OceanFloorSiltHardness; }
    void SetOceanFloorSiltHardness(float value) override { mSimulationParameters.OceanFloorSiltHardness = value; }
    float GetMinOceanFloorSiltHardness() const override { return SimulationParameters::MinOceanFloorSiltHardness; }
    float GetMaxOceanFloorSiltHardness() const override { return SimulationParameters::MaxOceanFloorSiltHardness; }

    float GetDestroyRadius() const override { return mSimulationParameters.DestroyRadius; }
    void SetDestroyRadius(float value) override { mSimulationParameters.DestroyRadius = value; }
    float GetMinDestroyRadius() const override { return SimulationParameters::MinDestroyRadius; }
    float GetMaxDestroyRadius() const override { return SimulationParameters::MaxDestroyRadius; }

    float GetRepairRadius() const override { return mSimulationParameters.RepairRadius; }
    void SetRepairRadius(float value) override { mSimulationParameters.RepairRadius = value; }
    float GetMinRepairRadius() const override { return SimulationParameters::MinRepairRadius; }
    float GetMaxRepairRadius() const override { return SimulationParameters::MaxRepairRadius; }

    float GetRepairSpeedAdjustment() const override { return mSimulationParameters.RepairSpeedAdjustment; }
    void SetRepairSpeedAdjustment(float value) override { mSimulationParameters.RepairSpeedAdjustment = value; }
    float GetMinRepairSpeedAdjustment() const override { return SimulationParameters::MinRepairSpeedAdjustment; }
    float GetMaxRepairSpeedAdjustment() const override { return SimulationParameters::MaxRepairSpeedAdjustment; }

    bool GetDoApplyPhysicsToolsToShips() const override { return mSimulationParameters.DoApplyPhysicsToolsToShips; }
    void SetDoApplyPhysicsToolsToShips(bool value) override { mSimulationParameters.DoApplyPhysicsToolsToShips = value; }

    bool GetDoApplyPhysicsToolsToNpcs() const override { return mSimulationParameters.DoApplyPhysicsToolsToNpcs; }
    void SetDoApplyPhysicsToolsToNpcs(bool value) override { mSimulationParameters.DoApplyPhysicsToolsToNpcs = value; }

    float GetBombBlastRadius() const override { return mSimulationParameters.BombBlastRadius; }
    void SetBombBlastRadius(float value) override { mSimulationParameters.BombBlastRadius = value; }
    float GetMinBombBlastRadius() const override { return SimulationParameters::MinBombBlastRadius; }
    float GetMaxBombBlastRadius() const override { return SimulationParameters::MaxBombBlastRadius; }

    float GetBombBlastForceAdjustment() const override { return mSimulationParameters.BombBlastForceAdjustment; }
    void SetBombBlastForceAdjustment(float value) override { mSimulationParameters.BombBlastForceAdjustment = value; }
    float GetMinBombBlastForceAdjustment() const override { return SimulationParameters::MinBombBlastForceAdjustment; }
    float GetMaxBombBlastForceAdjustment() const override { return SimulationParameters::MaxBombBlastForceAdjustment; }

    float GetBombBlastHeat() const override { return mSimulationParameters.BombBlastHeat; }
    void SetBombBlastHeat(float value) override { mSimulationParameters.BombBlastHeat = value; }
    float GetMinBombBlastHeat() const override { return SimulationParameters::MinBombBlastHeat; }
    float GetMaxBombBlastHeat() const override { return SimulationParameters::MaxBombBlastHeat; }

    float GetAntiMatterBombImplosionStrength() const override { return mSimulationParameters.AntiMatterBombImplosionStrength; }
    void SetAntiMatterBombImplosionStrength(float value) override { mSimulationParameters.AntiMatterBombImplosionStrength = value; }
    float GetMinAntiMatterBombImplosionStrength() const override { return SimulationParameters::MinAntiMatterBombImplosionStrength; }
    float GetMaxAntiMatterBombImplosionStrength() const override { return SimulationParameters::MaxAntiMatterBombImplosionStrength; }

    float GetFloodRadius() const override { return mSimulationParameters.FloodRadius; }
    void SetFloodRadius(float value) override { mSimulationParameters.FloodRadius = value; }
    float GetMinFloodRadius() const override { return SimulationParameters::MinFloodRadius; }
    float GetMaxFloodRadius() const override { return SimulationParameters::MaxFloodRadius; }

    float GetFloodQuantity() const override { return mSimulationParameters.FloodQuantity; }
    void SetFloodQuantity(float value) override { mSimulationParameters.FloodQuantity = value; }
    float GetMinFloodQuantity() const override { return SimulationParameters::MinFloodQuantity; }
    float GetMaxFloodQuantity() const override { return SimulationParameters::MaxFloodQuantity; }

    float GetInjectPressureQuantity() const override { return mSimulationParameters.InjectPressureQuantity; }
    void SetInjectPressureQuantity(float value) override { mSimulationParameters.InjectPressureQuantity = value; }
    float GetMinInjectPressureQuantity() const override { return SimulationParameters::MinInjectPressureQuantity; }
    float GetMaxInjectPressureQuantity() const override { return SimulationParameters::MaxInjectPressureQuantity; }

    float GetBlastToolRadius() const override { return mSimulationParameters.BlastToolRadius; }
    void SetBlastToolRadius(float value) override { mSimulationParameters.BlastToolRadius = value; }
    float GetMinBlastToolRadius() const override { return SimulationParameters::MinBlastToolRadius; }
    float GetMaxBlastToolRadius() const override { return SimulationParameters::MaxBlastToolRadius; }

    float GetBlastToolForceAdjustment() const override { return mSimulationParameters.BlastToolForceAdjustment; }
    void SetBlastToolForceAdjustment(float value) override { mSimulationParameters.BlastToolForceAdjustment = value; }
    float GetMinBlastToolForceAdjustment() const override { return SimulationParameters::MinBlastToolForceAdjustment; }
    float GetMaxBlastToolForceAdjustment() const override { return SimulationParameters::MaxBlastToolForceAdjustment; }

    float GetScrubRotToolRadius() const override { return mSimulationParameters.ScrubRotToolRadius; }
    void SetScrubRotToolRadius(float value) override { mSimulationParameters.ScrubRotToolRadius = value; }
    float GetMinScrubRotToolRadius() const override { return SimulationParameters::MinScrubRotToolRadius; }
    float GetMaxScrubRotToolRadius() const override { return SimulationParameters::MaxScrubRotToolRadius; }

    float GetWindMakerToolWindSpeed() const override { return mSimulationParameters.WindMakerToolWindSpeed; }
    void SetWindMakerToolWindSpeed(float value) override { mSimulationParameters.WindMakerToolWindSpeed = value; }
    float GetMinWindMakerToolWindSpeed() const override { return SimulationParameters::MinWindMakerToolWindSpeed; }
    float GetMaxWindMakerToolWindSpeed() const override { return SimulationParameters::MaxWindMakerToolWindSpeed; }

    float GetLuminiscenceAdjustment() const override { return mSimulationParameters.LuminiscenceAdjustment; }
    void SetLuminiscenceAdjustment(float value) override { mSimulationParameters.LuminiscenceAdjustment = value; }
    float GetMinLuminiscenceAdjustment() const override { return SimulationParameters::MinLuminiscenceAdjustment; }
    float GetMaxLuminiscenceAdjustment() const override { return SimulationParameters::MaxLuminiscenceAdjustment; }

    float GetLightSpreadAdjustment() const override { return mSimulationParameters.LightSpreadAdjustment; }
    void SetLightSpreadAdjustment(float value) override { mSimulationParameters.LightSpreadAdjustment = value; }
    float GetMinLightSpreadAdjustment() const override { return SimulationParameters::MinLightSpreadAdjustment; }
    float GetMaxLightSpreadAdjustment() const override { return SimulationParameters::MaxLightSpreadAdjustment; }

    bool GetUltraViolentMode() const override { return mSimulationParameters.IsUltraViolentMode; }
    void SetUltraViolentMode(bool value) override { mSimulationParameters.IsUltraViolentMode = value; mNotificationLayer.SetUltraViolentModeIndicator(value); }

    bool GetDoGenerateDebris() const override { return mSimulationParameters.DoGenerateDebris; }
    void SetDoGenerateDebris(bool value) override { mSimulationParameters.DoGenerateDebris = value; }

    bool GetDoGenerateSparklesForCuts() const override { return mSimulationParameters.DoGenerateSparklesForCuts; }
    void SetDoGenerateSparklesForCuts(bool value) override { mSimulationParameters.DoGenerateSparklesForCuts = value; }

    float GetAirBubblesDensity() const override { return mSimulationParameters.AirBubblesDensity; }
    void SetAirBubblesDensity(float value) override { mSimulationParameters.AirBubblesDensity = value; }
    float GetMaxAirBubblesDensity() const override { return SimulationParameters::MaxAirBubblesDensity; }
    float GetMinAirBubblesDensity() const override { return SimulationParameters::MinAirBubblesDensity; }

    bool GetDoGenerateEngineWakeParticles() const override { return mSimulationParameters.DoGenerateEngineWakeParticles; }
    void SetDoGenerateEngineWakeParticles(bool value) override { mSimulationParameters.DoGenerateEngineWakeParticles = value; }

    unsigned int GetNumberOfStars() const override { return mSimulationParameters.NumberOfStars; }
    void SetNumberOfStars(unsigned int value) override { mSimulationParameters.NumberOfStars = value; }
    unsigned int GetMinNumberOfStars() const override { return SimulationParameters::MinNumberOfStars; }
    unsigned int GetMaxNumberOfStars() const override { return SimulationParameters::MaxNumberOfStars; }

    unsigned int GetNumberOfClouds() const override { return mSimulationParameters.NumberOfClouds; }
    void SetNumberOfClouds(unsigned int value) override { mSimulationParameters.NumberOfClouds = value; }
    unsigned int GetMinNumberOfClouds() const override { return SimulationParameters::MinNumberOfClouds; }
    unsigned int GetMaxNumberOfClouds() const override { return SimulationParameters::MaxNumberOfClouds; }

    bool GetDoDayLightCycle() const override { return mSimulationParameters.DoDayLightCycle; }
    void SetDoDayLightCycle(bool value) override;

    std::chrono::minutes GetDayLightCycleDuration() const override { return mSimulationParameters.DayLightCycleDuration; }
    void SetDayLightCycleDuration(std::chrono::minutes value) override { mSimulationParameters.DayLightCycleDuration = value; }
    std::chrono::minutes GetMinDayLightCycleDuration() const override { return SimulationParameters::MinDayLightCycleDuration; }
    std::chrono::minutes GetMaxDayLightCycleDuration() const override { return SimulationParameters::MaxDayLightCycleDuration; }

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
    float GetMinWaterLevelOfDetail() const override { return RenderContext::MinShipWaterLevelOfDetail; }
    float GetMaxWaterLevelOfDetail() const override { return RenderContext::MaxShipWaterLevelOfDetail; }

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

    ShipParticleRenderModeType GetShipParticleRenderMode() const override { return mRenderContext->GetShipParticleRenderMode(); }
    void SetShipParticleRenderMode(ShipParticleRenderModeType shipParticleRenderMode) override { mRenderContext->SetShipParticleRenderMode(shipParticleRenderMode); }

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
    float GetMinShipFlameSizeAdjustment() const override { return RenderContext::MinShipFlameSizeAdjustment; }
    float GetMaxShipFlameSizeAdjustment() const override { return RenderContext::MaxShipFlameSizeAdjustment; }

    float GetShipFlameKaosAdjustment() const override { return mRenderContext->GetShipFlameKaosAdjustment(); }
    void SetShipFlameKaosAdjustment(float value) override { mRenderContext->SetShipFlameKaosAdjustment(value); }
    float GetMinShipFlameKaosAdjustment() const override { return RenderContext::MinShipFlameKaosAdjustment; }
    float GetMaxShipFlameKaosAdjustment() const override { return RenderContext::MaxShipFlameKaosAdjustment; }

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
        std::unique_ptr<RenderContext> renderContext,
        std::unique_ptr<PerfStats> perfStats,
        FishSpeciesDatabase && fishSpeciesDatabase,
        NpcDatabase && npcDatabase,
        MaterialDatabase && materialDatabase,
        ThreadManager & threadManager,
        GameAssetManager const & gameAssetManager,
        ProgressCallback const & progressCallback);

    ShipMetadata InternalResetAndLoadShip(
        ShipLoadSpecifications const & loadSpecs,
        IAssetManager const & assetManager);

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

    std::optional<AutoFocusTargetKindType> CalculateEffectiveAutoFocusTarget() const;

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

    SimulationParameters mSimulationParameters;
    bool mIsFrozen;
    bool mIsPaused;
    bool mIsPulseUpdateSet;
    bool mIsMoveToolEngaged;


    //
    // The parameters that we own
    //

    float mTimeOfDay;
    bool mIsShiftOn;
    bool mDoShowTsunamiNotifications;
    bool mDoShowNpcNotifications;
    bool mDoDrawHeatBlasterFlame;
    std::optional<AutoFocusTargetKindType> mAutoFocusTarget;


    //
    // The doers
    //

    std::shared_ptr<RenderContext> mRenderContext;
    SimulationEventDispatcher mSimulationEventDispatcher;
    GameEventDispatcher mGameEventDispatcher;
    NotificationLayer mNotificationLayer;
    ThreadManager & mThreadManager;
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
