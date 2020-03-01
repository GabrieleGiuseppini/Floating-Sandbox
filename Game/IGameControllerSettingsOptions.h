/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "OceanFloorTerrain.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/UniqueBuffer.h>
#include <GameCore/Vectors.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/*
 * The interface presented by the GameController class to the external projects
 * for inquiring settings' limits and options.
 */
struct IGameControllerSettingsOptions
{
	virtual float GetMinNumMechanicalDynamicsIterationsAdjustment() const = 0;
	virtual float GetMaxNumMechanicalDynamicsIterationsAdjustment() const = 0;

	virtual float GetMinSpringStiffnessAdjustment() const = 0;
	virtual float GetMaxSpringStiffnessAdjustment() const = 0;

	virtual float GetMinSpringDampingAdjustment() const = 0;
	virtual float GetMaxSpringDampingAdjustment() const = 0;

	virtual float GetMinSpringStrengthAdjustment() const = 0;
	virtual float GetMaxSpringStrengthAdjustment() const = 0;

    virtual float GetMinGlobalDampingAdjustment() const = 0;
    virtual float GetMaxGlobalDampingAdjustment() const = 0;

	virtual float GetMinRotAcceler8r() const = 0;
	virtual float GetMaxRotAcceler8r() const = 0;

	virtual float GetMinWaterDensityAdjustment() const = 0;
	virtual float GetMaxWaterDensityAdjustment() const = 0;

	virtual float GetMinWaterDragAdjustment() const = 0;
	virtual float GetMaxWaterDragAdjustment() const = 0;

	virtual float GetMinWaterIntakeAdjustment() const = 0;
	virtual float GetMaxWaterIntakeAdjustment() const = 0;

	virtual float GetMinWaterCrazyness() const = 0;
	virtual float GetMaxWaterCrazyness() const = 0;

	virtual float GetMinWaterDiffusionSpeedAdjustment() const = 0;
	virtual float GetMaxWaterDiffusionSpeedAdjustment() const = 0;

    virtual float GetMinSmokeEmissionDensityAdjustment() const = 0;
    virtual float GetMaxSmokeEmissionDensityAdjustment() const = 0;

    virtual float GetMinSmokeParticleLifetimeAdjustment() const = 0;
    virtual float GetMaxSmokeParticleLifetimeAdjustment() const = 0;

	virtual float GetMinBasalWaveHeightAdjustment() const = 0;
	virtual float GetMaxBasalWaveHeightAdjustment() const = 0;

	virtual float GetMinBasalWaveLengthAdjustment() const = 0;
	virtual float GetMaxBasalWaveLengthAdjustment() const = 0;

	virtual float GetMinBasalWaveSpeedAdjustment() const = 0;
	virtual float GetMaxBasalWaveSpeedAdjustment() const = 0;

	virtual std::chrono::minutes GetMinTsunamiRate() const = 0;
	virtual std::chrono::minutes GetMaxTsunamiRate() const = 0;

	virtual std::chrono::minutes GetMinRogueWaveRate() const = 0;
	virtual std::chrono::minutes GetMaxRogueWaveRate() const = 0;

	virtual float GetMinWindSpeedBase() const = 0;
	virtual float GetMaxWindSpeedBase() const = 0;

	virtual float GetMinWindSpeedMaxFactor() const = 0;
	virtual float GetMaxWindSpeedMaxFactor() const = 0;

	// Storm

	virtual std::chrono::minutes GetMinStormRate() const = 0;
	virtual std::chrono::minutes GetMaxStormRate() const = 0;

	virtual std::chrono::seconds GetMinStormDuration() const = 0;
	virtual std::chrono::seconds GetMaxStormDuration() const = 0;

	virtual float GetMinStormStrengthAdjustment() const = 0;
	virtual float GetMaxStormStrengthAdjustment() const = 0;

    virtual float GetMinRainFloodAdjustment() const = 0;
    virtual float GetMaxRainFloodAdjustment() const = 0;

	// Heat

	virtual float GetMinAirTemperature() const = 0;
	virtual float GetMaxAirTemperature() const = 0;

	virtual float GetMinWaterTemperature() const = 0;
	virtual float GetMaxWaterTemperature() const = 0;

	virtual unsigned int GetMinMaxBurningParticles() const = 0;
	virtual unsigned int GetMaxMaxBurningParticles() const = 0;

	virtual float GetMinThermalConductivityAdjustment() const = 0;
	virtual float GetMaxThermalConductivityAdjustment() const = 0;

	virtual float GetMinHeatDissipationAdjustment() const = 0;
	virtual float GetMaxHeatDissipationAdjustment() const = 0;

	virtual float GetMinIgnitionTemperatureAdjustment() const = 0;
	virtual float GetMaxIgnitionTemperatureAdjustment() const = 0;

	virtual float GetMinMeltingTemperatureAdjustment() const = 0;
	virtual float GetMaxMeltingTemperatureAdjustment() const = 0;

	virtual float GetMinCombustionSpeedAdjustment() const = 0;
	virtual float GetMaxCombustionSpeedAdjustment() const = 0;

	virtual float GetMinCombustionHeatAdjustment() const = 0;
	virtual float GetMaxCombustionHeatAdjustment() const = 0;

	virtual float GetMinHeatBlasterHeatFlow() const = 0;
	virtual float GetMaxHeatBlasterHeatFlow() const = 0;

	virtual float GetMinHeatBlasterRadius() const = 0;
	virtual float GetMaxHeatBlasterRadius() const = 0;

	virtual float GetMinElectricalElementHeatProducedAdjustment() const = 0;
	virtual float GetMaxElectricalElementHeatProducedAdjustment() const = 0;

	// Misc

	virtual float GetMinSeaDepth() const = 0;
	virtual float GetMaxSeaDepth() const = 0;

	virtual float GetMinOceanFloorBumpiness() const = 0;
	virtual float GetMaxOceanFloorBumpiness() const = 0;

	virtual float GetMinOceanFloorDetailAmplification() const = 0;
	virtual float GetMaxOceanFloorDetailAmplification() const = 0;

    virtual float GetMinOceanFloorElasticity() const = 0;
    virtual float GetMaxOceanFloorElasticity() const = 0;

    virtual float GetMinOceanFloorFriction() const = 0;
    virtual float GetMaxOceanFloorFriction() const = 0;

	virtual float GetMinDestroyRadius() const = 0;
	virtual float GetMaxDestroyRadius() const = 0;

	virtual float GetMinRepairRadius() const = 0;
	virtual float GetMaxRepairRadius() const = 0;

	virtual float GetMinRepairSpeedAdjustment() const = 0;
	virtual float GetMaxRepairSpeedAdjustment() const = 0;

	virtual float GetMinBombBlastRadius() const = 0;
	virtual float GetMaxBombBlastRadius() const = 0;

    virtual float GetMinBombBlastForceAdjustment() const = 0;
    virtual float GetMaxBombBlastForceAdjustment() const = 0;

	virtual float GetMinBombBlastHeat() const = 0;
	virtual float GetMaxBombBlastHeat() const = 0;

	virtual float GetMinAntiMatterBombImplosionStrength() const = 0;
	virtual float GetMaxAntiMatterBombImplosionStrength() const = 0;

	virtual float GetMinFloodRadius() const = 0;
	virtual float GetMaxFloodRadius() const = 0;

	virtual float GetMinFloodQuantity() const = 0;
	virtual float GetMaxFloodQuantity() const = 0;

	virtual float GetMinLuminiscenceAdjustment() const = 0;
	virtual float GetMaxLuminiscenceAdjustment() const = 0;

	virtual float GetMinLightSpreadAdjustment() const = 0;
	virtual float GetMaxLightSpreadAdjustment() const = 0;

	virtual float GetMinAirBubblesDensity() const = 0;
	virtual float GetMaxAirBubblesDensity() const = 0;

	virtual unsigned int GetMinNumberOfStars() const = 0;
	virtual unsigned int GetMaxNumberOfStars() const = 0;

	virtual unsigned int GetMinNumberOfClouds() const = 0;
	virtual unsigned int GetMaxNumberOfClouds() const = 0;

    virtual float GetMinEngineThrustAdjustment() const = 0;
    virtual float GetMaxEngineThrustAdjustment() const = 0;

	//
	// Render parameters
	//

	virtual float GetMinWaterLevelOfDetail() const = 0;
	virtual float GetMaxWaterLevelOfDetail() const = 0;

	virtual std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const = 0;

	virtual std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const = 0;

	virtual float GetMinShipFlameSizeAdjustment() const = 0;
	virtual float GetMaxShipFlameSizeAdjustment() const = 0;
};
