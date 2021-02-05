/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "EventRecorder.h"
#include "IGameEventHandlers.h"
#include "ResourceLocator.h"
#include "ShipMetadata.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/UniqueBuffer.h>
#include <GameCore/Vectors.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <optional>
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
    virtual void RegisterGenericEventHandler(IGenericGameEventHandler * handler) = 0;

    virtual ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath) = 0;
    virtual ShipMetadata ResetAndReloadShip(std::filesystem::path const & shipDefinitionFilepath) = 0;
    virtual ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath) = 0;

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

    virtual void SetPaused(bool isPaused) = 0;
    virtual void SetMoveToolEngaged(bool isEngaged) = 0;
    virtual void DisplaySettingsLoadedNotification() = 0;

    virtual bool GetShowStatusText() const = 0;
    virtual void SetShowStatusText(bool value) = 0;
    virtual bool GetShowExtendedStatusText() const = 0;
    virtual void SetShowExtendedStatusText(bool value) = 0;

    virtual void NotifySoundMuted(bool isSoundMuted) = 0;

    //
    // World Probing
    //

    virtual float GetCurrentSimulationTime() const = 0;
    virtual float GetEffectiveAmbientLightIntensity() const = 0;
    virtual bool IsUnderwater(LogicalPixelCoordinates const & screenCoordinates) const = 0;
    virtual bool IsUnderwater(ElementId elementId) const = 0;

    //
    // Interactions
    //

    virtual void ScareFish(LogicalPixelCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) = 0;
    virtual void AttractFish(LogicalPixelCoordinates const & screenCoordinates, float radius, std::chrono::milliseconds delay) = 0;

    virtual void PickObjectToMove(LogicalPixelCoordinates const & screenCoordinates, std::optional<ElementId> & elementId) = 0;
    virtual void PickObjectToMove(LogicalPixelCoordinates const & screenCoordinates, std::optional<ShipId> & shipId) = 0;
    virtual void MoveBy(ElementId elementId, LogicalPixelSize const & screenOffset, LogicalPixelSize const & inertialScreenOffset) = 0;
    virtual void MoveBy(ShipId shipId, LogicalPixelSize const & screenOffset, LogicalPixelSize const & inertialScreenOffset) = 0;
    virtual void RotateBy(ElementId elementId, float screenDeltaY, LogicalPixelCoordinates const & screenCenter, float inertialScreenDeltaY) = 0;
    virtual void RotateBy(ShipId shipId, float screenDeltaY, LogicalPixelCoordinates const & screenCenter, float intertialScreenDeltaY) = 0;
    virtual std::optional<ElementId> PickObjectForPickAndPull(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void Pull(ElementId elementId, LogicalPixelCoordinates const & screenTarget) = 0;
    virtual void DestroyAt(LogicalPixelCoordinates const & screenCoordinates, float radiusMultiplier) = 0;
    virtual void RepairAt(LogicalPixelCoordinates const & screenCoordinates, float radiusMultiplier, RepairSessionId sessionId, RepairSessionStepId sessionStepId) = 0;
    virtual void SawThrough(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) = 0;
    virtual bool ApplyHeatBlasterAt(LogicalPixelCoordinates const & screenCoordinates, HeatBlasterActionType action) = 0;
    virtual bool ExtinguishFireAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void DrawTo(LogicalPixelCoordinates const & screenCoordinates, float strengthFraction) = 0;
    virtual void SwirlAt(LogicalPixelCoordinates const & screenCoordinates, float strengthFraction) = 0;
    virtual void TogglePinAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual bool InjectBubblesAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual bool FloodAt(LogicalPixelCoordinates const & screenCoordinates, float waterQuantityMultiplier) = 0;
    virtual void ToggleAntiMatterBombAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void ToggleImpactBombAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void TogglePhysicsProbeAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void ToggleRCBombAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void ToggleTimerBombAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual void DetonateRCBombs() = 0;
    virtual void DetonateAntiMatterBombs() = 0;
    virtual void AdjustOceanSurfaceTo(std::optional<LogicalPixelCoordinates> const & screenCoordinates) = 0;
    virtual std::optional<bool> AdjustOceanFloorTo(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) = 0;
    virtual bool ScrubThrough(LogicalPixelCoordinates const & startScreenCoordinates, LogicalPixelCoordinates const & endScreenCoordinates) = 0;
    virtual void ApplyThanosSnapAt(LogicalPixelCoordinates const & screenCoordinates) = 0;
    virtual std::optional<ElementId> GetNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const = 0;
    virtual void QueryNearestPointAt(LogicalPixelCoordinates const & screenCoordinates) const = 0;

    virtual void TriggerTsunami() = 0;
    virtual void TriggerRogueWave() = 0;
    virtual void TriggerStorm() = 0;
    virtual void TriggerLightning() = 0;

    virtual void HighlightElectricalElement(ElectricalElementId electricalElementId) = 0;

    virtual void SetSwitchState(
        ElectricalElementId electricalElementId,
        ElectricalState switchState) = 0;

    virtual void SetEngineControllerState(
        ElectricalElementId electricalElementId,
        int telegraphValue) = 0;

    virtual bool DestroyTriangle(ElementId triangleId) = 0;
    virtual bool RestoreTriangle(ElementId triangleId) = 0;

    //
    // Rendering controls
    //

    virtual void SetCanvasSize(LogicalPixelSize const & canvasSize) = 0;
    virtual void Pan(LogicalPixelSize const & screenOffset) = 0;
    virtual void PanImmediate(LogicalPixelSize const & screenOffset) = 0;
    virtual void ResetPan() = 0;
    virtual void AdjustZoom(float amount) = 0;
    virtual void ResetZoom() = 0;
    virtual vec2f ScreenToWorld(LogicalPixelCoordinates const & screenCoordinates) const = 0;
    virtual vec2f ScreenOffsetToWorldOffset(LogicalPixelSize const & screenOffset) const = 0;

    //
    // UI parameters
    //

    virtual bool GetDoShowTsunamiNotifications() const = 0;
    virtual void SetDoShowTsunamiNotifications(bool value) = 0;

    virtual bool GetDoShowElectricalNotifications() const = 0;
    virtual void SetDoShowElectricalNotifications(bool value) = 0;

    virtual bool GetDoAutoZoomOnShipLoad() const = 0;
    virtual void SetDoAutoZoomOnShipLoad(bool value) = 0;

    virtual ShipAutoTexturizationSettings const & GetShipAutoTexturizationDefaultSettings() const = 0;
    virtual ShipAutoTexturizationSettings & GetShipAutoTexturizationDefaultSettings() = 0;
    virtual void SetShipAutoTexturizationDefaultSettings(ShipAutoTexturizationSettings const & value) = 0;

    virtual bool GetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings() const = 0;
    virtual void SetShipAutoTexturizationDoForceDefaultSettingsOntoShipSettings(bool value) = 0;
};
