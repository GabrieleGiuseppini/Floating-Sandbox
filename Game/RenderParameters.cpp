/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderParameters.h"

namespace Render {

RenderParameters::RenderParameters(ImageSize const & initialCanvasSize)
	: View(1.0f, vec2f::zero(), initialCanvasSize.Width, initialCanvasSize.Height)
	, IsViewDirty(true)
	, IsCanvasSizeDirty(true)
	, EffectiveAmbientLightIntensity(1.0f) // Calculated
	, IsEffectiveAmbientLightIntensityDirty(true)
	// World
	, FlatSkyColor(0x87, 0xce, 0xfa) // (cornflower blue)	
	, OceanTransparency(0.8125f)
	, OceanDarkeningRate(0.356993f)
	, IsOceanDarkeningRateDirty(true)
	, OceanRenderMode(OceanRenderModeType::Texture)
	, DepthOceanColorStart(0x4a, 0x84, 0x9f)
	, DepthOceanColorEnd(0x00, 0x00, 0x00)
	, FlatOceanColor(0x00, 0x3d, 0x99)
	, AreOceanRenderParametersDirty(true)
	, OceanTextureIndex(0) // Wavy Clear Thin
	, IsOceanTextureIndexDirty(true)
	, ShowShipThroughOcean(false)
	// Ship
	, FlatLampLightColor(0xff, 0xff, 0xbf)
	, IsFlatLampLightColorDirty(true)
	, ShipFlameRenderMode(ShipFlameRenderModeType::Mode1)
	, ShowStressedSprings(false)
	, ShipWaterColor(vec4f::zero()) // Calculated
	, IsShipWaterColorDirty(true)
	// TODOOLD	
	, LandRenderMode(LandRenderModeType::Texture)
	, SelectedLandTextureIndex(3) // Rock Coarse 3
	, FlatLandColor(0x72, 0x46, 0x05)
	//		
	, WaterContrast(0.71875f)
	, WaterLevelOfDetail(0.6875f)
	, DebugShipRenderMode(DebugShipRenderModeType::None)
	, VectorFieldLengthMultiplier(1.0f)	
	, DrawHeatOverlay(false)
	, HeatOverlayTransparency(0.1875f)	
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
	IsOceanDarkeningRateDirty = false;
	AreOceanRenderParametersDirty = false;
	IsOceanTextureIndexDirty = false;
	//
	IsFlatLampLightColorDirty = false;
	IsShipWaterColorDirty = false;

	return copy;
}

}