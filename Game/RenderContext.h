/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GlobalRenderContext.h"
#include "NotificationRenderContext.h"
#include "PerfStats.h"
#include "RenderParameters.h"
#include "RenderTypes.h"
#include "ResourceLocator.h"
#include "ShaderTypes.h"
#include "ShipRenderContext.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"
#include "UploadedTextureManager.h"
#include "ViewModel.h"
#include "WorldRenderContext.h"

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/GameOpenGLMappedBuffer.h>
#include <GameOpenGL/ShaderManager.h>

#include <Game/GameParameters.h>

#include <GameCore/AABB.h>
#include <GameCore/BoundedVector.h>
#include <GameCore/Colors.h>
#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>
#include <GameCore/ImageSize.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/SysSpecifics.h>
#include <GameCore/TaskThread.h>
#include <GameCore/Vectors.h>

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Render {

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
        ImageSize const & initialCanvasSize,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction,
        PerfStats & perfStats,
        ResourceLocator const & resourceLocator,
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

    int GetCanvasWidth() const
    {
        return mRenderParameters.View.GetCanvasWidth();
    }

    int GetCanvasHeight() const
    {
        return mRenderParameters.View.GetCanvasHeight();
    }

    void SetCanvasSize(int width, int height)
    {
        mRenderParameters.View.SetCanvasSize(width, height);
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

    //

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

    //

    rgbColor const & GetFlatSkyColor() const
    {
        return mRenderParameters.FlatSkyColor;
    }

    void SetFlatSkyColor(rgbColor const & color)
    {
        mRenderParameters.FlatSkyColor = color;
        // No need to set dirty, this is picked up at each cycle anyway
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

    float GetOceanDarkeningRate() const
    {
        return mRenderParameters.OceanDarkeningRate;
    }

    void SetOceanDarkeningRate(float darkeningRate)
    {
        mRenderParameters.OceanDarkeningRate = darkeningRate;
        mRenderParameters.IsOceanDarkeningRateDirty = true;
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

    //
    // Ship rendering properties
    //

    rgbColor const & GetFlatLampLightColor() const
    {
        return mRenderParameters.FlatLampLightColor;
    }

    void SetFlatLampLightColor(rgbColor const & color)
    {
        mRenderParameters.FlatLampLightColor = color;
        mRenderParameters.IsFlatLampLightColorDirty = true;
    }

    ShipFlameRenderModeType GetShipFlameRenderMode() const
    {
        return mRenderParameters.ShipFlameRenderMode;
    }

    void SetShipFlameRenderMode(ShipFlameRenderModeType shipFlameRenderMode)
    {
        mRenderParameters.ShipFlameRenderMode = shipFlameRenderMode;
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

    bool GetDrawHeatOverlay() const
    {
        return mRenderParameters.DrawHeatOverlay;
    }

    void SetDrawHeatOverlay(bool drawHeatOverlay)
    {
        mRenderParameters.DrawHeatOverlay = drawHeatOverlay;
        // No need to set dirty, this is picked up at each cycle anway
    }

    float GetHeatOverlayTransparency() const
    {
        return mRenderParameters.HeatOverlayTransparency;
    }

    void SetHeatOverlayTransparency(float transparency)
    {
        mRenderParameters.HeatOverlayTransparency = transparency;
        mRenderParameters.IsHeatOverlayTransparencyDirty = true;
    }

    VectorFieldRenderModeType GetVectorFieldRenderMode() const
    {
        return mVectorFieldRenderMode;
    }

    void SetVectorFieldRenderMode(VectorFieldRenderModeType vectorFieldRenderMode)
    {
        mVectorFieldRenderMode = vectorFieldRenderMode;
    }

    float GetVectorFieldLengthMultiplier() const
    {
        return mVectorFieldLengthMultiplier;
    }

    void SetVectorFieldLengthMultiplier(float vectorFieldLengthMultiplier)
    {
        mVectorFieldLengthMultiplier = vectorFieldLengthMultiplier;
    }

    DebugShipRenderModeType GetDebugShipRenderMode() const
    {
        return mRenderParameters.DebugShipRenderMode;
    }

    void SetDebugShipRenderMode(DebugShipRenderModeType debugShipRenderMode)
    {
        mRenderParameters.DebugShipRenderMode = debugShipRenderMode;
        mRenderParameters.IsDebugShipRenderModeDirty = true;
    }

    //
    // Screen <-> World transformations
    //

    inline vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        return mRenderParameters.View.ScreenToWorld(screenCoordinates);
    }

    inline vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
    {
        return mRenderParameters.View.ScreenOffsetToWorldOffset(screenOffset);
    }

    //
    // Statistics
    //

    RenderStatistics GetStatistics() const
    {
        return mRenderStats.load();
    }

public:

    void RebindContext(std::function<void()> rebindContextFunction);

    void Reset();

    void ValidateShipTexture(RgbaImageData const & texture) const;

    void AddShip(
        ShipId shipId,
        size_t pointCount,
        RgbaImageData texture);

    RgbImageData TakeScreenshot();

public:

    void UpdateStart();

    void UpdateEnd();

    void RenderStart();

    void UploadStart();

    inline void UploadStarsStart(size_t starCount)
    {
        mWorldRenderContext->UploadStarsStart(starCount);
    }

    inline void UploadStar(
        float ndcX,
        float ndcY,
        float brightness)
    {
        mWorldRenderContext->UploadStar(
            ndcX,
            ndcY,
            brightness);
    }

    inline void UploadStarsEnd()
    {
        mWorldRenderContext->UploadStarsEnd();
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
        float growthProgress)   // [0.0, +1.0]
    {
        mWorldRenderContext->UploadCloud(
            cloudId,
            virtualX,
            virtualY,
            virtualZ,
            scale,
            darkening,
            growthProgress,
            mRenderParameters);
    }

    inline void UploadCloudsEnd()
    {
        mWorldRenderContext->UploadCloudsEnd();
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

    inline void UploadOceanStart(size_t slices)
    {
        mWorldRenderContext->UploadOceanStart(slices);
    }

    inline void UploadOcean(
        float x,
        float yOcean,
        float oceanDepth)
    {
        mWorldRenderContext->UploadOcean(
            x,
            yOcean,
            oceanDepth,
            mRenderParameters);
    }

    inline void UploadOceanEnd()
    {
        mWorldRenderContext->UploadOceanEnd();
    }

    inline void UploadFishesStart(size_t fishCount)
    {
        mWorldRenderContext->UploadFishesStart(fishCount);
    }

    inline void UploadFish(
        TextureFrameId<FishTextureGroups> const & textureFrameId,
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

    inline void UploadHeatBlasterFlame(
        vec2f const & centerPosition,
        float radius,
        HeatBlasterActionType action)
    {
        mNotificationRenderContext->UploadHeatBlasterFlame(
            centerPosition,
            radius,
            action);
    }

    inline void UploadFireExtinguisherSpray(
        vec2f const & centerPosition,
        float radius)
    {
        mNotificationRenderContext->UploadFireExtinguisherSpray(
            centerPosition,
            radius);
    }

    inline void UploadShipsStart()
    {
        // Nop
    }

    inline void UploadShipStart(
        ShipId shipId,
        PlaneId maxMaxPlaneId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadStart(maxMaxPlaneId);
    }

    inline void UploadShipPointImmutableAttributes(
        ShipId shipId,
        vec2f const * textureCoordinates)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointImmutableAttributes(textureCoordinates);
    }

    inline void UploadShipPointMutableAttributesStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesStart();
    }

    inline void UploadShipPointMutableAttributes(
        ShipId shipId,
        vec2f const * position,
        float const * light,
        float const * water,
        size_t lightAndWaterCount)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributes(
            position,
            light,
            water,
            lightAndWaterCount);
    }

    inline void UploadShipPointMutableAttributesPlaneId(
        ShipId shipId,
        float const * planeId,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesPlaneId(
            planeId,
            startDst,
            count);
    }

    inline void UploadShipPointMutableAttributesDecay(
        ShipId shipId,
        float const * decay,
        size_t startDst,
        size_t count)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesDecay(
            decay,
            startDst,
            count);
    }

    inline void UploadShipPointMutableAttributesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadPointMutableAttributesEnd();
    }

    // Upload is Asynchronous - buffer may not be used until the
    // next UpdateStart
    inline void UploadShipPointColors(
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
    inline void UploadShipPointTemperature(
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
    inline void UploadShipPointFrontierColors(
        ShipId shipId,
        FrontierColor const * colors)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        // Run upload asynchronously
        mRenderThread.QueueTask(
            [=]()
            {
                mShips[shipId]->UploadPointFrontierColors(colors);
            });
    }


    inline void UploadShipElementsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsStart();
    }

    inline void UploadShipElementPoint(
        ShipId shipId,
        int shipPointIndex)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementPoint(shipPointIndex);
    }

    inline void UploadShipElementSpring(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementSpring(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementRope(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementRope(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementTrianglesStart(
        ShipId shipId,
        size_t trianglesCount)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTrianglesStart(trianglesCount);
    }

    inline void UploadShipElementTriangle(
        ShipId shipId,
        size_t triangleIndex,
        int shipPointIndex1,
        int shipPointIndex2,
        int shipPointIndex3)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTriangle(
            triangleIndex,
            shipPointIndex1,
            shipPointIndex2,
            shipPointIndex3);
    }

    inline void UploadShipElementTrianglesEnd(
        ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementTrianglesEnd();
    }

    inline void UploadShipElementsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementsEnd();
    }

    inline void UploadShipElementStressedSpringsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsStart();
    }

    inline void UploadShipElementStressedSpring(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpring(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementStressedSpringsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementStressedSpringsEnd();
    }

    inline void UploadShipElementFrontierEdgesStart(
        ShipId shipId,
        size_t edgesCount)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementFrontierEdgesStart(edgesCount);
    }

    inline void UploadShipElementFrontierEdge(
        ShipId shipId,
        int shipPointIndex1,
        int shipPointIndex2)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementFrontierEdge(
            shipPointIndex1,
            shipPointIndex2);
    }

    inline void UploadShipElementFrontierEdgesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementFrontierEdgesEnd();
    }

    inline void UploadShipFlamesStart(
        ShipId shipId,
        size_t count,
        float windSpeedMagnitude)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlamesStart(count, windSpeedMagnitude);
    }

    inline void UploadShipBackgroundFlame(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadBackgroundFlame(
            planeId,
            baseCenterPosition,
            flameVector,
            scale,
            flamePersonalitySeed,
            mRenderParameters);
    }

    inline void UploadShipForegroundFlame(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & baseCenterPosition,
        vec2f const & flameVector,
        float scale,
        float flamePersonalitySeed)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadForegroundFlame(
            planeId,
            baseCenterPosition,
            flameVector,
            scale,
            flamePersonalitySeed,
            mRenderParameters);
    }

    inline void UploadShipFlamesEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadFlamesEnd();
    }

    inline void UploadShipExplosion(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        ExplosionType explosionType,
        float personalitySeed,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadExplosion(
            planeId,
            centerPosition,
            halfQuadSize,
            explosionType,
            personalitySeed,
            progress);
    }

    inline void UploadShipSparkle(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & position,
        vec2f const & velocityVector,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadSparkle(
            planeId,
            position,
            velocityVector,
            progress);
    }

    inline void UploadShipAirBubble(
        ShipId shipId,
        PlaneId planeId,
        vec2f const & position,
        float scale,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadAirBubble(
            planeId,
            position,
            scale,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        vec2f const & rotationBase,
        vec2f const & rotationOffset,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            rotationBase,
            rotationOffset,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        TextureFrameId<GenericMipMappedTextureGroups> const & textureFrameId,
        vec2f const & position,
        float scale,
        float angle,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            textureFrameId,
            position,
            scale,
            angle,
            alpha);
    }

    inline void UploadShipGenericMipMappedTextureRenderSpecification(
        ShipId shipId,
        PlaneId planeId,
        float personalitySeed,
        GenericMipMappedTextureGroups textureGroup,
        vec2f const & position,
        float scale,
        float alpha)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadGenericMipMappedTextureRenderSpecification(
            planeId,
            personalitySeed,
            textureGroup,
            position,
            scale,
            alpha);
    }

    inline void UploadShipElementEphemeralPointsStart(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsStart();
    }

    inline void UploadShipElementEphemeralPoint(
        ShipId shipId,
        int pointIndex)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPoint(
            pointIndex);
    }

    inline void UploadShipElementEphemeralPointsEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadElementEphemeralPointsEnd();
    }

    inline void UploadShipHighlight(
        ShipId shipId,
        HighlightModeType highlightMode,
        PlaneId planeId,
        vec2f const & centerPosition,
        float halfQuadSize,
        rgbColor color,
        float progress)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadHighlight(
            highlightMode,
            planeId,
            centerPosition,
            halfQuadSize,
            color,
            progress);
    }

    inline void UploadShipVectors(
        ShipId shipId,
        size_t count,
        vec2f const * position,
        float const * planeId,
        vec2f const * vector,
        float lengthAdjustment,
        vec4f const & color)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadVectors(
            count,
            position,
            planeId,
            vector,
            lengthAdjustment * mVectorFieldLengthMultiplier,
            color);
    }

    inline void UploadShipEnd(ShipId shipId)
    {
        assert(shipId >= 0 && shipId < mShips.size());

        mShips[shipId]->UploadEnd();
    }

    inline void UploadShipsEnd()
    {
        // Nop
    }

    inline void UploadTextNotificationStart(FontType fontType)
    {
        mNotificationRenderContext->UploadTextNotificationStart(fontType);
    }

    inline void UploadTextNotificationLine(
        FontType font,
        std::string const & text,
        AnchorPositionType anchor,
        vec2f const & screenOffset,
        float alpha)
    {
        mNotificationRenderContext->UploadTextNotificationLine(
            font,
            text,
            anchor,
            screenOffset,
            alpha);
    }

    inline void UploadTextNotificationEnd(FontType fontType)
    {
        mNotificationRenderContext->UploadTextNotificationEnd(fontType);
    }

    inline void UploadTextureNotificationStart()
    {
        mNotificationRenderContext->UploadTextureNotificationStart();
    }

    inline void UploadTextureNotification(
        TextureFrameId<GenericLinearTextureGroups> const & textureFrameId,
        AnchorPositionType anchor,
        vec2f const & screenOffset, // In texture-size fraction (0.0 -> 1.0)
        float alpha)
    {
        mNotificationRenderContext->UploadTextureNotification(
            textureFrameId,
            anchor,
            screenOffset,
            alpha);
    }

    inline void UploadTextureNotificationEnd()
    {
        mNotificationRenderContext->UploadTextureNotificationEnd();
    }

    void UploadEnd();

    void Draw();

    void RenderEnd();

private:

    void ProcessParameterChanges(RenderParameters const & renderParameters);

    void ApplyCanvasSizeChanges(RenderParameters const & renderParameters);

    void ApplyDebugShipRenderModeChanges(RenderParameters const & renderParameters);

    static float CalculateEffectiveAmbientLightIntensity(
        float ambientLightIntensity,
        float stormAmbientDarkening);

    vec4f CalculateShipWaterColor() const;

private:

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

    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;

    //
    // Child contextes
    //

    std::unique_ptr<GlobalRenderContext> mGlobalRenderContext;
    std::unique_ptr<WorldRenderContext> mWorldRenderContext;
    std::vector<std::unique_ptr<ShipRenderContext>> mShips;
    std::unique_ptr<NotificationRenderContext> mNotificationRenderContext;

    //
    // Externally-controlled parameters that only affect Upload (i.e. that do
    // not affect rendering directly), or that purely serve as input to calculated
    // render parameters, or that only need storage here (e.g. being used in other
    // contexts to control upload's)
    //

    float mAmbientLightIntensity;
    float mShipFlameSizeAdjustment;
    rgbColor mShipDefaultWaterColor;
    VectorFieldRenderModeType mVectorFieldRenderMode;
    float mVectorFieldLengthMultiplier;


    //
    // Rendering externals
    //

    std::function<void()> const mSwapRenderBuffersFunction;


    //
    // Render parameters
    //

    RenderParameters mRenderParameters;

    //
    // Statistics
    //

    PerfStats & mPerfStats;
    std::atomic<RenderStatistics> mRenderStats;
};

}