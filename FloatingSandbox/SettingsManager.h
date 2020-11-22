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

    // Water
    WaterDensityAdjustment,
    WaterFrictionDragAdjustment,
    WaterPressureDragAdjustment,
    WaterIntakeAdjustment,
    WaterCrazyness,
    WaterDiffusionSpeedAdjustment,

    BasalWaveHeightAdjustment,
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
    MaxBurningParticles,
    ThermalConductivityAdjustment,
    HeatDissipationAdjustment,
    IgnitionTemperatureAdjustment,
    MeltingTemperatureAdjustment,
    CombustionSpeedAdjustment,
    CombustionHeatAdjustment,
    HeatBlasterHeatFlow,
    HeatBlasterRadius,

    // Electricals
    LuminiscenceAdjustment,
    LightSpreadAdjustment,
    ElectricalElementHeatProducedAdjustment,
    EngineThrustAdjustment,
    WaterPumpPowerAdjustment,

    // Fishes
    NumberOfFishes,
    FishSizeMultiplier,
    FishSpeedAdjustment,
    DoFishShoaling,
    FishShoalRadiusAdjustment,

    // Misc
    OceanFloorTerrain,
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
    UltraViolentMode,
    DoGenerateDebris,
    SmokeEmissionDensityAdjustment,
    SmokeParticleLifetimeAdjustment,
    DoGenerateSparklesForCuts,
    DoGenerateAirBubbles,
    AirBubblesDensity,
    DoDisplaceOceanSurfaceAtAirBubblesSurfacing,
    DoGenerateEngineWakeParticles,
    NumberOfStars,
    NumberOfClouds,
    DoDayLightCycle,
    DayLightCycleDuration,

    // Render
    FlatSkyColor,
    OceanTransparency,
    OceanDarkeningRate,
    FlatLampLightColor,
    DefaultWaterColor,
    WaterContrast,
    WaterLevelOfDetail,
    ShowShipThroughOcean,
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
    ShowShipFrontiers,
    ShowAABBs,
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
    PlayAirBubbleSurfaceSound,

    _Last = PlayAirBubbleSurfaceSound
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