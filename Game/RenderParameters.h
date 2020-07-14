/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ViewModel.h"

#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageSize.h>

#include <cstdint>

namespace Render {

/*
 * The entire set of scalars that are input to the rendering process.
 * The set is comprised of:
 *  - Settings
 *  - Calculated scalars
 */ 
struct RenderParameters
{
    ViewModel View;
    bool IsViewDirty;
    bool IsCanvasSizeDirty;

    float AmbientLightIntensity;
    float StormAmbientDarkening;
    float EffectiveAmbientLightIntensity;
    bool IsEffectiveAmbientLightIntensityDirty;

    float RainDensity;
    bool IsRainDensityDirty;

    //
    // World
    //

    rgbColor FlatSkyColor;    
    float OceanTransparency;
    float OceanDarkeningRate;
    OceanRenderModeType OceanRenderMode;
    size_t SelectedOceanTextureIndex;
    rgbColor DepthOceanColorStart;
    rgbColor DepthOceanColorEnd;
    rgbColor FlatOceanColor;
    LandRenderModeType LandRenderMode;
    size_t SelectedLandTextureIndex;
    rgbColor FlatLandColor;

    //
    // Ship
    //

    size_t ShipCount;    
    bool IsShipCountDirty;

    rgbColor FlatLampLightColor;
    rgbColor DefaultWaterColor;
    bool ShowShipThroughOcean;
    float WaterContrast;
    float WaterLevelOfDetail;
    DebugShipRenderModeType DebugShipRenderMode;
    VectorFieldRenderModeType VectorFieldRenderMode;
    float VectorFieldLengthMultiplier;
    bool ShowStressedSprings;
    bool DrawHeatOverlay;
    float HeatOverlayTransparency;
    ShipFlameRenderModeType ShipFlameRenderMode;
    float ShipFlameSizeAdjustment;

    RenderParameters(ImageSize const & initialCanvasSize);

    RenderParameters Snapshot();
};

}