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
    
    // TODOHERE

    ADD_GC_SETTING(float, LuminiscenceAdjustment);
    ADD_GC_SETTING(float, LightSpreadAdjustment);
    

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
void Setting<OceanFloorTerrain>::Serialize(SettingsSerializationContext & context) const
{
    auto os = context.GetNamedStream(GetName(), "bin");

    mValue.SaveToStream(*os);
}

template<>
void Setting<OceanFloorTerrain>::Deserialize(SettingsDeserializationContext const & context)
{
    auto const is = context.GetNamedStream(GetName(), "bin");

    auto terrain = OceanFloorTerrain::LoadFromStream(*is);

    SetValue(std::move(terrain));
}
