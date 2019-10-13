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
    
    // TODOHERE

    ADD_GC_SETTING(float, LuminiscenceAdjustment);

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
