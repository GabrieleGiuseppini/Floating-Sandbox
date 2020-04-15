/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/IGameControllerSettings.h>

#include <GameCore/Settings.h>

enum class GameSettings : size_t
{
    NumMechanicalDynamicsIterationsAdjustment = 0,
    SpringStiffnessAdjustment,
    SpringDampingAdjustment,
    SpringStrengthAdjustment,
    GlobalDampingAdjustment,
    RotAcceler8r,
    WaterDensityAdjustment,
    WaterDragAdjustment,
    WaterIntakeAdjustment,
    WaterCrazyness,
    WaterDiffusionSpeedAdjustment,

    BasalWaveHeightAdjustment,	// 10
    BasalWaveLengthAdjustment,
    BasalWaveSpeedAdjustment,
    TsunamiRate,
    RogueWaveRate,
    DoModulateWind,
    WindSpeedBase,
    WindSpeedMaxFactor,

	// Storm
	StormRate,
	StormDuration,
	StormStrengthAdjustment,
	DoRainWithStorm,
    RainFloodAdjustment,

    // Heat
    AirTemperature,
    WaterTemperature,
    MaxBurningParticles, // 20
    ThermalConductivityAdjustment,
    HeatDissipationAdjustment,
    IgnitionTemperatureAdjustment,
    MeltingTemperatureAdjustment,
    CombustionSpeedAdjustment,
    CombustionHeatAdjustment,
    HeatBlasterHeatFlow,
    HeatBlasterRadius,
    ElectricalElementHeatProducedAdjustment,

    // Misc
    OceanFloorTerrain, // 30
    SeaDepth,
    OceanFloorBumpiness,
    OceanFloorDetailAmplification,
    OceanFloorElasticity,
    OceanFloorFriction,
    DestroyRadius,
    RepairRadius,
    RepairSpeedAdjustment,
    BombBlastRadius,
    BombBlastHeat,
    AntiMatterBombImplosionStrength,
    FloodRadius,
    FloodQuantity,
    LuminiscenceAdjustment,
    LightSpreadAdjustment,
    UltraViolentMode,
    DoGenerateDebris,
    SmokeEmissionDensityAdjustment,
    SmokeParticleLifetimeAdjustment,
    DoGenerateSparklesForCuts,
    DoGenerateAirBubbles,
    AirBubblesDensity,
    DoGenerateEngineWakeParticles,
    NumberOfStars,
    NumberOfClouds,
    EngineThrustAdjustment,
    WaterPumpPowerAdjustment,

    // Render
    FlatSkyColor,
    OceanTransparency,
    OceanDarkeningRate,
    FlatLampLightColor,
    DefaultWaterColor,
    WaterContrast,
    WaterLevelOfDetail,
    ShowShipThroughOcean,
    ShipRenderMode,
    DebugShipRenderMode,
    OceanRenderMode,
    TextureOceanTextureIndex,
    DepthOceanColorStart,
    DepthOceanColorEnd,
    FlatOceanColor,
    LandRenderMode,
    TextureLandTextureIndex,
    FlatLandColor,
    VectorFieldRenderMode,
    ShowShipStress,
    DrawHeatOverlay,
    HeatOverlayTransparency,
    ShipFlameRenderMode,
    ShipFlameSizeAdjustment,
	DrawHeatBlasterFlame,

    // Sound
    MasterEffectsVolume,
    MasterToolsVolume,
    PlayBreakSounds,
    PlayStressSounds,
    PlayWindSound,

    _Last = PlayWindSound
};

class SettingsManager final : public BaseSettingsManager<GameSettings>
{
public:

    SettingsManager(
        std::shared_ptr<IGameControllerSettings> gameControllerSettings,
        std::shared_ptr<SoundController> soundController,
        std::filesystem::path const & rootSystemSettingsDirectoryPath,
        std::filesystem::path const & rootUserSettingsDirectoryPath);

private:

    static BaseSettingsManagerFactory MakeSettingsFactory(
        std::shared_ptr<IGameControllerSettings> gameControllerSettings,
        std::shared_ptr<SoundController> soundController);
};