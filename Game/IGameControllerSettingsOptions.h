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

	virtual float GetMinBasalWaveHeightAdjustment() const = 0;
	virtual float GetMaxBasalWaveHeightAdjustment() const = 0;

	virtual float GetMinBasalWaveLengthAdjustment() const = 0;
	virtual float GetMaxBasalWaveLengthAdjustment() const = 0;

	virtual float GetMinBasalWaveSpeedAdjustment() const = 0;
	virtual float GetMaxBasalWaveSpeedAdjustment() const = 0;

	virtual float GetMinTsunamiRate() const = 0;
	virtual float GetMaxTsunamiRate() const = 0;

	virtual float GetMinRogueWaveRate() const = 0;
	virtual float GetMaxRogueWaveRate() const = 0;

	virtual float GetMinWindSpeedBase() const = 0;
	virtual float GetMaxWindSpeedBase() const = 0;

	virtual float GetMinWindSpeedMaxFactor() const = 0;
	virtual float GetMaxWindSpeedMaxFactor() const = 0;

	// Storm

	virtual float GetMinStormMaxWindSpeed() const = 0;
	virtual float GetMaxStormMaxWindSpeed() const = 0;

	virtual float GetMinLightningRate() const = 0;
	virtual float GetMaxLightningRate() const = 0;

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

	virtual float GetMinDestroyRadius() const = 0;
	virtual float GetMaxDestroyRadius() const = 0;

	virtual float GetMinRepairRadius() const = 0;
	virtual float GetMaxRepairRadius() const = 0;

	virtual float GetMinRepairSpeedAdjustment() const = 0;
	virtual float GetMaxRepairSpeedAdjustment() const = 0;

	virtual float GetMinBombBlastRadius() const = 0;
	virtual float GetMaxBombBlastRadius() const = 0;

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
