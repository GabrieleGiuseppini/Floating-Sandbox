/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SettingsManager.h"

#define ADD_GC_SETTING(type, name)                      \
    factory.AddSetting<type>(                           \
        GameSettings::##name,                           \
        #name,                                          \
        [gameController]() -> type { return gameController->Get##name(); }, \
        [gameController](auto const & v) { gameController->Set##name(v); });

#define ADD_SC_SETTING(type, name)                      \
    factory.AddSetting<type>(                           \
        GameSettings::##name,                           \
        #name,                                          \
        [soundController]() -> type { return soundController->Get##name(); }, \
        [soundController](auto const & v) { soundController->Set##name(v); });

BaseSettingsManager<GameSettings>::BaseSettingsManagerFactory SettingsManager::MakeSettingsFactory(
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController)
{
    BaseSettingsManagerFactory factory;

    ADD_GC_SETTING(float, NumMechanicalDynamicsIterationsAdjustment);
    ADD_GC_SETTING(float, SpringStiffnessAdjustment);
    ADD_GC_SETTING(float, SpringDampingAdjustment);
    ADD_GC_SETTING(float, SpringStrengthAdjustment);
    ADD_GC_SETTING(float, RotAcceler8r);
    ADD_GC_SETTING(float, WaterDensityAdjustment);
    ADD_GC_SETTING(float, WaterDragAdjustment);
    ADD_GC_SETTING(float, WaterIntakeAdjustment);
    ADD_GC_SETTING(float, WaterCrazyness);
    ADD_GC_SETTING(float, WaterDiffusionSpeedAdjustment);    

    ADD_GC_SETTING(float, BasalWaveHeightAdjustment);
    ADD_GC_SETTING(float, BasalWaveLengthAdjustment);
    ADD_GC_SETTING(float, BasalWaveSpeedAdjustment);
    ADD_GC_SETTING(float, TsunamiRate);
    ADD_GC_SETTING(float, RogueWaveRate);
    ADD_GC_SETTING(bool, DoModulateWind);
    ADD_GC_SETTING(float, WindSpeedBase);
    ADD_GC_SETTING(float, WindSpeedMaxFactor);

    // Heat
    ADD_GC_SETTING(float, AirTemperature);
    ADD_GC_SETTING(float, WaterTemperature);
    ADD_GC_SETTING(unsigned int, MaxBurningParticles);
    ADD_GC_SETTING(float, ThermalConductivityAdjustment);
    ADD_GC_SETTING(float, HeatDissipationAdjustment);
    ADD_GC_SETTING(float, IgnitionTemperatureAdjustment);
    ADD_GC_SETTING(float, MeltingTemperatureAdjustment);
    ADD_GC_SETTING(float, CombustionSpeedAdjustment);
    ADD_GC_SETTING(float, CombustionHeatAdjustment);
    ADD_GC_SETTING(float, HeatBlasterHeatFlow);
    ADD_GC_SETTING(float, HeatBlasterRadius);
    ADD_GC_SETTING(float, ElectricalElementHeatProducedAdjustment);

    // Misc
    ADD_GC_SETTING(OceanFloorTerrain, OceanFloorTerrain);
    ADD_GC_SETTING(float, SeaDepth);
    ADD_GC_SETTING(float, OceanFloorBumpiness);
    ADD_GC_SETTING(float, OceanFloorDetailAmplification);
    ADD_GC_SETTING(float, DestroyRadius);
    ADD_GC_SETTING(float, RepairRadius);
    ADD_GC_SETTING(float, RepairSpeedAdjustment);
    ADD_GC_SETTING(float, BombBlastRadius);
    ADD_GC_SETTING(float, BombBlastHeat);
    ADD_GC_SETTING(float, AntiMatterBombImplosionStrength);
    ADD_GC_SETTING(float, FloodRadius);
    ADD_GC_SETTING(float, FloodQuantity);
    ADD_GC_SETTING(float, LuminiscenceAdjustment);
    ADD_GC_SETTING(float, LightSpreadAdjustment);
    ADD_GC_SETTING(bool, UltraViolentMode);
    ADD_GC_SETTING(bool, DoGenerateDebris);
    ADD_GC_SETTING(bool, DoGenerateSparkles);
    ADD_GC_SETTING(bool, DoGenerateAirBubbles);
    ADD_GC_SETTING(float, AirBubblesDensity);
    ADD_GC_SETTING(unsigned int, NumberOfStars);
    ADD_GC_SETTING(unsigned int, NumberOfClouds);

    // Render
    ADD_GC_SETTING(rgbColor, FlatSkyColor);
    ADD_GC_SETTING(float, WaterContrast);
    ADD_GC_SETTING(float, OceanTransparency);
    ADD_GC_SETTING(float, OceanDarkeningRate);
    ADD_GC_SETTING(bool, ShowShipThroughOcean);
    ADD_GC_SETTING(float, WaterLevelOfDetail);
    ADD_GC_SETTING(ShipRenderMode, ShipRenderMode);
    ADD_GC_SETTING(DebugShipRenderMode, DebugShipRenderMode);
    ADD_GC_SETTING(OceanRenderMode, OceanRenderMode);
    ADD_GC_SETTING(size_t, TextureOceanTextureIndex);
    ADD_GC_SETTING(rgbColor, DepthOceanColorStart);
    ADD_GC_SETTING(rgbColor, DepthOceanColorEnd);
    ADD_GC_SETTING(rgbColor, FlatOceanColor);
    ADD_GC_SETTING(LandRenderMode, LandRenderMode);
    ADD_GC_SETTING(size_t, TextureLandTextureIndex);
    ADD_GC_SETTING(rgbColor, FlatLandColor);
    ADD_GC_SETTING(VectorFieldRenderMode, VectorFieldRenderMode);
    ADD_GC_SETTING(bool, ShowShipStress);
    ADD_GC_SETTING(bool, DrawHeatOverlay);
    ADD_GC_SETTING(float, HeatOverlayTransparency);
    ADD_GC_SETTING(ShipFlameRenderMode, ShipFlameRenderMode);
    ADD_GC_SETTING(float, ShipFlameSizeAdjustment);

    // Sound
    ADD_SC_SETTING(float, MasterEffectsVolume);
    ADD_SC_SETTING(float, MasterToolsVolume);
    ADD_SC_SETTING(float, MasterMusicVolume);
    ADD_SC_SETTING(bool, PlayBreakSounds);
    ADD_SC_SETTING(bool, PlayStressSounds);
    ADD_SC_SETTING(bool, PlayWindSound);
    ADD_SC_SETTING(bool, PlaySinkingMusic);
    
    return factory;
}

SettingsManager::SettingsManager(
    std::shared_ptr<IGameController> gameController,
    std::shared_ptr<SoundController> soundController,
    std::filesystem::path const & rootSystemSettingsDirectoryPath,
    std::filesystem::path const & rootUserSettingsDirectoryPath)
    : BaseSettingsManager<GameSettings>(
        MakeSettingsFactory(gameController, soundController),
        rootSystemSettingsDirectoryPath,
        rootUserSettingsDirectoryPath)
{}

//
// Specializations for special settings
//

template<>
void SettingSerializer::Serialize<OceanFloorTerrain>(
    SettingsSerializationContext & context,
    std::string const & settingName,
    OceanFloorTerrain const & value)
{
    auto os = context.GetNamedStream(settingName, "bin");

    value.SaveToStream(*os);
}

template<>
bool SettingSerializer::Deserialize<OceanFloorTerrain>(
    SettingsDeserializationContext const & context,
    std::string const & settingName,
    OceanFloorTerrain & value)
{
    auto const is = context.GetNamedStream(settingName, "bin");
    if (!!is)
    {
        value = OceanFloorTerrain::LoadFromStream(*is);
        return true;
    }

    return false;
}
