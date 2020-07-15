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
 * The entire set of user-controllable settings or calculated parameters 
 * that are direct input to the rendering process (i.e. which are accessed
 *  by rendering code)
 */ 
struct RenderParameters
{
    ViewModel View;
    bool IsViewDirty;
    bool IsCanvasSizeDirty;
    
    float EffectiveAmbientLightIntensity; // Calculated
    bool IsEffectiveAmbientLightIntensityDirty;    

    //
    // World
    //

    rgbColor FlatSkyColor;

    float OceanTransparency;

    float OceanDarkeningRate;
    bool IsOceanDarkeningRateDirty;

    OceanRenderModeType OceanRenderMode;
    rgbColor DepthOceanColorStart;
    rgbColor DepthOceanColorEnd;
    rgbColor FlatOceanColor;
    bool AreOceanRenderParametersDirty; // Tracks all of the above as a whole, for convenience

    size_t OceanTextureIndex;
    bool IsOceanTextureIndexDirty;

    bool ShowShipThroughOcean;

    //
    // Ship
    //

    rgbColor FlatLampLightColor;
    bool IsFlatLampLightColorDirty;

    ShipFlameRenderModeType ShipFlameRenderMode;

    bool ShowStressedSprings;

    vec4f ShipWaterColor; // Calculated
    bool IsShipWaterColorDirty;

    // TODOOLD           
    LandRenderModeType LandRenderMode;
    size_t SelectedLandTextureIndex;
    rgbColor FlatLandColor;
    //
    float WaterContrast;
    float WaterLevelOfDetail;
    DebugShipRenderModeType DebugShipRenderMode;
    float VectorFieldLengthMultiplier;    
    bool DrawHeatOverlay;
    float HeatOverlayTransparency;    

    RenderParameters(ImageSize const & initialCanvasSize);

    RenderParameters TakeSnapshotAndClear();
};

}