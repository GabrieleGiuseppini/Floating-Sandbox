/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderParameters.h"

namespace Render {

RenderParameters::RenderParameters(
	LogicalPixelSize const & initialCanvasSize,
	int logicalToPhysicalPixelFactor)
	: View(1.0f, vec2f::zero(), initialCanvasSize, logicalToPhysicalPixelFactor)
	, IsViewDirty(true)
	, IsCanvasSizeDirty(true)
	, EffectiveAmbientLightIntensity(1.0f) // Calculated
	, IsEffectiveAmbientLightIntensityDirty(true)
	// World
	, FlatSkyColor(0x87, 0xce, 0xfa) // (cornflower blue)
	, IsFlatSkyColorDirty(true)
	, OceanTransparency(0.8125f)
	, OceanDarkeningRate(0.356993f)
	, IsOceanDarkeningRateDirty(true)
	, OceanRenderMode(OceanRenderModeType::Texture)
	, DepthOceanColorStart(0x4a, 0x84, 0x9f)
	, DepthOceanColorEnd(0x00, 0x00, 0x00)
	, FlatOceanColor(0x7d, 0xe2, 0xee)
	, AreOceanRenderModeParametersDirty(true)
	, OceanTextureIndex(0) // Wavy Clear Thin
	, IsOceanTextureIndexDirty(true)
	, OceanRenderDetail(OceanRenderDetailType::Detailed)
	, ShowShipThroughOcean(false)
	, LandRenderMode(LandRenderModeType::Texture)
	, FlatLandColor(0x72, 0x46, 0x05)
	, AreLandRenderParametersDirty(true)
	, LandTextureIndex(3) // Rock Coarse 3
	, IsLandTextureIndexDirty(true)
	// Ship
	, FlatLampLightColor(0xff, 0xff, 0xbf)
	, IsFlatLampLightColorDirty(true)
	, DrawExplosions(true)
	, DrawFlames(true)
	, ShowStressedSprings(false)
	, ShowFrontiers(false)
	, ShowAABBs(false)
	, ShipWaterColor(vec3f::zero()) // Calculated
	, IsShipWaterColorDirty(true)
	, ShipWaterContrast(0.71875f)
	, IsShipWaterContrastDirty(true)
	, ShipWaterLevelOfDetail(0.6875f)
	, IsShipWaterLevelOfDetailDirty(true)
	, HeatRenderMode(HeatRenderModeType::Incandescence)
	, HeatSensitivity(0.0f)
	, IsHeatSensitivityDirty(true)
	, DebugShipRenderMode(DebugShipRenderModeType::None)
	, AreShipStructureRenderModeSelectorsDirty(true)
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
	AreOceanRenderModeParametersDirty = false;
	IsOceanTextureIndexDirty = false;
	AreLandRenderParametersDirty = false;
	IsLandTextureIndexDirty = false;
	//
	IsFlatLampLightColorDirty = false;
	IsShipWaterColorDirty = false;
	IsShipWaterContrastDirty = false;
	IsShipWaterLevelOfDetailDirty = false;
	IsHeatSensitivityDirty = false;
	AreShipStructureRenderModeSelectorsDirty = false;

	return copy;
}

}