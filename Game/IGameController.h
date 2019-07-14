/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-06-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventHandlers.h"
#include "ShipMetadata.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/Vectors.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/*
 * The interface presented by the GameController class to the external projects.
 */
struct IGameController
{
    virtual void RegisterLifecycleEventHandler(ILifecycleGameEventHandler * handler) = 0;
    virtual void RegisterStructuralEventHandler(IStructuralGameEventHandler * handler) = 0;
    virtual void RegisterWavePhenomenaEventHandler(IWavePhenomenaGameEventHandler * handler) = 0;
    virtual void RegisterCombustionEventHandler(ICombustionGameEventHandler * handler) = 0;
    virtual void RegisterStatisticsEventHandler(IStatisticsGameEventHandler * handler) = 0;
    virtual void RegisterGenericEventHandler(IGenericGameEventHandler * handler) = 0;

    virtual ShipMetadata ResetAndLoadShip(std::filesystem::path const & shipDefinitionFilepath) = 0;
    virtual ShipMetadata AddShip(std::filesystem::path const & shipDefinitionFilepath) = 0;
    virtual void ReloadLastShip() = 0;

    virtual RgbImageData TakeScreenshot() = 0;

    virtual void RunGameIteration() = 0;
    virtual void LowFrequencyUpdate() = 0;

    virtual void Update() = 0;
    virtual void Render() = 0;


    //
    // Game Control
    //

    virtual void SetPaused(bool isPaused) = 0;
    virtual void SetMoveToolEngaged(bool isEngaged) = 0;
    virtual void SetStatusTextEnabled(bool isEnabled) = 0;
    virtual void SetExtendedStatusTextEnabled(bool isEnabled) = 0;

    //
    // World Probing
    //

    virtual float GetCurrentSimulationTime() const = 0;
    virtual bool IsUnderwater(vec2f const & screenCoordinates) const = 0;

    //
    // Interactions
    //

    virtual void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ElementId> & elementId) = 0;
    virtual void PickObjectToMove(vec2f const & screenCoordinates, std::optional<ShipId> & shipId) = 0;
    virtual void MoveBy(ElementId elementId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) = 0;
    virtual void MoveBy(ShipId shipId, vec2f const & screenOffset, vec2f const & inertialScreenOffset) = 0;
    virtual void RotateBy(ElementId elementId, float screenDeltaY, vec2f const & screenCenter, float inertialScreenDeltaY) = 0;
    virtual void RotateBy(ShipId shipId, float screenDeltaY, vec2f const & screenCenter, float intertialScreenDeltaY) = 0;
    virtual void DestroyAt(vec2f const & screenCoordinates, float radiusFraction) = 0;
    virtual void RepairAt(vec2f const & screenCoordinates, float radiusMultiplier, RepairSessionId sessionId, RepairSessionStepId sessionStepId) = 0;
    virtual void SawThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) = 0;
    virtual bool ApplyHeatBlasterAt(vec2f const & screenCoordinates, HeatBlasterActionType action) = 0;
    virtual void DrawTo(vec2f const & screenCoordinates, float strengthFraction) = 0;
    virtual void SwirlAt(vec2f const & screenCoordinates, float strengthFraction) = 0;
    virtual void TogglePinAt(vec2f const & screenCoordinates) = 0;
    virtual bool InjectBubblesAt(vec2f const & screenCoordinates) = 0;
    virtual bool FloodAt(vec2f const & screenCoordinates, float waterQuantityMultiplier) = 0;
    virtual void ToggleAntiMatterBombAt(vec2f const & screenCoordinates) = 0;
    virtual void ToggleImpactBombAt(vec2f const & screenCoordinates) = 0;
    virtual void ToggleRCBombAt(vec2f const & screenCoordinates) = 0;
    virtual void ToggleTimerBombAt(vec2f const & screenCoordinates) = 0;
    virtual void DetonateRCBombs() = 0;
    virtual void DetonateAntiMatterBombs() = 0;
    virtual void AdjustOceanSurfaceTo(std::optional<vec2f> const & screenCoordinates) = 0;
    virtual bool AdjustOceanFloorTo(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) = 0;
    virtual bool ScrubThrough(vec2f const & startScreenCoordinates, vec2f const & endScreenCoordinates) = 0;
    virtual std::optional<ElementId> GetNearestPointAt(vec2f const & screenCoordinates) const = 0;
    virtual void QueryNearestPointAt(vec2f const & screenCoordinates) const = 0;

    virtual void TriggerTsunami() = 0;
    virtual void TriggerRogueWave() = 0;

    //
    // Rendering controls
    //

    virtual void SetCanvasSize(int width, int height) = 0;
    virtual void Pan(vec2f const & screenOffset) = 0;
    virtual void PanImmediate(vec2f const & screenOffset) = 0;
    virtual void ResetPan() = 0;
    virtual void AdjustZoom(float amount) = 0;
    virtual void ResetZoom() = 0;
    virtual vec2f ScreenToWorld(vec2f const & screenCoordinates) const = 0;

    //
    // Game parameters
    //

    virtual float GetNumMechanicalDynamicsIterationsAdjustment() const = 0;
    virtual void SetNumMechanicalDynamicsIterationsAdjustment(float value) = 0;
    virtual float GetMinNumMechanicalDynamicsIterationsAdjustment() const = 0;
    virtual float GetMaxNumMechanicalDynamicsIterationsAdjustment() const = 0;

    virtual float GetSpringStiffnessAdjustment() const = 0;
    virtual void SetSpringStiffnessAdjustment(float value) = 0;
    virtual float GetMinSpringStiffnessAdjustment() const = 0;
    virtual float GetMaxSpringStiffnessAdjustment() const = 0;

    virtual float GetSpringDampingAdjustment() const = 0;
    virtual void SetSpringDampingAdjustment(float value) = 0;
    virtual float GetMinSpringDampingAdjustment() const = 0;
    virtual float GetMaxSpringDampingAdjustment() const = 0;

    virtual float GetSpringStrengthAdjustment() const = 0;
    virtual void SetSpringStrengthAdjustment(float value) = 0;
    virtual float GetMinSpringStrengthAdjustment() const = 0;
    virtual float GetMaxSpringStrengthAdjustment() const = 0;

    virtual float GetRotAcceler8r() const = 0;
    virtual void SetRotAcceler8r(float value) = 0;
    virtual float GetMinRotAcceler8r() const = 0;
    virtual float GetMaxRotAcceler8r() const = 0;

    virtual float GetWaterDensityAdjustment() const = 0;
    virtual void SetWaterDensityAdjustment(float value) = 0;
    virtual float GetMinWaterDensityAdjustment() const = 0;
    virtual float GetMaxWaterDensityAdjustment() const = 0;

    virtual float GetWaterDragAdjustment() const = 0;
    virtual void SetWaterDragAdjustment(float value) = 0;
    virtual float GetMinWaterDragAdjustment() const = 0;
    virtual float GetMaxWaterDragAdjustment() const = 0;

    virtual float GetWaterIntakeAdjustment() const = 0;
    virtual void SetWaterIntakeAdjustment(float value) = 0;
    virtual float GetMinWaterIntakeAdjustment() const = 0;
    virtual float GetMaxWaterIntakeAdjustment() const = 0;

    virtual float GetWaterCrazyness() const = 0;
    virtual void SetWaterCrazyness(float value) = 0;
    virtual float GetMinWaterCrazyness() const = 0;
    virtual float GetMaxWaterCrazyness() const = 0;

    virtual float GetWaterDiffusionSpeedAdjustment() const = 0;
    virtual void SetWaterDiffusionSpeedAdjustment(float value) = 0;
    virtual float GetMinWaterDiffusionSpeedAdjustment() const = 0;
    virtual float GetMaxWaterDiffusionSpeedAdjustment() const = 0;

    virtual float GetBasalWaveHeightAdjustment() const = 0;
    virtual void SetBasalWaveHeightAdjustment(float value) = 0;
    virtual float GetMinBasalWaveHeightAdjustment() const = 0;
    virtual float GetMaxBasalWaveHeightAdjustment() const = 0;

    virtual float GetBasalWaveLengthAdjustment() const = 0;
    virtual void SetBasalWaveLengthAdjustment(float value) = 0;
    virtual float GetMinBasalWaveLengthAdjustment() const = 0;
    virtual float GetMaxBasalWaveLengthAdjustment() const = 0;

    virtual float GetBasalWaveSpeedAdjustment() const = 0;
    virtual void SetBasalWaveSpeedAdjustment(float value) = 0;
    virtual float GetMinBasalWaveSpeedAdjustment() const = 0;
    virtual float GetMaxBasalWaveSpeedAdjustment() const = 0;

    virtual float GetTsunamiRate() const = 0;
    virtual void SetTsunamiRate(float value) = 0;
    virtual float GetMinTsunamiRate() const = 0;
    virtual float GetMaxTsunamiRate() const = 0;

    virtual float GetRogueWaveRate() const = 0;
    virtual void SetRogueWaveRate(float value) = 0;
    virtual float GetMinRogueWaveRate() const = 0;
    virtual float GetMaxRogueWaveRate() const = 0;

    virtual bool GetDoModulateWind() const = 0;
    virtual void SetDoModulateWind(bool value) = 0;

    virtual float GetWindSpeedBase() const = 0;
    virtual void SetWindSpeedBase(float value) = 0;
    virtual float GetMinWindSpeedBase() const = 0;
    virtual float GetMaxWindSpeedBase() const = 0;

    virtual float GetWindSpeedMaxFactor() const = 0;
    virtual void SetWindSpeedMaxFactor(float value) = 0;
    virtual float GetMinWindSpeedMaxFactor() const = 0;
    virtual float GetMaxWindSpeedMaxFactor() const = 0;

    // Heat

    virtual float GetAirTemperature() const = 0;
    virtual void SetAirTemperature(float value) = 0;
    virtual float GetMinAirTemperature() const = 0;
    virtual float GetMaxAirTemperature() const = 0;

    virtual float GetWaterTemperature() const = 0;
    virtual void SetWaterTemperature(float value) = 0;
    virtual float GetMinWaterTemperature() const = 0;
    virtual float GetMaxWaterTemperature() const = 0;

    virtual size_t GetMaxBurningParticles() const = 0;
    virtual void SetMaxBurningParticles(size_t value) = 0;
    virtual float GetMinMaxBurningParticles() const = 0;
    virtual float GetMaxMaxBurningParticles() const = 0;

    virtual float GetThermalConductivityAdjustment() const = 0;
    virtual void SetThermalConductivityAdjustment(float value) = 0;
    virtual float GetMinThermalConductivityAdjustment() const = 0;
    virtual float GetMaxThermalConductivityAdjustment() const = 0;

    virtual float GetHeatDissipationAdjustment() const = 0;
    virtual void SetHeatDissipationAdjustment(float value) = 0;
    virtual float GetMinHeatDissipationAdjustment() const = 0;
    virtual float GetMaxHeatDissipationAdjustment() const = 0;

    virtual float GetIgnitionTemperatureAdjustment() const = 0;
    virtual void SetIgnitionTemperatureAdjustment(float value) = 0;
    virtual float GetMinIgnitionTemperatureAdjustment() const = 0;
    virtual float GetMaxIgnitionTemperatureAdjustment() const = 0;

    virtual float GetMeltingTemperatureAdjustment() const = 0;
    virtual void SetMeltingTemperatureAdjustment(float value) = 0;
    virtual float GetMinMeltingTemperatureAdjustment() const = 0;
    virtual float GetMaxMeltingTemperatureAdjustment() const = 0;

    virtual float GetCombustionSpeedAdjustment() const = 0;
    virtual void SetCombustionSpeedAdjustment(float value) = 0;
    virtual float GetMinCombustionSpeedAdjustment() const = 0;
    virtual float GetMaxCombustionSpeedAdjustment() const = 0;

    virtual float GetCombustionHeatAdjustment() const = 0;
    virtual void SetCombustionHeatAdjustment(float value) = 0;
    virtual float GetMinCombustionHeatAdjustment() const = 0;
    virtual float GetMaxCombustionHeatAdjustment() const = 0;

    virtual float GetHeatBlasterHeatFlow() const = 0;
    virtual void SetHeatBlasterHeatFlow(float value) = 0;
    virtual float GetMinHeatBlasterHeatFlow() const = 0;
    virtual float GetMaxHeatBlasterHeatFlow() const = 0;

    virtual float GetHeatBlasterRadius() const = 0;
    virtual void SetHeatBlasterRadius(float value) = 0;
    virtual float GetMinHeatBlasterRadius() const = 0;
    virtual float GetMaxHeatBlasterRadius() const = 0;

    // Misc

    virtual float GetSeaDepth() const = 0;
    virtual void SetSeaDepth(float value) = 0;
    virtual float GetMinSeaDepth() const = 0;
    virtual float GetMaxSeaDepth() const = 0;

    virtual float GetOceanFloorBumpiness() const = 0;
    virtual void SetOceanFloorBumpiness(float value) = 0;
    virtual float GetMinOceanFloorBumpiness() const = 0;
    virtual float GetMaxOceanFloorBumpiness() const = 0;

    virtual float GetOceanFloorDetailAmplification() const = 0;
    virtual void SetOceanFloorDetailAmplification(float value) = 0;
    virtual float GetMinOceanFloorDetailAmplification() const = 0;
    virtual float GetMaxOceanFloorDetailAmplification() const = 0;

    virtual float GetDestroyRadius() const = 0;
    virtual void SetDestroyRadius(float value) = 0;
    virtual float GetMinDestroyRadius() const = 0;
    virtual float GetMaxDestroyRadius() const = 0;

    virtual float GetRepairRadius() const = 0;
    virtual void SetRepairRadius(float value) = 0;
    virtual float GetMinRepairRadius() const = 0;
    virtual float GetMaxRepairRadius() const = 0;

    virtual float GetRepairSpeedAdjustment() const = 0;
    virtual void SetRepairSpeedAdjustment(float value) = 0;
    virtual float GetMinRepairSpeedAdjustment() const = 0;
    virtual float GetMaxRepairSpeedAdjustment() const = 0;

    virtual float GetBombBlastRadius() const = 0;
    virtual void SetBombBlastRadius(float value) = 0;
    virtual float GetMinBombBlastRadius() const = 0;
    virtual float GetMaxBombBlastRadius() const = 0;

    virtual float GetBombBlastHeat() const = 0;
    virtual void SetBombBlastHeat(float value) = 0;
    virtual float GetMinBombBlastHeat() const = 0;
    virtual float GetMaxBombBlastHeat() const = 0;

    virtual float GetAntiMatterBombImplosionStrength() const = 0;
    virtual void SetAntiMatterBombImplosionStrength(float value) = 0;
    virtual float GetMinAntiMatterBombImplosionStrength() const = 0;
    virtual float GetMaxAntiMatterBombImplosionStrength() const = 0;

    virtual float GetFloodRadius() const = 0;
    virtual void SetFloodRadius(float value) = 0;
    virtual float GetMinFloodRadius() const = 0;
    virtual float GetMaxFloodRadius() const = 0;

    virtual float GetFloodQuantity() const = 0;
    virtual void SetFloodQuantity(float value) = 0;
    virtual float GetMinFloodQuantity() const = 0;
    virtual float GetMaxFloodQuantity() const = 0;

    virtual float GetLuminiscenceAdjustment() const = 0;
    virtual void SetLuminiscenceAdjustment(float value) = 0;
    virtual float GetMinLuminiscenceAdjustment() const = 0;
    virtual float GetMaxLuminiscenceAdjustment() const = 0;

    virtual float GetLightSpreadAdjustment() const = 0;
    virtual void SetLightSpreadAdjustment(float value) = 0;
    virtual float GetMinLightSpreadAdjustment() const = 0;
    virtual float GetMaxLightSpreadAdjustment() const = 0;

    virtual bool GetUltraViolentMode() const = 0;
    virtual void SetUltraViolentMode(bool value) = 0;

    virtual bool GetDoGenerateDebris() const = 0;
    virtual void SetDoGenerateDebris(bool value) = 0;

    virtual bool GetDoGenerateSparkles() const = 0;
    virtual void SetDoGenerateSparkles(bool value) = 0;

    virtual bool GetDoGenerateAirBubbles() const = 0;
    virtual void SetDoGenerateAirBubbles(bool value) = 0;

    virtual float GetAirBubblesDensity() const = 0;
    virtual void SetAirBubblesDensity(float value) = 0;
    virtual float GetMinAirBubblesDensity() const = 0;
    virtual float GetMaxAirBubblesDensity() const = 0;

    virtual size_t GetNumberOfStars() const = 0;
    virtual void SetNumberOfStars(size_t value) = 0;
    virtual size_t GetMinNumberOfStars() const = 0;
    virtual size_t GetMaxNumberOfStars() const = 0;

    virtual size_t GetNumberOfClouds() const = 0;
    virtual void SetNumberOfClouds(size_t value) = 0;
    virtual size_t GetMinNumberOfClouds() const = 0;
    virtual size_t GetMaxNumberOfClouds() const = 0;

    //
    // Render parameters
    //

    virtual rgbColor const & GetFlatSkyColor() const = 0;
    virtual void SetFlatSkyColor(rgbColor const & color) = 0;

    virtual float GetAmbientLightIntensity() const = 0;
    virtual void SetAmbientLightIntensity(float value) = 0;

    virtual float GetWaterContrast() const = 0;
    virtual void SetWaterContrast(float value) = 0;

    virtual float GetOceanTransparency() const = 0;
    virtual void SetOceanTransparency(float value) = 0;

    virtual float GetOceanDarkeningRate() const = 0;
    virtual void SetOceanDarkeningRate(float value) = 0;

    virtual bool GetShowShipThroughOcean() const = 0;
    virtual void SetShowShipThroughOcean(bool value) = 0;

    virtual float GetWaterLevelOfDetail() const = 0;
    virtual void SetWaterLevelOfDetail(float value) = 0;
    virtual float GetMinWaterLevelOfDetail() const = 0;
    virtual float GetMaxWaterLevelOfDetail() const = 0;

    virtual ShipRenderMode GetShipRenderMode() const = 0;
    virtual void SetShipRenderMode(ShipRenderMode shipRenderMode) = 0;

    virtual DebugShipRenderMode GetDebugShipRenderMode() const = 0;
    virtual void SetDebugShipRenderMode(DebugShipRenderMode debugShipRenderMode) = 0;

    virtual OceanRenderMode GetOceanRenderMode() const = 0;
    virtual void SetOceanRenderMode(OceanRenderMode oceanRenderMode) = 0;

    virtual std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const = 0;
    virtual size_t GetTextureOceanTextureIndex() const = 0;
    virtual void SetTextureOceanTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetDepthOceanColorStart() const = 0;
    virtual void SetDepthOceanColorStart(rgbColor const & color) = 0;

    virtual rgbColor const & GetDepthOceanColorEnd() const = 0;
    virtual void SetDepthOceanColorEnd(rgbColor const & color) = 0;

    virtual rgbColor const & GetFlatOceanColor() const = 0;
    virtual void SetFlatOceanColor(rgbColor const & color) = 0;

    virtual LandRenderMode GetLandRenderMode() const = 0;
    virtual void SetLandRenderMode(LandRenderMode landRenderMode) = 0;

    virtual std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const = 0;
    virtual size_t GetTextureLandTextureIndex() const = 0;
    virtual void SetTextureLandTextureIndex(size_t index) = 0;

    virtual rgbColor const & GetFlatLandColor() const = 0;
    virtual void SetFlatLandColor(rgbColor const & color) = 0;

    virtual VectorFieldRenderMode GetVectorFieldRenderMode() const = 0;
    virtual void SetVectorFieldRenderMode(VectorFieldRenderMode VectorFieldRenderMode) = 0;

    virtual bool GetShowShipStress() const = 0;
    virtual void SetShowShipStress(bool value) = 0;

    virtual bool GetDrawHeatOverlay() const = 0;
    virtual void SetDrawHeatOverlay(bool value) = 0;

    virtual float GetHeatOverlayTransparency() const = 0;
    virtual void SetHeatOverlayTransparency(float value) = 0;

    virtual ShipFlameRenderMode GetShipFlameRenderMode() const = 0;
    virtual void SetShipFlameRenderMode(ShipFlameRenderMode shipFlameRenderMode) = 0;

    virtual float GetShipFlameSizeAdjustment() const = 0;
    virtual void SetShipFlameSizeAdjustment(float value) = 0;
    virtual float GetMinShipFlameSizeAdjustment() const = 0;
    virtual float GetMaxShipFlameSizeAdjustment() const = 0;

    //
    // Interaction parameters
    //

    virtual bool GetShowTsunamiNotifications() const = 0;
    virtual void SetShowTsunamiNotifications(bool value) = 0;

    virtual bool GetDrawHeatBlasterFlame() const = 0;
    virtual void SetDrawHeatBlasterFlame(bool value) = 0;
};
