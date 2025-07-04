/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameShaderSets.h"
#include "GameTextureDatabases.h"
#include "GlobalRenderContext.h"
#include "NotificationRenderContext.h"
#include "RenderDeviceProperties.h"
#include "RenderParameters.h"
#include "ShipRenderContext.h"
#include "WorldRenderContext.h"

#include <OpenGLCore/GameOpenGL.h>
#include <OpenGLCore/ShaderManager.h>

#include <Core/AABB.h>
#include <Core/Colors.h>
#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/ImageData.h>
#include <Core/PerfStats.h>
#include <Core/ProgressCallback.h>
#include <Core/RunningAverage.h>
#include <Core/SysSpecifics.h>
#include <Core/TaskThread.h>
#include <Core/TextureAtlas.h>
#include <Core/ThreadManager.h>
#include <Core/Vectors.h>

#include <array>
#include <atomic>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/*
 * This class is the entry point of the entire rendering subsystem, providing
 * the API for rendering, which is agnostic about the render platform implementation.
 *
 * This class is in turn a coordinator of a number of child contextes, each focusing
 * on a different subset of the rendering universe (world, ships, UI); this class
 * dispatches all externally-invoked API calls to the child contexts implementing
 * those calls.
 */
class RenderContext
{
public:

    RenderContext(
        RenderDeviceProperties const & renderDeviceProperties,
        FloatSize const & maxWorldSize,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> && npcTextureAtlas,
        PerfStats & perfStats,
        ThreadManager & threadManager,
        IAssetManager const & assetManager,
        ProgressCallback const & progressCallback);

    ~RenderContext();

public:

    //
    // World and view properties
    //

    float const & GetZoom() const
    {
        return mRenderParameters.View.GetZoom();
    }

    float ClampZoom(float zoom) const
    {
        return mRenderParameters.View.ClampZoom(zoom);
    }

    float const & SetZoom(float zoom)
    {
        float const & newZoom = mRenderParameters.View.SetZoom(zoom);
        mRenderParameters.IsViewDirty = true;

        return newZoom;
    }

    vec2f const & GetCameraWorldPosition() const
    {
        return mRenderParameters.View.GetCameraWorldPosition();
    }

    vec2f ClampCameraWorldPosition(vec2f const & pos) const
    {
        return mRenderParameters.View.ClampCameraWorldPosition(pos);
    }

    vec2f const & SetCameraWorldPosition(vec2f const & pos)
    {
        vec2f const & newCameraWorldPosition = mRenderParameters.View.SetCameraWorldPosition(pos);
        mRenderParameters.IsViewDirty = true;

        return newCameraWorldPosition;
    }

    VisibleWorld const & GetVisibleWorld() const
    {
        return mRenderParameters.View.GetVisibleWorld();
    }

    DisplayLogicalSize const & GetCanvasLogicalSize() const
    {
        return mRenderParameters.View.GetCanvasLogicalSize();
    }

    DisplayPhysicalSize const & GetCanvasPhysicalSize() const
    {
        return mRenderParameters.View.GetCanvasPhysicalSize();
    }

    void SetCanvasLogicalSize(DisplayLogicalSize const & canvasSize)
    {
        mRenderParameters.View.SetCanvasLogicalSize(canvasSize);
        mRenderParameters.IsViewDirty = true;
        mRenderParameters.IsCanvasSizeDirty = true;
    }

    void SetPixelOffset(float x, float y)
    {
        mRenderParameters.View.SetPixelOffset(x, y);
        mRenderParameters.IsViewDirty = true;
    }

    void ResetPixelOffset()
    {
        mRenderParameters.View.ResetPixelOffset();
        mRenderParameters.IsViewDirty = true;
    }

    float CalculateZoomForWorldWidth(float worldWidth) const
    {
        return mRenderParameters.View.CalculateZoomForWorldWidth(worldWidth);
    }

    float CalculateZoomForWorldHeight(float worldHeight) const
    {
        return mRenderParameters.View.CalculateZoomForWorldHeight(worldHeight);
    }

    ViewModel const & GetViewModel() const
    {
        return mRenderParameters.View;
    }

    // Render properties

    float GetAmbientLightIntensity() const
    {
        return mAmbientLightIntensity;
    }

    void SetAmbientLightIntensity(float intensity)
    {
        // Assume calls are already damped

        mAmbientLightIntensity = intensity;

        // Re-calculate effective ambient light intensity
        mRenderParameters.EffectiveAmbientLightIntensity = CalculateEffectiveAmbientLightIntensity(
            mAmbientLightIntensity,
            mWorldRenderContext->GetStormAmbientDarkening());
        mRenderParameters.IsEffectiveAmbientLightIntensityDirty = true;
    }

    float GetEffectiveAmbientLightIntensity() const
    {
        return mRenderParameters.EffectiveAmbientLightIntensity;
    }

    void SetSunRaysInclination(float value)
    {
        mWorldRenderContext->SetSunRaysInclination(value);
    }

    void SetLamp(
        DisplayLogicalCoordinates const & pos,
        float radiusScreenFraction)
    {
        DisplayPhysicalCoordinates const physicalPos = mRenderParameters.View.ScreenToPhysicalDisplay(pos);
        float physicalRadius = mRenderParameters.View.ScreenFractionToPhysicalDisplay(radiusScreenFraction);

        // Safe as it's copied to thread
        mLampToolToSet.emplace(
            static_cast<float>(physicalPos.x),
            static_cast<float>(physicalPos.y),
            physicalRadius,
            1.0f);
    }

    void ResetLamp()
    {
        // Safe as it's copied to thread
        mLampToolToSet.emplace(vec4f::zero());
    }

    //

    rgbColor const & GetFlatSkyColor() const
    {
        return mRenderParameters.FlatSkyColor;
    }

    void SetFlatSkyColor(rgbColor const & color)
    {
        mRenderParameters.FlatSkyColor = color;
        mRenderParameters.IsSkyDirty = true;
    }

    bool GetDoMoonlight() const
    {
        return mDoMoonlight;
    }

    void SetDoMoonlight(bool value)
    {
        mDoMoonlight = value;

        // Re-calculate effective moonlight color
        mRenderParameters.EffectiveMoonlightColor = CalculateEffectiveMoonlightColor(
            mMoonlightColor,
            mDoMoonlight);
        mRenderParameters.IsSkyDirty = true;
    }

    rgbColor const & GetMoonlightColor() const
    {
        return mMoonlightColor;
    }

    void SetMoonlightColor(rgbColor const & color)
    {
        mMoonlightColor = color;

        // Re-calculate effective moonlight color
        mRenderParameters.EffectiveMoonlightColor = CalculateEffectiveMoonlightColor(
            mMoonlightColor,
            mDoMoonlight);
        mRenderParameters.IsSkyDirty = true;
    }

    bool GetDoCrepuscularGradient() const
    {
        return mRenderParameters.DoCrepuscularGradient;
    }

    void SetDoCrepuscularGradient(bool value)
    {
        mRenderParameters.DoCrepuscularGradient = value;
        mRenderParameters.IsSkyDirty = true;
    }

    rgbColor const & GetCrepuscularColor() const
    {
        return mRenderParameters.CrepuscularColor;
    }

    void SetCrepuscularColor(rgbColor const & color)
    {
        mRenderParameters.CrepuscularColor = color;
        mRenderParameters.IsSkyDirty = true;
    }

    CloudRenderDetailType GetCloudRenderDetail() const
    {
        return mRenderParameters.CloudRenderDetail;
    }

    void SetCloudRenderDetail(CloudRenderDetailType cloudRenderDetail)
    {
        mRenderParameters.CloudRenderDetail = cloudRenderDetail;
        // No need to set dirty, this is picked up at each cycle anway
    }

    float GetOceanTransparency() const
    {
        return mRenderParameters.OceanTransparency;
    }

    void SetOceanTransparency(float transparency)
    {
        mRenderParameters.OceanTransparency = transparency;
        // No need to set dirty, this is picked up at each cycle anway
    }

    float GetOceanDepthDarkeningRate() const
    {
        return mRenderParameters.OceanDepthDarkeningRate;
    }

    void SetOceanDepthDarkeningRate(float darkeningRate)
    {
        mRenderParameters.OceanDepthDarkeningRate = darkeningRate;
        mRenderParameters.IsOceanDepthDarkeningRateDirty = true;
    }

    OceanRenderModeType GetOceanRenderMode() const
    {
        return mRenderParameters.OceanRenderMode;
    }

    void SetOceanRenderMode(OceanRenderModeType oceanRenderMode)
    {
        mRenderParameters.OceanRenderMode = oceanRenderMode;
        mRenderParameters.AreOceanRenderParametersDirty = true;

        mRenderParameters.ShipWaterColor = CalculateShipWaterColor();
        mRenderParameters.IsShipWaterColorDirty = true;
    }

    rgbColor const & GetDepthOceanColorStart() const
    {
        return mRenderParameters.DepthOceanColorStart;
    }

    void SetDepthOceanColorStart(rgbColor const & color)
    {
        mRenderParameters.DepthOceanColorStart = color;
        mRenderParameters.AreOceanRenderParametersDirty = true;

        mRenderParameters.ShipWaterColor = CalculateShipWaterColor();
        mRenderParameters.IsShipWaterColorDirty = true;
    }

    rgbColor const & GetDepthOceanColorEnd() const
    {
        return mRenderParameters.DepthOceanColorEnd;
    }

    void SetDepthOceanColorEnd(rgbColor const & color)
    {
        mRenderParameters.DepthOceanColorEnd = color;
        mRenderParameters.AreOceanRenderParametersDirty = true;

        mRenderParameters.ShipWaterColor = CalculateShipWaterColor();
        mRenderParameters.IsShipWaterColorDirty = true;
    }

    rgbColor const & GetFlatOceanColor() const
    {
        return mRenderParameters.FlatOceanColor;
    }

    void SetFlatOceanColor(rgbColor const & color)
    {
        mRenderParameters.FlatOceanColor = color;
        mRenderParameters.AreOceanRenderParametersDirty = true;

        mRenderParameters.ShipWaterColor = CalculateShipWaterColor();
        mRenderParameters.IsShipWaterColorDirty = true;
    }

    inline std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureOceanAvailableThumbnails() const
    {
        return mWorldRenderContext->GetTextureOceanAvailableThumbnails();
    }

    size_t GetTextureOceanTextureIndex() const
    {
        return mRenderParameters.OceanTextureIndex;
    }

    void SetTextureOceanTextureIndex(size_t index)
    {
        mRenderParameters.OceanTextureIndex = index;
        mRenderParameters.IsOceanTextureIndexDirty = true;
    }

    OceanRenderDetailType GetOceanRenderDetail() const
    {
        return mRenderParameters.OceanRenderDetail;
    }

    void SetOceanRenderDetail(OceanRenderDetailType oceanRenderDetail)
    {
        mRenderParameters.OceanRenderDetail = oceanRenderDetail;
        // No need to set dirty, this is picked up at each cycle anway
    }

    bool GetShowShipThroughOcean() const
    {
        return mRenderParameters.ShowShipThroughOcean;
    }

    void SetShowShipThroughOcean(bool showShipThroughOcean)
    {
        mRenderParameters.ShowShipThroughOcean = showShipThroughOcean;
        // No need to set dirty, this is picked up at each cycle anway
    }

    LandRenderModeType GetLandRenderMode() const
    {
        return mRenderParameters.LandRenderMode;
    }

    void SetLandRenderMode(LandRenderModeType landRenderMode)
    {
        mRenderParameters.LandRenderMode = landRenderMode;
        mRenderParameters.AreLandRenderParametersDirty = true;
    }

    rgbColor const & GetFlatLandColor() const
    {
        return mRenderParameters.FlatLandColor;
    }

    void SetFlatLandColor(rgbColor const & color)
    {
        mRenderParameters.FlatLandColor = color;
        mRenderParameters.AreLandRenderParametersDirty = true;
    }

    std::vector<std::pair<std::string, RgbaImageData>> const & GetTextureLandAvailableThumbnails() const
    {
        return mWorldRenderContext->GetTextureLandAvailableThumbnails();
    }

    size_t GetTextureLandTextureIndex() const
    {
        return mRenderParameters.LandTextureIndex;
    }

    void SetTextureLandTextureIndex(size_t index)
    {
        mRenderParameters.LandTextureIndex = index;
        mRenderParameters.IsLandTextureIndexDirty = true;
    }

    LandRenderDetailType GetLandRenderDetail() const
    {
        return mRenderParameters.LandRenderDetail;
    }

    void SetLandRenderDetail(LandRenderDetailType landRenderDetail)
    {
        mRenderParameters.LandRenderDetail = landRenderDetail;
        mRenderParameters.IsLandRenderDetailDirty = true;
    }

    //
    // Ship rendering properties
    //

    ShipViewModeType GetShipViewMode() const
    {
        return mRenderParameters.ShipViewMode;
    }

    void SetShipViewMode(ShipViewModeType shipViewMode)
    {
        mRenderParameters.ShipViewMode = shipViewMode;
        mRenderParameters.IsShipViewModeDirty = true;
    }

    float GetShipAmbientLightSensitivity() const
    {
        return mRenderParameters.ShipAmbientLightSensitivity;
    }

    void SetShipAmbientLightSensitivity(float shipAmbientLightSensitivity)
    {
        mRenderParameters.ShipAmbientLightSensitivity = shipAmbientLightSensitivity;
        mRenderParameters.IsShipAmbientLightSensitivityDirty = true;
    }

    float GetShipDepthDarkeningSensitivity() const
    {
        return mRenderParameters.ShipDepthDarkeningSensitivity;
    }

    void SetShipDepthDarkeningSensitivity(float shipDepthDarkeningSensitivity)
    {
        mRenderParameters.ShipDepthDarkeningSensitivity = shipDepthDarkeningSensitivity;
        mRenderParameters.IsShipDepthDarkeningSensitivityDirty = true;
    }

    rgbColor const & GetFlatLampLightColor() const
    {
        return mRenderParameters.FlatLampLightColor;
    }

    void SetFlatLampLightColor(rgbColor const & color)
    {
        mRenderParameters.FlatLampLightColor = color;
        mRenderParameters.IsFlatLampLightColorDirty = true;
    }

    bool GetDrawExplosions() const
    {
        return mRenderParameters.DrawExplosions;
    }

    void SetDrawExplosions(bool drawExplosions)
    {
        mRenderParameters.DrawExplosions = drawExplosions;
        // No need to set dirty, this is picked up at each cycle anway
    }

    bool GetDrawFlames() const
    {
        return mRenderParameters.DrawFlames;
    }

    void SetDrawFlames(bool drawFlames)
    {
        mRenderParameters.DrawFlames = drawFlames;
        // No need to set dirty, this is picked up at each cycle anway
    }

    float const & GetShipFlameSizeAdjustment() const
    {
        return mShipFlameSizeAdjustment;
    }

    void SetShipFlameSizeAdjustment(float shipFlameSizeAdjustment)
    {
        mShipFlameSizeAdjustment = shipFlameSizeAdjustment;

        for (auto & s : mShips)
        {
            s->SetShipFlameSizeAdjustment(mShipFlameSizeAdjustment);
        }
    }

    static constexpr float MinShipFlameSizeAdjustment = 0.1f;
    static constexpr float MaxShipFlameSizeAdjustment = 20.0f;

    float GetShipFlameKaosAdjustment() const
    {
        return mRenderParameters.ShipFlameKaosAdjustment;
    }

    void SetShipFlameKaosAdjustment(float value)
    {
        mRenderParameters.ShipFlameKaosAdjustment = value;
        mRenderParameters.AreShipFlameRenderParametersDirty = true;
    }

    static constexpr float MinShipFlameKaosAdjustment = 0.0f;
    static constexpr float MaxShipFlameKaosAdjustment = 2.0f;

    bool GetShowStressedSprings() const
    {
        return mRenderParameters.ShowStressedSprings;
    }

    void SetShowStressedSprings(bool showStressedSprings)
    {
        mRenderParameters.ShowStressedSprings = showStressedSprings;
        // No need to set dirty, this is picked up at each cycle anway
    }

    bool GetShowFrontiers() const
    {
        return mRenderParameters.ShowFrontiers;
    }

    void SetShowFrontiers(bool showFrontiers)
    {
        mRenderParameters.ShowFrontiers = showFrontiers;
        // No need to set dirty, this is picked up at each cycle anway
    }

    bool GetShowAABBs() const
    {
        return mRenderParameters.ShowAABBs;
    }

    void SetShowAABBs(bool showAABBs)
    {
        mRenderParameters.ShowAABBs = showAABBs;
        // No need to set dirty, this is picked up at each cycle anway
    }

    rgbColor const & GetShipDefaultWaterColor() const
    {
        return mShipDefaultWaterColor;
    }

    void SetShipDefaultWaterColor(rgbColor const & color)
    {
        mShipDefaultWaterColor = color;

        mRenderParameters.ShipWaterColor = CalculateShipWaterColor();
        mRenderParameters.IsShipWaterColorDirty = true;
    }

    float GetShipWaterContrast() const
    {
        return mRenderParameters.ShipWaterContrast;
    }

    void SetShipWaterContrast(float contrast)
    {
        mRenderParameters.ShipWaterContrast = contrast;
        mRenderParameters.IsShipWaterContrastDirty = true;
    }

    float GetShipWaterLevelOfDetail() const
    {
        return mRenderParameters.ShipWaterLevelOfDetail;
    }

    void SetShipWaterLevelOfDetail(float levelOfDetail)
    {
        mRenderParameters.ShipWaterLevelOfDetail = levelOfDetail;
        mRenderParameters.IsShipWaterLevelOfDetailDirty = true;
    }

    static constexpr float MinShipWaterLevelOfDetail = 0.0f;
    static constexpr float MaxShipWaterLevelOfDetail = 1.0f;

    HeatRenderModeType GetHeatRenderMode() const
    {
        return mRenderParameters.HeatRenderMode;
    }

    void SetHeatRenderMode(HeatRenderModeType heatRenderMode)
    {
        mRenderParameters.HeatRenderMode = heatRenderMode;
        mRenderParameters.AreShipStructureRenderModeSelectorsDirty = true;
    }

    float GetHeatSensitivity() const
    {
        return mRenderParameters.HeatSensitivity;
    }

    void SetHeatSensitivity(float heatSensitivity)
    {
        mRenderParameters.HeatSensitivity = heatSensitivity;
        mRenderParameters.IsHeatSensitivityDirty = true;
    }

    StressRenderModeType GetStressRenderMode() const
    {
        return mRenderParameters.StressRenderMode;
    }

    void SetStressRenderMode(StressRenderModeType stressRenderMode)
    {
        mRenderParameters.StressRenderMode = stressRenderMode;
        mRenderParameters.AreShipStructureRenderModeSelectorsDirty = true;
    }

    VectorFieldRenderModeType GetVectorFieldRenderMode() const
    {
        return mVectorFieldRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderModeType vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;

        for (auto & s : mShips)
        {
            s->SetVectorFieldLengthMultiplier(mVectorFieldLengthMultiplier);
        }
    }

    float GetVectorFieldLengthMultiplier() const
    {
        return mVectorFieldLengthMultiplier;
    }

    void SetVectorFieldLengthMultiplier(float vectorFieldLengthMultiplier)
    {
        mVectorFieldLengthMultiplier = vectorFieldLengthMultiplier;
    }

    ShipParticleRenderModeType GetShipParticleRenderMode() const
    {
        return mRenderParameters.ShipParticleRenderMode;
    }

    void SetShipParticleRenderMode(ShipParticleRenderModeType shipParticleRenderMode)
    {
        mRenderParameters.ShipParticleRenderMode = shipParticleRenderMode;
        mRenderParameters.AreShipStructureRenderModeSelectorsDirty = true;
    }

    DebugShipRenderModeType GetDebugShipRenderMode() const
    {
        return mRenderParameters.DebugShipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode)
    {
        mRenderParameters.DebugShipRenderMode = debugShipRenderMode;
        mRenderParameters.AreShipStructureRenderModeSelectorsDirty = true;
    }

    NpcRenderModeType GetNpcRenderMode() const
    {
        return mRenderParameters.NpcRenderMode;
    }

    void SetNpcRenderMode(NpcRenderModeType npcRenderMode)
    {
        mRenderParameters.NpcRenderMode = npcRenderMode;
        mRenderParameters.AreNpcRenderParametersDirty = true;
    }

    rgbColor const & GetNpcQuadFlatColor() const
    {
        return mRenderParameters.NpcQuadFlatColor;
    }

    void SetNpcQuadFlatColor(rgbColor const & color)
    {
        mRenderParameters.NpcQuadFlatColor = color;
        mRenderParameters.AreNpcRenderParametersDirty = true;
    }

    //
    // Misc rendering properties
    //

    UnitsSystem GetDisplayUnitsSystem() const
    {
        return mRenderParameters.DisplayUnitsSystem;
    }

    void SetDisplayUnitsSystem(UnitsSystem unitsSystem)
    {
        mRenderParameters.DisplayUnitsSystem = unitsSystem;
        mRenderParameters.IsDisplayUnitsSystemDirty = true;
    }

    //
    // Coordinate transformations
    //

    inline vec2f WorldToNdc(vec2f const & worldCoordinates) const
    {
        return mRenderParameters.View.WorldToNdc(worldCoordinates);
    }

    inline vec2f WorldToNdc(
        vec2f const & worldCoordinates,
        float zoom,
        vec2f const & cameraWorldPosition) const
    {
        return mRenderParameters.View.WorldToNdc(
            worldCoordinates,
            zoom,
            cameraWorldPosition);
    }

    inline vec2f NdcOffsetToWorldOffset(
        vec2f const & ndcOffset,
        float zoom) const
    {
        return mRenderParameters.View.NdcOffsetToWorldOffset(
            ndcOffset,
            zoom);
    }

    inline vec2f ScreenToWorld(DisplayLogicalCoordinates const & screenCoordinates) const
    {
        return mRenderParameters.View.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(DisplayLogicalSize const & screenOffset) const
    {
        return mRenderParameters.View.ScreenOffsetToWorldOffset(screenOffset);
    }

    inline float ScreenOffsetToWorldOffset(int screenOffset) const
    {
        return mRenderParameters.View.ScreenOffsetToWorldOffset(screenOffset);
    }

    inline float ScreenFractionToWorldOffset(float screenFraction) const
    {
        return mRenderParameters.View.ScreenFractionToWorldOffset(screenFraction);
    }

    //
    // Statistics
    //

    RenderStatistics GetStatistics() const
    {
        return mRenderStats.load();
    }

public:

    void RebindContext();

    void Reset();

    void ValidateShipTexture(RgbaImageData const & texture) const;

    void AddShip(
        ShipId shipId,
        size_t pointCount,
        size_t maxEphemeralParticles,
        size_t maxSpringsPerPoint,
        RgbaImageData exteriorTextureImage,
        RgbaImageData interiorViewImage);

    inline ShipRenderContext & GetShipRenderContext(ShipId shipId) const
    {
        assert(shipId >= 0 && shipId < mShips.size());

        return *(mShips[shipId]);
    }

    inline NotificationRenderContext & GetNoficationRenderContext() const
    {
        assert(!!mNotificationRenderContext);

        return *mNotificationRenderContext;
    }

    RgbImageData TakeScreenshot();

public:

    void UpdateStart();

    void UpdateEnd();

    void RenderStart();

    void UploadStart();

    inline void UploadStarsStart(
        size_t uploadCount,
        size_t totalCount)
    {
        mWorldRenderContext->UploadStarsStart(uploadCount, totalCount);
    }

    inline void UploadStar(
        size_t starIndex,
        vec2f const & positionNdc,
        float brightness)
    {
        mWorldRenderContext->UploadStar(
            starIndex,
            positionNdc,
            brightness);
    }

    inline void UploadStarsEnd()
    {
        mWorldRenderContext->UploadStarsEnd();
    }

    inline void UploadWind(vec2f speed)
    {
        float const smoothedWindMagnitude = mWindSpeedMagnitudeRunningAverage.Update(speed.x);
        if (smoothedWindMagnitude != mCurrentWindSpeedMagnitude) // Damp frequency of calls
        {
            mWorldRenderContext->UploadWind(smoothedWindMagnitude);

            mCurrentWindSpeedMagnitude = smoothedWindMagnitude;
        }
    }

    inline void UploadStormAmbientDarkening(float darkening)
    {
        if (mWorldRenderContext->UploadStormAmbientDarkening(darkening))
        {
            mRenderParameters.EffectiveAmbientLightIntensity = CalculateEffectiveAmbientLightIntensity(
                mAmbientLightIntensity,
                mWorldRenderContext->GetStormAmbientDarkening());

            mRenderParameters.IsEffectiveAmbientLightIntensityDirty = true;
        }
    }

    inline void UploadRain(float density)
    {
        mWorldRenderContext->UploadRain(density);
    }

    inline void UploadLightningsStart(size_t lightningCount)
    {
        mWorldRenderContext->UploadLightningsStart(lightningCount);
    }

    inline void UploadBackgroundLightning(
        float ndcX,
        float progress,
        float renderProgress,
        float personalitySeed)
    {
        mWorldRenderContext->UploadBackgroundLightning(
            ndcX,
            progress,
            renderProgress,
            personalitySeed,
            mRenderParameters);
    }

    inline void UploadForegroundLightning(
        vec2f tipWorldCoordinates,
        float progress,
        float renderProgress,
        float personalitySeed)
    {
        mWorldRenderContext->UploadForegroundLightning(
            tipWorldCoordinates,
            progress,
            renderProgress,
            personalitySeed,
            mRenderParameters);
    }

    inline void UploadLightningsEnd()
    {
        mWorldRenderContext->UploadLightningsEnd();
    }

    inline void UploadCloudsStart(size_t cloudCount)
    {
        mWorldRenderContext->UploadCloudsStart(cloudCount);
    }

    inline void UploadCloud(
        uint32_t cloudId,
        float virtualX,         // [-1.5, +1.5]
        float virtualY,         // [0.0, +1.0]
        float virtualZ,         // [0.0, +1.0]
        float scale,
        float darkening,        // 0.0:dark, 1.0:light
        float volumetricGrowthProgress)
    {
        mWorldRenderContext->UploadCloud(
            cloudId,
            virtualX,
            virtualY,
            virtualZ,
            scale,
            darkening,
            volumetricGrowthProgress,
            mRenderParameters);
    }

    inline void UploadCloudsEnd()
    {
        mWorldRenderContext->UploadCloudsEnd();
    }

    inline void UploadCloudShadows(
        float const * shadowBuffer,
        size_t shadowSampleCount)
    {
        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mWorldRenderContext->UploadCloudShadows(
                    shadowBuffer,
                    shadowSampleCount);
            });
    }

    inline void UploadLandStart(size_t slices)
    {
        mWorldRenderContext->UploadLandStart(slices);
    }

    inline void UploadLand(
        float x,
        float yLand)
    {
        mWorldRenderContext->UploadLand(
            x,
            yLand,
            mRenderParameters);
    }

    inline void UploadLandEnd()
    {
        mWorldRenderContext->UploadLandEnd();
    }

    inline void UploadOceanBasicStart(size_t slices)
    {
        mWorldRenderContext->UploadOceanBasicStart(slices);
    }

    inline void UploadOceanBasic(
        float x,
        float yOcean)
    {
        mWorldRenderContext->UploadOceanBasic(
            x,
            yOcean,
            mRenderParameters);
    }

    inline void UploadOceanBasicEnd()
    {
        mWorldRenderContext->UploadOceanBasicEnd();
    }

    inline void UploadOceanDetailedStart(size_t slices)
    {
        mWorldRenderContext->UploadOceanDetailedStart(slices);
    }

    inline void UploadOceanDetailed(
        float x,
        float yBack,
        float yMid,
        float yFront,
        float d2YFront)
    {
        mWorldRenderContext->UploadOceanDetailed(
            x,
            yBack,
            yMid,
            yFront,
            d2YFront,
            mRenderParameters);
    }

    inline void UploadOceanDetailedEnd()
    {
        mWorldRenderContext->UploadOceanDetailedEnd();
    }

    inline void UploadFishesStart(size_t fishCount)
    {
        mWorldRenderContext->UploadFishesStart(fishCount);
    }

    inline void UploadFish(
        TextureFrameId<GameTextureDatabases::FishTextureGroups> const & textureFrameId,
        vec2f const & position,
        vec2f const & worldSize,
        float angleCw,
        float horizontalScale,
        float tailX,
        float tailSwing,
        float tailProgress)
    {
        mWorldRenderContext->UploadFish(
            textureFrameId,
            position,
            worldSize,
            angleCw,
            horizontalScale,
            tailX,
            tailSwing,
            tailProgress);
    }

    inline void UploadFishesEnd()
    {
        mWorldRenderContext->UploadFishesEnd();
    }

    inline void UploadAMBombPreImplosion(
        vec2f const & centerPosition,
        float progress,
        float radius)
    {
        mWorldRenderContext->UploadAMBombPreImplosion(
            centerPosition,
            progress,
            radius);
    }

    inline void UploadCrossOfLight(
        vec2f const & centerPosition,
        float progress)
    {
        mWorldRenderContext->UploadCrossOfLight(
            centerPosition,
            progress,
            mRenderParameters);
    }

    inline void UploadAABBsStart(size_t aabbCount)
    {
        mWorldRenderContext->UploadAABBsStart(aabbCount);
    }

    inline void UploadAABB(
        Geometry::AABB const & aabb,
        vec4f const & color)
    {
        mWorldRenderContext->UploadAABB(
            aabb,
            color);
    }

    inline void UploadAABBsEnd()
    {
        mWorldRenderContext->UploadAABBsEnd();
    }

    inline void UploadShipsStart()
    {
        // Nop
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointColorsAsync(
        ShipId shipId,
        vec4f const * color,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointColors(
                    color,
                    startDst,
                    count);
            });
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointTemperatureAsync(
        ShipId shipId,
        float const * temperature,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointTemperature(
                    temperature,
                    startDst,
                    count);
            });
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointStressAsync(
        ShipId shipId,
        float const * stress,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointStress(
                    stress,
                    startDst,
                    count);
            });
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointAuxiliaryDataAsync(
        ShipId shipId,
        float const * auxiliaryData,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointAuxiliaryData(
                    auxiliaryData,
                    startDst,
                    count);
            });
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointFrontierColorsAsync(
        ShipId shipId,
        ColorWithProgress const * colors)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointFrontierColors(colors);
            });
    }

    inline void UploadShipsEnd()
    {
        // Nop
    }

    void UploadRectSelection(
        vec2f const & centerPosition,
        vec2f const & verticalDir,
        float width,
        float height,
        rgbColor const & color,
        float elapsedSimulationTime)
    {
        mNotificationRenderContext->UploadRectSelection(
            centerPosition,
            verticalDir,
            width,
            height,
            color,
            elapsedSimulationTime,
            mRenderParameters.View);
    }

    void UploadEnd();

    void Draw();

    void RenderEnd();

    void WaitForPendingTasks();

private:

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);
    void ApplyShipStructureRenderModeChanges(RenderParameters const & renderParameters);

    static float CalculateEffectiveAmbientLightIntensity(
        float ambientLightIntensity,
        float stormAmbientDarkening);

    static rgbColor CalculateEffectiveMoonlightColor(
        rgbColor moonlightColor,
        bool doMoonlight);

    vec3f CalculateShipWaterColor() const;

private:

    //
    // Boot settings
    //

    bool mDoInvokeGlFinish;

    //
    // Render Thread
    //

    // The thread running all of our OpenGL calls
    TaskThread mRenderThread;

    // The asynchronous rendering tasks from the previous iteration,
    // which we have to wait for before proceeding further
    TaskThread::TaskCompletionIndicator mLastRenderUploadEndCompletionIndicator;
    TaskThread::TaskCompletionIndicator mLastRenderDrawCompletionIndicator;

    //
    // Shader manager
    //

    std::unique_ptr<ShaderManager<GameShaderSets::ShaderSet>> mShaderManager;

    //
    // Child contextes
    //

    std::unique_ptr<GlobalRenderContext> mGlobalRenderContext;
    std::unique_ptr<WorldRenderContext> mWorldRenderContext;
    std::vector<std::unique_ptr<ShipRenderContext>> mShips;
    std::unique_ptr<NotificationRenderContext> mNotificationRenderContext;

    //
    // Storage for externally-controlled parameters that only affect Upload (i.e.
    // that do not affect rendering directly), or that purely serve as input to
    // calculated render parameters, or that only need storage here (e.g. being used
    // in other contexts to control upload's)
    //

    float mAmbientLightIntensity; // Combined with real-time stork darkening to make EffectiveAmbientLightIntensity
    rgbColor mMoonlightColor; // Combined with below to make EffectiveMoonlightColor
    bool mDoMoonlight; // Combined with above to make EffectiveMoonlightColor
    float mShipFlameSizeAdjustment;
    rgbColor mShipDefaultWaterColor;
    VectorFieldRenderModeType mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier; // Storage

    //
    // Render state
    //

    std::optional<vec4f> mLampToolToSet; // When set, we need to give to render thread at Draw

    //
    // Rendering externals
    //

    std::function<void()> const mMakeRenderContextCurrentFunction;
    std::function<void()> const mSwapRenderBuffersFunction;


    //
    // Render parameters
    //

    RenderParameters mRenderParameters;

    //
    // State
    //

    // Wind
    RunningAverage<32> mWindSpeedMagnitudeRunningAverage;
    float mCurrentWindSpeedMagnitude;


    //
    // Statistics
    //

    PerfStats & mPerfStats;
    std::atomic<RenderStatistics> mRenderStats;
};
