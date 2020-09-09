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

    LandRenderModeType LandRenderMode;
    rgbColor FlatLandColor;
    bool AreLandRenderParametersDirty; // Tracks all of the above as a whole, for convenience

    size_t LandTextureIndex;
    bool IsLandTextureIndexDirty;

    //
    // Ship
    //

    rgbColor FlatLampLightColor;
    bool IsFlatLampLightColorDirty;

    ShipFlameRenderModeType ShipFlameRenderMode;

    bool ShowStressedSprings;
    bool ShowFrontiers;

    vec4f ShipWaterColor; // Calculated
    bool IsShipWaterColorDirty;

    float ShipWaterContrast;
    bool IsShipWaterContrastDirty;

    float ShipWaterLevelOfDetail;
    bool IsShipWaterLevelOfDetailDirty;

    bool DrawHeatOverlay;

    float HeatOverlayTransparency;
    float IsHeatOverlayTransparencyDirty;

    DebugShipRenderModeType DebugShipRenderMode;
    bool IsDebugShipRenderModeDirty;

    RenderParameters(ImageSize const & initialCanvasSize);

    RenderParameters TakeSnapshotAndClear();
};

}