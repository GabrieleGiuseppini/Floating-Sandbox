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
 * The entire set of user-controllable settings that are input to the rendering process.
 */ 
struct RenderSettings
{
    ViewModel View;
    bool IsViewDirty;
    bool IsCanvasSizeDirty;

    float AmbientLightIntensity;
    float EffectiveAmbientLightIntensity;
    bool IsEffectiveAmbientLightIntensityDirty;    

    //
    // World
    //

    rgbColor FlatSkyColor;
    // TODOHERE
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

    RenderSettings(ImageSize const & initialCanvasSize);

    RenderSettings Snapshot();
};

}