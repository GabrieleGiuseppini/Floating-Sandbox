/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SoundController.h"

#include <Game/IGameController.h>

#include <GameCore/Settings.h>

enum class GameSettings : size_t
{
    NumMechanicalDynamicsIterationsAdjustment = 0,
    SpringStiffnessAdjustment,
    SpringDampingAdjustment,
    SpringStrengthAdjustment,
    RotAcceler8r,
    WaterDensityAdjustment,
    WaterDragAdjustment,
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
    ElectricalElementHeatProducedAdjustment,

    // Misc

    // TODOHERE

    LuminiscenceAdjustment,
    LightSpreadAdjustment,

    _Last = LightSpreadAdjustment
};

class SettingsManager final : public BaseSettingsManager<GameSettings>
{
public:

    SettingsManager(
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController,
        std::filesystem::path const & rootSystemSettingsDirectoryPath,
        std::filesystem::path const & rootUserSettingsDirectoryPath);

private:

    static BaseSettingsManagerFactory MakeSettingsFactory(
        std::shared_ptr<IGameController> gameController,
        std::shared_ptr<SoundController> soundController);
};