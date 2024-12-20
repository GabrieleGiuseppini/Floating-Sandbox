/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EventRecorder.h"
#include "IGameEventHandlers.h"
#include "ResourceLocator.h"
#include "ShipAutoTexturizationSettings.h"
#include "ShipLoadSpecifications.h"
#include "ShipMetadata.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/UniqueBuffer.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

/*
 * The interface presented by the GameController class to the external projects.
 */
struct IGameController
{
    virtual ~IGameController()
    {}

    virtual void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * handler) = 0;
    virtual void RegisterStructuralEventHandler(IStructuralGameEventHandler * handler) = 0;
    virtual void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * handler) = 0;
    virtual void RegisterCombustionEventHandler(ICombustionGameEventHandler * handler) = 0;
    virtual void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * handler) = 0;
    virtual void RegisterAtmosphereEventHandler(IAtmosphereGameEventHandler * handler) = 0;
    virtual void RegisterElectricalElementEventHandler(IElectricalElementGameEventHandler * handler) = 0;
    virtual void RegisterNpcEventHandler(INpcGameEventHandler * handler) = 0;
    virtual void RegisterGenericEventHandler(IGenericGameEventHandler * handler) = 0;
    virtual void RegisterControlEventHandler(IControlGameEventHandler * handler) = 0;

    virtual ShipMetadata ResetAndLoadShip(ShipLoadSpecifications const & loadSpecs) = 0;
    virtual ShipMetadata ResetAndReloadShip(ShipLoadSpecifications const & loadSpecs) = 0;
    virtual ShipMetadata AddShip(ShipLoadSpecifications const & loadSpecs) = 0;

    virtual RgbImageData TakeScreenshot() = 0;

    virtual void RunGameIteration() = 0;
    virtual void LowFrequencyUpdate() = 0;

    virtual void PulseUpdateAtNextGameIteration() = 0;

    virtual void StartRecordingEvents(std::function<void(uint32_t, RecordedEvent const &)> onEventCallback) = 0;
    virtual RecordedEvents StopRecordingEvents() = 0;
    virtual void ReplayRecordedEvent(RecordedEvent const & event) = 0;


    //
    // Game Control and notifications
    //

    virtual void Freeze() = 0;
    virtual void Thaw() = 0;
    virtual void SetPaused(bool isPaused) = 0;
    virtual void SetMoveToolEngaged(bool isEngaged) = 0;
    virtual void DisplaySettingsLoadedNotification() = 0;

    virtual bool GetShowStatusText() const = 0;
    virtual void SetShowStatusText(bool value) = 0;
    virtual bool GetShowExtendedStatusText() const = 0;
    virtual void SetShowExtendedStatusText(bool value) = 0;

    virtual void DisplayEphemeralTextLine(std::string const & text) = 0;

    virtual void NotifySoundMuted(bool isSoundMuted) = 0;

    virtual bool IsShiftOn() const = 0;
    virtual void SetShiftOn(bool value) = 0;

    // Not Sticky

    virtual void ShowInteractiveToolDashedLine(
        DisplayLogicalCoordinates const & start,
        DisplayLogicalCoordinates const & end) = 0;

    virtual void ShowInteractiveToolDashedRect(
        DisplayLogicalCoordinates const & corner1,
        DisplayLogicalCoordinates const & corner2) = 0;

    //
    // World
    //

    virtual float GetCurrentSimulationTime() const = 0;
    virtual void ToggleToFullDayOrNight() = 0;
    virtual float GetEffectiveAmbientLightIntensity() const = 0;
    virtual bool IsUnderwater(DisplayLogicalCoordinates const & screenCoordinates) const = 0;
    virtual bool IsUnderwater(GlobalElementId elementId) const = 0;
    virtual bool HasNpcs() const = 0;

    //
    // Interactions
    //

    virtual void PickObjectToMove(DisplayLogicalCoordinates const & screenCoordinates, std::optional<GlobalConnectedComponentId> & connectedComponentId) = 0;
    virtual void PickObjectToMove(DisplayLogicalCoordinates const & screenCoordinates, std::optional<ShipId> & shipId) = 0;
    virtual void MoveBy(GlobalConnectedComponentId const & connectedComponentId, DisplayLogicalSize const & screenOffset, DisplayLogicalSize const & inertialScreenOffset) = 0;
    virtual void MoveBy(ShipId shipId, DisplayLogicalSize const & screenOffset, DisplayLogicalSize const & inertialScreenOffset) = 0;
    virtual void RotateBy(GlobalConnectedComponentId const & connectedComponentId, float screenDeltaY, DisplayLogicalCoordinates const & screenCenter, float inertialScreenDeltaY) = 0;
    virtual void RotateBy(ShipId shipId, float screenDeltaY, DisplayLogicalCoordinates const & screenCenter, float intertialScreenDeltaY) = 0;
    virtual std::optional<GlobalElementId> PickObjectForPickAndPull(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void Pull(GlobalElementId elementId, DisplayLogicalCoordinates const & screenTarget) = 0;
    virtual void DestroyAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier) = 0;
    virtual void RepairAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, SequenceNumber repairStepId) = 0;
    virtual bool SawThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, bool isFirstSegment) = 0;
    virtual bool ApplyHeatBlasterAt(DisplayLogicalCoordinates const & screenCoordinates, HeatBlasterActionType action) = 0;
    virtual bool ExtinguishFireAt(DisplayLogicalCoordinates const & screenCoordinates, float strengthMultiplier) = 0;
    virtual void ApplyBlastAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusMultiplier, float forceMultiplier, float renderProgress, float personalitySeed) = 0;
    virtual bool ApplyElectricSparkAt(DisplayLogicalCoordinates const & screenCoordinates, std::uint64_t counter, float lengthMultiplier, float currentSimulationTime) = 0;
    virtual void ApplyRadialWindFrom(DisplayLogicalCoordinates const & sourcePos, float preFrontSimulationTimeElapsed, float preFrontIntensityMultiplier, float mainFrontSimulationTimeElapsed, float mainFrontIntensityMultiplier) = 0;
    virtual bool ApplyLaserCannonThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates, std::optional<float> strength) = 0;
    virtual void DrawTo(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) = 0;
    virtual void SwirlAt(DisplayLogicalCoordinates const & screenCoordinates, float strengthFraction) = 0;
    virtual void TogglePinAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void RemoveAllPins() = 0;
    virtual std::optional<ToolApplicationLocus> InjectPressureAt(DisplayLogicalCoordinates const & screenCoordinates, float pressureQuantityMultiplier) = 0;
    virtual bool FloodAt(DisplayLogicalCoordinates const & screenCoordinates, float waterQuantityMultiplier) = 0;
    virtual void ToggleAntiMatterBombAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void ToggleImpactBombAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void TogglePhysicsProbeAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void ToggleRCBombAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void ToggleTimerBombAt(DisplayLogicalCoordinates const & screenCoordinates) = 0;
    virtual void DetonateRCBombs() = 0;
    virtual void DetonateAntiMatterBombs() = 0;
    virtual void AdjustOceanSurfaceTo(DisplayLogicalCoordinates const & screenCoordinates, int screenRadius) = 0;
    virtual std::optional<bool> AdjustOceanFloorTo(vec2f const & startWorldPosition, vec2f const & endWorldPosition) = 0;
    virtual bool ScrubThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates) = 0;
    virtual bool RotThrough(DisplayLogicalCoordinates const & startScreenCoordinates, DisplayLogicalCoordinates const & endScreenCoordinates) = 0;
    virtual void ApplyThanosSnapAt(DisplayLogicalCoordinates const & screenCoordinates, bool isSparseMode) = 0;
    virtual void ScareFish(DisplayLogicalCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) = 0;
    virtual void AttractFish(DisplayLogicalCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) = 0;
    virtual void SetLampAt(DisplayLogicalCoordinates const & screenCoordinates, float radiusScreenFraction) = 0;
    virtual void ResetLamp() = 0;
    virtual NpcKindType GetNpcKind(NpcId id) = 0;
    virtual std::optional<PickedNpc> BeginPlaceNewFurnitureNpc(std::optional<NpcSubKindIdType> subKind, DisplayLogicalCoordinates const & screenCoordinates, bool doMoveWholeMesh) = 0;
    virtual std::optional<PickedNpc> BeginPlaceNewHumanNpc(std::optional<NpcSubKindIdType> subKind, DisplayLogicalCoordinates const & screenCoordinates, bool doMoveWholeMesh) = 0;
    virtual std::optional<PickedNpc> ProbeNpcAt(DisplayLogicalCoordinates const & screenCoordinates) const = 0;
    virtual std::vector<NpcId> ProbeNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) const = 0;
    virtual void BeginMoveNpc(NpcId id, int particleOrdinal, bool doMoveWholeMesh) = 0;
    virtual void BeginMoveNpcs(std::vector<NpcId> const & ids) = 0;
    virtual void MoveNpcTo(NpcId id, DisplayLogicalCoordinates const & screenCoordinates, vec2f const & worldOffset, bool doMoveWholeMesh) = 0;
    virtual void MoveNpcsBy(std::vector<NpcId> const & ids, DisplayLogicalSize const & screenOffset) = 0;
    virtual void EndMoveNpc(NpcId id) = 0;
    virtual void CompleteNewNpc(NpcId id) = 0;
    virtual void RemoveNpc(NpcId id) = 0;
    virtual void RemoveNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) = 0;
    virtual void AbortNewNpc(NpcId id) = 0;
    virtual void AddNpcGroup(NpcKindType kind) = 0;
    virtual void TurnaroundNpc(NpcId id) = 0;
    virtual void TurnaroundNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) = 0;
    virtual std::optional<NpcId> GetCurrentlySelectedNpc() const = 0;
    virtual void SelectNpc(std::optional<NpcId> id) = 0;
    virtual void SelectNextNpc() = 0;
    virtual void HighlightNpcs(std::vector<NpcId> const & ids) = 0;
    virtual void HighlightNpcsInRect(DisplayLogicalCoordinates const & corner1ScreenCoordinates, DisplayLogicalCoordinates const & corner2ScreenCoordinates) = 0;
    virtual std::optional<GlobalElementId> GetNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const = 0;
    virtual void QueryNearestPointAt(DisplayLogicalCoordinates const & screenCoordinates) const = 0;

    virtual void TriggerTsunami() = 0;
    virtual void TriggerRogueWave() = 0;
    virtual void TriggerStorm() = 0;
    virtual void TriggerLightning() = 0;

    virtual void HighlightElectricalElement(GlobalElectricalElementId electricalElementId) = 0;

    virtual void SetSwitchState(
        GlobalElectricalElementId electricalElementId,
        ElectricalState switchState) = 0;

    virtual void SetEngineControllerState(
        GlobalElectricalElementId electricalElementId,
        float controllerValue) = 0;

    virtual bool DestroyTriangle(GlobalElementId triangleId) = 0;
    virtual bool RestoreTriangle(GlobalElementId triangleId) = 0;

    //
    // Rendering controls and parameters
    //

    virtual void SetCanvasSize(DisplayLogicalSize const & canvasSize) = 0;
    virtual void Pan(DisplayLogicalSize const & screenOffset) = 0;
    virtual void PanToWorldEnd(int side) = 0;
    virtual void AdjustZoom(float amount) = 0;
    virtual void ResetView() = 0;
    virtual void FocusOnShips() = 0;
    virtual vec2f ScreenToWorld(DisplayLogicalCoordinates const & screenCoordinates) const = 0;
    virtual vec2f ScreenOffsetToWorldOffset(DisplayLogicalSize const & screenOffset) const = 0;

    virtual float GetCameraSpeedAdjustment() const = 0;
    virtual void SetCameraSpeedAdjustment(float value) = 0;
    virtual float GetMinCameraSpeedAdjustment() const = 0;
    virtual float GetMaxCameraSpeedAdjustment() const = 0;

    virtual bool GetDoAutoFocusOnShipLoad() const = 0;
    virtual void SetDoAutoFocusOnShipLoad(bool value) = 0;

    virtual bool GetDoAutoFocusOnNpcPlacement() const = 0;
    virtual void SetDoAutoFocusOnNpcPlacement(bool value) = 0;

    virtual std::optional<AutoFocusTargetKindType> GetAutoFocusTarget() const = 0;
    virtual void SetAutoFocusTarget(std::optional<AutoFocusTargetKindType> const & autoFocusTarget) = 0;

    //
    // NPCs
    //

    virtual size_t GetMaxNpcs() const = 0;
    virtual void SetMaxNpcs(size_t value) = 0;
    virtual size_t GetMinMaxNpcs() const = 0;
    virtual size_t GetMaxMaxNpcs() const = 0;

    virtual size_t GetNpcsPerGroup() const = 0;
    virtual void SetNpcsPerGroup(size_t value) = 0;
    virtual size_t GetMinNpcsPerGroup() const = 0;
    virtual size_t GetMaxNpcsPerGroup() const = 0;

    //
    // UI parameters
    //

    virtual bool GetDoShowTsunamiNotifications() const = 0;
    virtual void SetDoShowTsunamiNotifications(bool value) = 0;

    virtual bool GetDoShowElectricalNotifications() const = 0;
    virtual void SetDoShowElectricalNotifications(bool value) = 0;

    virtual bool GetDoShowNpcNotifications() const = 0;
    virtual void SetDoShowNpcNotifications(bool value) = 0;

    virtual UnitsSystem GetDisplayUnitsSystem() const = 0;
    virtual void SetDisplayUnitsSystem(UnitsSystem value) = 0;

    //
    // Ship building parameters
    //

    virtual ShipAutoTexturizationSettings const & GetShipAutoTexturizationSharedSettings() const = 0;
    virtual ShipAutoTexturizationSettings & GetShipAutoTexturizationSharedSettings() = 0;
    virtual void SetShipAutoTexturizationSharedSettings(ShipAutoTexturizationSettings const & value) = 0;

    virtual bool GetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings() const = 0;
    virtual void SetShipAutoTexturizationDoForceSharedSettingsOntoShipSettings(bool value) = 0;
};
