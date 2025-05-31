/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderParameters.h"

RenderParameters::RenderParameters(
	FloatSize const & maxWorldSize,
	DisplayLogicalSize const & initialCanvasSize,
	int logicalToPhysicalDisplayFactor)
	: View(maxWorldSize, 1.0f, vec2f::zero(), initialCanvasSize, logicalToPhysicalDisplayFactor)
	, EffectiveAmbientLightIntensity(1.0f) // Calculated
	// World
	, FlatSkyColor(0x39, 0xa8, 0xf2)
	, EffectiveMoonlightColor(0x00, 0x00, 0x00) // Calculated
	, DoCrepuscularGradient(true)
	, CrepuscularColor (0xe5, 0xd3, 0xe5)
	, CloudRenderDetail(CloudRenderDetailType::Detailed)
	, OceanTransparency(0.594f)
	, OceanDepthDarkeningRate(0.126745f)
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
	, LandRenderDetail(LandRenderDetailType::Detailed)
	// Ship
	, ShipViewMode(ShipViewModeType::Exterior)
	, ShipAmbientLightSensitivity(1.0f)
	, ShipDepthDarkeningSensitivity(0.906f)
	, FlatLampLightColor(0xff, 0xff, 0xbf)
	, DrawExplosions(true)
	, DrawFlames(true)
	, ShipFlameKaosAdjustment(0.656f)
	, ShowStressedSprings(false)
	, ShowFrontiers(false)
	, ShowAABBs(false)
	, ShipWaterColor(vec3f::zero()) // Calculated
	, ShipWaterContrast(0.71875f)
	, ShipWaterLevelOfDetail(0.6875f)
	, HeatRenderMode(HeatRenderModeType::Incandescence)
	, HeatSensitivity(0.0f)
	, StressRenderMode(StressRenderModeType::None)
	, ShipParticleRenderMode(ShipParticleRenderModeType::Fragment)
	, DebugShipRenderMode(DebugShipRenderModeType::None)
	, NpcRenderMode(NpcRenderModeType::Texture)
	, NpcQuadFlatColor(143, 201, 242)
	// Misc
	, DisplayUnitsSystem(UnitsSystem::SI_Kelvin)
	// Flags
	, IsViewDirty(true)
	, IsCanvasSizeDirty(true)
	, IsEffectiveAmbientLightIntensityDirty(true)
	, IsSkyDirty(true)
	, IsCloudRenderDetailDirty(true)
	, IsOceanDepthDarkeningRateDirty(true)
	, AreOceanRenderParametersDirty(true)
	, IsOceanTextureIndexDirty(true)
	, AreLandRenderParametersDirty(true)
	, IsLandTextureIndexDirty(true)
	, IsLandRenderDetailDirty(true)
	, IsShipViewModeDirty(true)
	, IsShipAmbientLightSensitivityDirty(true)
	, IsShipDepthDarkeningSensitivityDirty(true)
	, IsFlatLampLightColorDirty(true)
	, AreShipFlameRenderParametersDirty(true)
	, IsShipWaterColorDirty(true)
	, IsShipWaterContrastDirty(true)
	, IsShipWaterLevelOfDetailDirty(true)
	, IsHeatSensitivityDirty(true)
	, AreShipStructureRenderModeSelectorsDirty(true)
	, AreNpcRenderParametersDirty(true)
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
	IsSkyDirty = false;
	IsCloudRenderDetailDirty = false;
	IsOceanDepthDarkeningRateDirty = false;
	AreOceanRenderParametersDirty = false;
	IsOceanTextureIndexDirty = false;
	AreLandRenderParametersDirty = false;
	IsLandTextureIndexDirty = false;
	IsLandRenderDetailDirty = false;
	//
	IsShipViewModeDirty = false;
	IsShipAmbientLightSensitivityDirty = false;
	IsShipDepthDarkeningSensitivityDirty = false;
	IsFlatLampLightColorDirty = false;
	AreShipFlameRenderParametersDirty = false;
	IsShipWaterColorDirty = false;
	IsShipWaterContrastDirty = false;
	IsShipWaterLevelOfDetailDirty = false;
	IsHeatSensitivityDirty = false;
	AreShipStructureRenderModeSelectorsDirty = false;
	AreNpcRenderParametersDirty = false;
	//
	IsDisplayUnitsSystemDirty = false;

	return copy;
}
