/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ViewModel.h"

#include <Core/Colors.h>
#include <Core/GameTypes.h>

#include <cstdint>

/*
 * The entire set of user-controllable settings or calculated parameters
 * that are direct input to the rendering process (i.e. which are accessed
 *  by rendering code)
 */
struct RenderParameters
{
    ViewModel View;
    float EffectiveAmbientLightIntensity; // Calculated

    //
    // World
    //

    rgbColor FlatSkyColor;
    rgbColor EffectiveMoonlightColor; // Calculated
    bool DoCrepuscularGradient;
    rgbColor CrepuscularColor;

    CloudRenderDetailType CloudRenderDetail;

    float OceanTransparency;
    float OceanDepthDarkeningRate;
    OceanRenderModeType OceanRenderMode;
    rgbColor DepthOceanColorStart;
    rgbColor DepthOceanColorEnd;
    rgbColor FlatOceanColor;
    size_t OceanTextureIndex;
    OceanRenderDetailType OceanRenderDetail;
    bool ShowShipThroughOcean;

    LandRenderModeType LandRenderMode;
    rgbColor FlatLandColor;
    size_t LandTextureIndex;
    LandRenderDetailType LandRenderDetail;

    //
    // Ship
    //

    ShipViewModeType ShipViewMode;
    float ShipAmbientLightSensitivity;
    float ShipDepthDarkeningSensitivity;
    rgbColor FlatLampLightColor;
    bool DrawExplosions;
    bool DrawFlames;
    float ShipFlameKaosAdjustment;
    bool ShowStressedSprings;
    bool ShowFrontiers;
    bool ShowAABBs;
    vec3f ShipWaterColor; // Calculated
    float ShipWaterContrast;
    float ShipWaterLevelOfDetail;
    HeatRenderModeType HeatRenderMode;
    float HeatSensitivity;
    StressRenderModeType StressRenderMode;
    ShipParticleRenderModeType ShipParticleRenderMode;
    DebugShipRenderModeType DebugShipRenderMode;
    NpcRenderModeType NpcRenderMode;
    rgbColor NpcQuadFlatColor;

    //
    // Misc
    //

    UnitsSystem DisplayUnitsSystem;

    //
    // Dirty flags
    //

    // World
    bool IsViewDirty;
    bool IsCanvasSizeDirty;
    bool IsEffectiveAmbientLightIntensityDirty;
    bool IsSkyDirty; // Tracks various sky render parameters as a whole, for convenience
    bool IsCloudRenderDetailDirty;
    bool IsOceanDepthDarkeningRateDirty;
    bool AreOceanRenderParametersDirty; // Tracks various ocean render parameters as a whole, for convenience
    bool IsOceanTextureIndexDirty;
    bool AreLandRenderParametersDirty; // Tracks various land render parameters as a whole, for convenience
    bool IsLandTextureIndexDirty;
    bool IsLandRenderDetailDirty;
    // Ship
    bool IsShipViewModeDirty;
    bool IsShipAmbientLightSensitivityDirty;
    bool IsShipDepthDarkeningSensitivityDirty;
    bool IsFlatLampLightColorDirty;
    bool AreShipFlameRenderParametersDirty;
    bool IsShipWaterColorDirty;
    bool IsShipWaterContrastDirty;
    bool IsShipWaterLevelOfDetailDirty;
    bool IsHeatSensitivityDirty;
    bool AreShipStructureRenderModeSelectorsDirty; // For all those parameters that require changing ship shaders
    bool AreNpcRenderParametersDirty;
    // Misc
    bool IsDisplayUnitsSystemDirty;

    RenderParameters(
        FloatSize const & maxWorldSize,
        DisplayLogicalSize const & initialCanvasSize,
        int logicalToPhysicalDisplayFactor);

    RenderParameters TakeSnapshotAndClear();
};
