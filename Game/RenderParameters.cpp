/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderParameters.h"

namespace Render {

RenderParameters::RenderParameters(
	DisplayLogicalSize const & initialCanvasSize,
	int logicalToPhysicalDisplayFactor)
	: View(1.0f, vec2f::zero(), initialCanvasSize, logicalToPhysicalDisplayFactor)
	, EffectiveAmbientLightIntensity(1.0f) // Calculated
	// World
	, FlatSkyColor(0x00, 0x77, 0xc4)
	, OceanTransparency(0.8125f)
	, OceanDarkeningRate(0.12795731425285339f)
	, OceanRenderMode(OceanRenderModeType::Flat)
	, DepthOceanColorStart(0x4a, 0x84, 0x9f)
	, DepthOceanColorEnd(0x00, 0x00, 0x00)
	, FlatOceanColor(0x00, 0x53, 0x91)
	, OceanTextureIndex(0) // Wavy Clear Thin
	, OceanRenderDetail(OceanRenderDetailType::Detailed)
	, ShowShipThroughOcean(false)
	, LandRenderMode(LandRenderModeType::Texture)
	, FlatLandColor(0x72, 0x46, 0x05)
	, LandTextureIndex(3) // Rock Coarse 3
	// Ship
	, ShipAmbientLightSensitivity(1.0f)
	, FlatLampLightColor(0xff, 0xff, 0xbf)
	, DrawExplosions(true)
	, DrawFlames(true)
	, ShowStressedSprings(false)
	, ShowFrontiers(false)
	, ShowAABBs(false)
	, ShipWaterColor(vec3f::zero()) // Calculated
	, ShipWaterContrast(0.71875f)
	, ShipWaterLevelOfDetail(0.6875f)
	, HeatRenderMode(HeatRenderModeType::Incandescence)
	, HeatSensitivity(0.0f)
	, StressRenderMode(StressRenderModeType::None)
	, DebugShipRenderMode(DebugShipRenderModeType::None)
	// Misc
	, DisplayUnitsSystem(UnitsSystem::SI_Kelvin)
	// Flags
	, IsViewDirty(true)
	, IsCanvasSizeDirty(true)
	, IsEffectiveAmbientLightIntensityDirty(true)
	, IsFlatSkyColorDirty(true)
	, IsOceanDarkeningRateDirty(true)
	, AreOceanRenderParametersDirty(true)
	, IsOceanTextureIndexDirty(true)
	, AreLandRenderParametersDirty(true)
	, IsLandTextureIndexDirty(true)
	, IsShipAmbientLightSensitivityDirty(true)
	, IsFlatLampLightColorDirty(true)
	, IsShipWaterColorDirty(true)
	, IsShipWaterContrastDirty(true)
	, IsShipWaterLevelOfDetailDirty(true)
	, IsHeatSensitivityDirty(true)
	, AreShipStructureRenderModeSelectorsDirty(true)
	, IsDisplayUnitsSystemDirty(true)
{
}

RenderParameters RenderParameters::TakeSnapshotAndClear()
{
	// Make copy
	RenderParameters copy = *this;

	// Clear own 'dirty' flags
	IsViewDirty = false;
	IsCanvasSizeDirty = false;
	IsEffectiveAmbientLightIntensityDirty = false;
	IsFlatSkyColorDirty = false;
	IsOceanDarkeningRateDirty = false;
	AreOceanRenderParametersDirty = false;
	IsOceanTextureIndexDirty = false;
	AreLandRenderParametersDirty = false;
	IsLandTextureIndexDirty = false;
	//
	IsShipAmbientLightSensitivityDirty = false;
	IsFlatLampLightColorDirty = false;
	IsShipWaterColorDirty = false;
	IsShipWaterContrastDirty = false;
	IsShipWaterLevelOfDetailDirty = false;
	IsHeatSensitivityDirty = false;
	AreShipStructureRenderModeSelectorsDirty = false;
	//
	IsDisplayUnitsSystemDirty = false;

	return copy;
}

}