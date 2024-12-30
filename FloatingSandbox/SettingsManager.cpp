/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SettingsManager.h"

#include <cctype>
#include <sstream>

std::string MangleSettingName(std::string && settingName);

#define ADD_GC_SETTING(type, name)                      \
    factory.AddSetting<type>(                           \
        GameSettings::name,                             \
        MangleSettingName(#name),                       \
        [&gameControllerSettings]() -> type { return gameControllerSettings.Get##name(); }, \
        [&gameControllerSettings](auto const & v) { gameControllerSettings.Set##name(v); }, \
        [&gameControllerSettings](auto const & v) { gameControllerSettings.Set##name(v); });

#define ADD_GC_SETTING_WITH_IMMEDIATE(type, name)       \
    factory.AddSetting<type>(                           \
        GameSettings::name,                             \
        MangleSettingName(#name),                       \
        [&gameControllerSettings]() -> type { return gameControllerSettings.Get##name(); }, \
        [&gameControllerSettings](auto const & v) { gameControllerSettings.Set##name(v); }, \
        [&gameControllerSettings](auto const & v) { gameControllerSettings.Set##name ## Immediate(v); });

#define ADD_SC_SETTING(type, name)                      \
    factory.AddSetting<type>(                           \
        GameSettings::name,                             \
        MangleSettingName(#name),                       \
        [&soundController]() -> type { return soundController.Get##name(); },	\
        [&soundController](auto const & v) { soundController.Set##name(v); },	\
        [&soundController](auto const & v) { soundController.Set##name(v); });

BaseSettingsManager<GameSettings>::BaseSettingsManagerFactory SettingsManager::MakeSettingsFactory(
    IGameControllerSettings & gameControllerSettings,
    SoundController & soundController)
{
    BaseSettingsManagerFactory factory;

    ADD_GC_SETTING(unsigned int, MaxNumSimulationThreads);
    ADD_GC_SETTING(float, NumMechanicalDynamicsIterationsAdjustment);
    ADD_GC_SETTING(float, SpringStiffnessAdjustment);
    ADD_GC_SETTING(float, SpringDampingAdjustment);
    ADD_GC_SETTING(float, SpringStrengthAdjustment);
    ADD_GC_SETTING(float, GlobalDampingAdjustment);
    ADD_GC_SETTING(float, ElasticityAdjustment);
    ADD_GC_SETTING(float, StaticFrictionAdjustment);
    ADD_GC_SETTING(float, KineticFrictionAdjustment);
    ADD_GC_SETTING(float, RotAcceler8r);
    ADD_GC_SETTING(float, StaticPressureForceAdjustment);

    // Air
    ADD_GC_SETTING(float, AirDensityAdjustment);
    ADD_GC_SETTING(float, AirFrictionDragAdjustment);
    ADD_GC_SETTING(float, AirPressureDragAdjustment);

    // Water
    ADD_GC_SETTING(float, WaterDensityAdjustment);
    ADD_GC_SETTING(float, WaterFrictionDragAdjustment);
    ADD_GC_SETTING(float, WaterPressureDragAdjustment);
    ADD_GC_SETTING(float, WaterImpactForceAdjustment);
    ADD_GC_SETTING(float, HydrostaticPressureCounterbalanceAdjustment);
    ADD_GC_SETTING(float, WaterIntakeAdjustment);
    ADD_GC_SETTING(float, WaterDiffusionSpeedAdjustment);
    ADD_GC_SETTING(float, WaterCrazyness);
    ADD_GC_SETTING(bool, DoDisplaceWater);
    ADD_GC_SETTING(float, WaterDisplacementWaveHeightAdjustment);

    // Waves
    ADD_GC_SETTING(float, BasalWaveHeightAdjustment);
    ADD_GC_SETTING(float, BasalWaveLengthAdjustment);
    ADD_GC_SETTING(float, BasalWaveSpeedAdjustment);
    ADD_GC_SETTING(std::chrono::minutes, TsunamiRate);
    ADD_GC_SETTING(std::chrono::seconds, RogueWaveRate);
    ADD_GC_SETTING(bool, DoModulateWind);
    ADD_GC_SETTING(float, WindSpeedBase);
    ADD_GC_SETTING(float, WindSpeedMaxFactor);
    ADD_GC_SETTING(float, WaveSmoothnessAdjustment);

    // Storm
    ADD_GC_SETTING(std::chrono::minutes, StormRate);
    ADD_GC_SETTING(std::chrono::seconds, StormDuration);
    ADD_GC_SETTING(float, StormStrengthAdjustment);
    ADD_GC_SETTING(bool, DoRainWithStorm);
    ADD_GC_SETTING(float, RainFloodAdjustment);
    ADD_GC_SETTING(float, LightningBlastProbability);

    // Heat
    ADD_GC_SETTING(float, AirTemperature);
    ADD_GC_SETTING(float, WaterTemperature);
    ADD_GC_SETTING(unsigned int, MaxBurningParticlesPerShip);
    ADD_GC_SETTING(float, ThermalConductivityAdjustment);
    ADD_GC_SETTING(float, HeatDissipationAdjustment);
    ADD_GC_SETTING(float, IgnitionTemperatureAdjustment);
    ADD_GC_SETTING(float, MeltingTemperatureAdjustment);
    ADD_GC_SETTING(float, CombustionSpeedAdjustment);
    ADD_GC_SETTING(float, CombustionHeatAdjustment);
    ADD_GC_SETTING(float, HeatBlasterHeatFlow);
    ADD_GC_SETTING(float, HeatBlasterRadius);
    ADD_GC_SETTING(float, LaserRayHeatFlow);

    // Electricals
    ADD_GC_SETTING(float, LuminiscenceAdjustment);
    ADD_GC_SETTING(float, LightSpreadAdjustment);
    ADD_GC_SETTING(float, ElectricalElementHeatProducedAdjustment);
    ADD_GC_SETTING(float, EngineThrustAdjustment);
    ADD_GC_SETTING(float, WaterPumpPowerAdjustment);

    // Fishes
    ADD_GC_SETTING(unsigned int, NumberOfFishes);
    ADD_GC_SETTING(float, FishSizeMultiplier);
    ADD_GC_SETTING(float, FishSpeedAdjustment);
    ADD_GC_SETTING(bool, DoFishShoaling);
    ADD_GC_SETTING(float, FishShoalRadiusAdjustment);

    // NPCs
    ADD_GC_SETTING(float, NpcFrictionAdjustment);
    ADD_GC_SETTING(float, NpcSizeMultiplier);
    ADD_GC_SETTING(float, NpcPassiveBlastRadiusAdjustment);

    // Misc
    ADD_GC_SETTING(OceanFloorTerrain, OceanFloorTerrain);
    ADD_GC_SETTING_WITH_IMMEDIATE(float, SeaDepth);
    ADD_GC_SETTING(float, OceanFloorBumpiness);
    ADD_GC_SETTING_WITH_IMMEDIATE(float, OceanFloorDetailAmplification);
    ADD_GC_SETTING(float, OceanFloorElasticityCoefficient);
    ADD_GC_SETTING(float, OceanFloorFrictionCoefficient);
    ADD_GC_SETTING(float, OceanFloorSiltHardness);
    ADD_GC_SETTING(float, DestroyRadius);
    ADD_GC_SETTING(float, RepairRadius);
    ADD_GC_SETTING(float, RepairSpeedAdjustment);
    ADD_GC_SETTING(bool, DoApplyPhysicsToolsToShips);
    ADD_GC_SETTING(bool, DoApplyPhysicsToolsToNpcs);
    ADD_GC_SETTING(float, BombBlastRadius);
    ADD_GC_SETTING(float, BombBlastForceAdjustment);
    ADD_GC_SETTING(float, BombBlastHeat);
    ADD_GC_SETTING(float, AntiMatterBombImplosionStrength);
    ADD_GC_SETTING(float, FloodRadius);
    ADD_GC_SETTING(float, FloodQuantity);
    ADD_GC_SETTING(float, InjectPressureQuantity);
    ADD_GC_SETTING(float, BlastToolRadius);
    ADD_GC_SETTING(float, BlastToolForceAdjustment);
    ADD_GC_SETTING(float, ScrubRotToolRadius);
    ADD_GC_SETTING(float, WindMakerToolWindSpeed);
    ADD_GC_SETTING(bool, UltraViolentMode);
    ADD_GC_SETTING(bool, DoGenerateDebris);
    ADD_GC_SETTING(float, SmokeEmissionDensityAdjustment);
    ADD_GC_SETTING(float, SmokeParticleLifetimeAdjustment);
    ADD_GC_SETTING(bool, DoGenerateSparklesForCuts);
    ADD_GC_SETTING(float, AirBubblesDensity);
    ADD_GC_SETTING(bool, DoGenerateEngineWakeParticles);
    ADD_GC_SETTING(unsigned int, NumberOfStars);
    ADD_GC_SETTING(unsigned int, NumberOfClouds);
    ADD_GC_SETTING(bool, DoDayLightCycle);
    ADD_GC_SETTING(std::chrono::minutes, DayLightCycleDuration);
    ADD_GC_SETTING(float, ShipStrengthRandomizationDensityAdjustment);
    ADD_GC_SETTING(float, ShipStrengthRandomizationExtent);

    // Render
    ADD_GC_SETTING(rgbColor, FlatSkyColor);
    ADD_GC_SETTING(bool, DoMoonlight);
    ADD_GC_SETTING(rgbColor, MoonlightColor);
    ADD_GC_SETTING(bool, DoCrepuscularGradient);
    ADD_GC_SETTING(rgbColor, CrepuscularColor);
    ADD_GC_SETTING(CloudRenderDetailType, CloudRenderDetail);
    ADD_GC_SETTING(float, OceanTransparency);
    ADD_GC_SETTING(float, OceanDepthDarkeningRate);
    ADD_GC_SETTING(float, ShipAmbientLightSensitivity);
    ADD_GC_SETTING(float, ShipDepthDarkeningSensitivity);
    ADD_GC_SETTING(rgbColor, FlatLampLightColor);
    ADD_GC_SETTING(rgbColor, DefaultWaterColor);
    ADD_GC_SETTING(float, WaterContrast);
    ADD_GC_SETTING(float, WaterLevelOfDetail);
    ADD_GC_SETTING(bool, ShowShipThroughOcean);
    ADD_GC_SETTING(DebugShipRenderModeType, DebugShipRenderMode);
    ADD_GC_SETTING(OceanRenderModeType, OceanRenderMode);
    ADD_GC_SETTING(size_t, TextureOceanTextureIndex);
    ADD_GC_SETTING(rgbColor, DepthOceanColorStart);
    ADD_GC_SETTING(rgbColor, DepthOceanColorEnd);
    ADD_GC_SETTING(rgbColor, FlatOceanColor);
    ADD_GC_SETTING(OceanRenderDetailType, OceanRenderDetail);
    ADD_GC_SETTING(LandRenderModeType, LandRenderMode);
    ADD_GC_SETTING(size_t, TextureLandTextureIndex);
    ADD_GC_SETTING(rgbColor, FlatLandColor);
    ADD_GC_SETTING(LandRenderDetailType, LandRenderDetail);
    ADD_GC_SETTING(NpcRenderModeType, NpcRenderMode);
    ADD_GC_SETTING(rgbColor, NpcQuadFlatColor);
    ADD_GC_SETTING(VectorFieldRenderModeType, VectorFieldRenderMode);
    ADD_GC_SETTING(bool, ShowShipStress);
    ADD_GC_SETTING(bool, ShowShipFrontiers);
    ADD_GC_SETTING(bool, ShowAABBs);
    ADD_GC_SETTING(HeatRenderModeType, HeatRenderMode);
    ADD_GC_SETTING(float, HeatSensitivity);
    ADD_GC_SETTING(StressRenderModeType, StressRenderMode);
    ADD_GC_SETTING(bool, DrawExplosions);
    ADD_GC_SETTING(bool, DrawFlames);
    ADD_GC_SETTING(float, ShipFlameSizeAdjustment);
    ADD_GC_SETTING(float, ShipFlameKaosAdjustment);
    ADD_GC_SETTING(bool, DrawHeatBlasterFlame);

    // Sound
    ADD_SC_SETTING(float, MasterEffectsVolume);
    ADD_SC_SETTING(float, MasterToolsVolume);
    ADD_SC_SETTING(bool, PlayBreakSounds);
    ADD_SC_SETTING(bool, PlayStressSounds);
    ADD_SC_SETTING(bool, PlayWindSound);
    ADD_SC_SETTING(bool, PlayAirBubbleSurfaceSound);

    return factory;
}

SettingsManager::SettingsManager(
    IGameControllerSettings & gameControllerSettings,
    SoundController & soundController,
    std::filesystem::path const & rootSystemSettingsDirectoryPath,
    std::filesystem::path const & rootUserSettingsDirectoryPath)
    : BaseSettingsManager<GameSettings>(
        MakeSettingsFactory(gameControllerSettings, soundController),
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

std::string MangleSettingName(std::string && settingName)
{
    std::stringstream ss;

    bool isFirst = true;
    for (char ch : settingName)
    {
        if (std::isupper(ch))
        {
            if (!isFirst)
            {
                ss << '_';
            }

            ss << static_cast<char>(std::tolower(ch));
        }
        else
        {
            ss << ch;
        }

        isFirst = false;
    }

    return ss.str();
}