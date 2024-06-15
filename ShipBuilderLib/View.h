/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "OpenGLManager.h"
#include "ShaderTypes.h"
#include "ShipBuilderTypes.h"
#include "TextureTypes.h"
#include "ViewModel.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>
#include <Game/TextureAtlas.h>

#include <GameCore/Colors.h>
#include <GameCore/ImageData.h>

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * This class is the entry point of the entire OpenGL rendering subsystem, providing
 * the API for rendering, which is agnostic about the render platform implementation.
 *
 * All uploads are sticky, and thus need to be explicitly "undone" when they shouldn't
 * be drawn anymore.
 */
class View
{
public:

    View(
        ShipSpaceSize shipSpaceSize,
        rgbColor const & canvasBackgroundColor,
        VisualizationType primaryVisualization,
        float otherVisualizationsOpacity,
        bool isGridEnabled,
        DisplayLogicalSize displaySize,
        int logicalToPhysicalPixelFactor,
        OpenGLManager & openGLManager,
        std::function<void()> swapRenderBuffersFunction,
        ResourceLocator const & resourceLocator);

    ViewModel const & GetViewModel() const
    {
        return mViewModel;
    }

    int GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    int SetZoom(int zoom)
    {
        auto const newZoom = mViewModel.SetZoom(zoom);

        OnViewModelUpdated();

        return newZoom;
    }

    int CalculateIdealZoom() const
    {
        return mViewModel.CalculateIdealZoom();
    }

    ShipSpaceCoordinates const & GetCameraShipSpacePosition() const
    {
        return mViewModel.GetCameraShipSpacePosition();
    }

    ShipSpaceCoordinates SetCameraShipSpacePosition(ShipSpaceCoordinates const & pos)
    {
        auto const newPos = mViewModel.SetCameraShipSpacePosition(pos);

        OnViewModelUpdated();

        return newPos;
    }

    void SetShipSize(ShipSpaceSize const & size)
    {
        mViewModel.SetShipSize(size);

        OnViewModelUpdated();
    }

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mViewModel.SetDisplayLogicalSize(logicalSize);

        OnViewModelUpdated();
    }

    ShipSpaceSize GetCameraRange() const
    {
        return mViewModel.GetCameraRange();
    }

    ShipSpaceSize GetCameraThumbSize() const
    {
        return mViewModel.GetCameraThumbSize();
    }

    ShipSpaceSize GetDisplayShipSpaceSize() const
    {
        return mViewModel.GetDisplayShipSpaceSize();
    }

    ShipSpaceRect GetDisplayShipSpaceRect() const
    {
        return mViewModel.GetDisplayShipSpaceRect();
    }

    DisplayPhysicalRect GetPhysicalVisibleShipRegion() const
    {
        return mViewModel.GetPhysicalVisibleShipRegion();
    }

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToShipSpace(displayCoordinates);
    }

    ShipSpaceCoordinates ScreenToShipSpaceNearest(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToShipSpaceNearest(displayCoordinates);
    }

    ImageCoordinates ScreenToExteriorTextureSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToExteriorTextureSpace(displayCoordinates);
    }

    ImageCoordinates ScreenToInteriorTextureSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToInteriorTextureSpace(displayCoordinates);
    }

public:

    void SetCanvasBackgroundColor(rgbColor const & color);

    void SetPrimaryVisualization(VisualizationType visualization)
    {
        mPrimaryVisualization = visualization;
    }

    void SetOtherVisualizationsOpacity(float value)
    {
        mOtherVisualizationsOpacity = value;
    }

    void EnableVisualGrid(bool doEnable);

    // Sticky, always drawn
    void UploadBackgroundTexture(RgbaImageData && texture);

    //
    // Game viz (all sticky)
    //

    void UploadGameVisualization(RgbaImageData const & texture);

    void UpdateGameVisualization(
        RgbaImageData const & subTexture,
        ImageCoordinates const & origin);

    void RemoveGameVisualization();

    bool HasGameVisualization() const
    {
        return mHasGameVisualization;
    }

    //
    // Structural layer viz (all sticky)
    //

    enum class StructuralLayerVisualizationDrawMode
    {
        MeshMode,
        PixelMode
    };

    void SetStructuralLayerVisualizationDrawMode(StructuralLayerVisualizationDrawMode mode);

    void UploadStructuralLayerVisualization(RgbaImageData const & texture);

    void UpdateStructuralLayerVisualization(
        RgbaImageData const & subTexture,
        ImageCoordinates const & origin);

    void RemoveStructuralLayerVisualization();

    bool HasStructuralLayerVisualization() const
    {
        return mHasStructuralLayerVisualization;
    }

    //
    // Electrical layer viz (all sticky)
    //

    void UploadElectricalLayerVisualization(RgbaImageData const & texture);

    void RemoveElectricalLayerVisualization();

    bool HasElectricalLayerVisualization() const
    {
        return mHasElectricalLayerVisualization;
    }

    //
    // Ropes layer viz (all sticky)
    //

    void UploadRopesLayerVisualization(RopeBuffer const & ropeBuffer);

    void RemoveRopesLayerVisualization();

    bool HasRopesLayerVisualization() const
    {
        return mRopeCount > 0;
    }

    //
    // Exterior Texture layer viz (all sticky)
    //

    void UploadExteriorTextureLayerVisualization(RgbaImageData const & texture);

    void UpdateExteriorTextureLayerVisualization(
        RgbaImageData const & subTexture,
        ImageCoordinates const & origin);

    void RemoveExteriorTextureLayerVisualization();

    bool HasExteriorTextureLayerVisualization() const
    {
        return mHasExteriorTextureLayerVisualization;
    }

    //
    // Interior Texture layer viz (all sticky)
    //

    void UploadInteriorTextureLayerVisualization(RgbaImageData const & texture);

    void UpdateInteriorTextureLayerVisualization(
        RgbaImageData const & subTexture,
        ImageCoordinates const & origin);

    void RemoveInteriorTextureLayerVisualization();

    bool HasInteriorTextureLayerVisualization() const
    {
        return mHasInteriorTextureLayerVisualization;
    }

    //
    // Overlays (all sticky)
    //

    void UploadDebugRegionOverlay(ShipSpaceRect const & rect);

    void RemoveDebugRegionOverlays();

    enum class OverlayMode
    {
        Default,
        Error
    };

    void UploadCircleOverlay(
        ShipSpaceCoordinates const & center,
        OverlayMode mode);

    void RemoveCircleOverlay();

    // Ship space
    void UploadRectOverlay(
        ShipSpaceRect const & rect,
        OverlayMode mode);

    // Exterior Texture space
    void UploadRectOverlayExteriorTexture(
        ImageRect const & rect,
        OverlayMode mode);

    // Interior Texture space
    void UploadRectOverlayInteriorTexture(
        ImageRect const & rect,
        OverlayMode mode);

    void RemoveRectOverlay();

    void UploadDashedLineOverlay(
        ShipSpaceCoordinates const & start,
        ShipSpaceCoordinates const & end,
        OverlayMode mode);

    void RemoveDashedLineOverlay();

    void UploadDashedRectangleOverlay(
        ShipSpaceCoordinates const & cornerA,
        ShipSpaceCoordinates const & cornerB);

    void RemoveDashedRectangleOverlay();

    //
    // Misc
    //

    enum class WaterlineMarkerType
    {
        CenterOfBuoyancy,
        CenterOfMass
    };

    void UploadWaterlineMarker(
        vec2f const & center, // Ship space coords
        WaterlineMarkerType type);

    void RemoveWaterlineMarker(WaterlineMarkerType type);

    void UploadWaterline(
        vec2f const & center, // Ship space coords
        vec2f const & waterDirection);

    void RemoveWaterline();

public:

    void Render();

private:

    void OnViewModelUpdated();

    void UpdateStructuralLayerVisualizationParameters();

    void UpdateBackgroundTexture(ImageSize const & textureSize);
    void UpdateCanvas();
    void UpdateGrid();
    void UpdateCircleOverlay();
    void UpdateRectOverlay();
    void UpdateDashedLineOverlay();
    void UpdateDashedRectangleOverlay();

    inline void UploadTextureVerticesTriangleStripQuad(
        float leftXShip, float leftXTex,
        float rightXShip, float rightTex,
        float bottomYShip, float bottomYTex,
        float topYShip, float topYTex,
        GameOpenGLVBO const & vbo);

    void UploadDebugRegionOverlayVertexBuffer();

    void RenderGameVisualization();
    void RenderStructuralLayerVisualization();
    void RenderElectricalLayerVisualization();
    void RenderRopesLayerVisualization();
    void RenderExteriorTextureLayerVisualization();
    void RenderInteriorTextureLayerVisualization();

    vec3f GetOverlayColor(OverlayMode mode) const;

private:

    std::unique_ptr<OpenGLContext> mOpenGLContext; // Just placeholder
    ViewModel mViewModel;
    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::function<void()> const mSwapRenderBuffersFunction;

    //
    // Types
    //

#pragma pack(push, 1)

    struct GridVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionPixel; // Pixel space
        float midXPixel; // Pixel space

        GridVertex() = default;

        GridVertex(
            vec2f const & _positionShip,
            vec2f _positionPixel,
            float _midXPixel)
            : positionShip(_positionShip)
            , positionPixel(_positionPixel)
            , midXPixel(_midXPixel)
        {}
    };

    struct CanvasVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionNorm; //  0->1

        CanvasVertex() = default;

        CanvasVertex(
            vec2f _positionShip,
            vec2f _positionNorm)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
        {}
    };

    struct TextureVertex
    {
        vec2f positionShip; // Ship space
        vec2f textureCoords; // Texture space

        TextureVertex() = default;

        TextureVertex(
            vec2f const & _positionShip,
            vec2f _textureCoords)
            : positionShip(_positionShip)
            , textureCoords(_textureCoords)
        {}
    };

    struct TextureNdcVertex
    {
        vec2f positionNdc;
        vec2f textureCoords; // Texture space

        TextureNdcVertex() = default;

        TextureNdcVertex(
            vec2f const & _positionNdc,
            vec2f _textureCoords)
            : positionNdc(_positionNdc)
            , textureCoords(_textureCoords)
        {}
    };

    struct RopeVertex
    {
        vec2f positionShip; // Ship space
        vec4f color;

        RopeVertex() = default;

        RopeVertex(
            vec2f const & _positionShip,
            vec4f _color)
            : positionShip(_positionShip)
            , color(_color)
        {}
    };

    struct DebugRegionOverlayVertex
    {
        vec2f positionShip;
        vec4f overlayColor;

        DebugRegionOverlayVertex() = default;

        DebugRegionOverlayVertex(
            vec2f _positionShip,
            vec4f _overlayColor)
            : positionShip(_positionShip)
            , overlayColor(_overlayColor)
        {}
    };

    struct CircleOverlayVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionNorm; //  0->1
        vec3f overlayColor;

        CircleOverlayVertex() = default;

        CircleOverlayVertex(
            vec2f _positionShip,
            vec2f _positionNorm,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
            , overlayColor(_overlayColor)
        {}
    };

    struct RectOverlayVertex
    {
        vec2f positionShip; // Ship space - also fractional
        vec2f positionNorm; //  0->1
        vec3f overlayColor;

        RectOverlayVertex() = default;

        RectOverlayVertex(
            vec2f _positionShip,
            vec2f _positionNorm,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
            , overlayColor(_overlayColor)
        {}
    };

    struct DashedLineOverlayVertex
    {
        vec2f positionShip; // Ship space
        float pixelCoord; //  PixelSpace
        vec3f overlayColor;

        DashedLineOverlayVertex() = default;

        DashedLineOverlayVertex(
            vec2f _positionShip,
            float _pixelCoord,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , pixelCoord(_pixelCoord)
            , overlayColor(_overlayColor)
        {}
    };

    struct WaterlineVertex
    {
        vec2f positionShip; // Ship space
        vec2f centerShip; // Ship space
        vec2f direction;

        WaterlineVertex() = default;

        WaterlineVertex(
            vec2f _positionShip,
            vec2f _centerShip,
            vec2f _direction)
            : positionShip(_positionShip)
            , centerShip(_centerShip)
            , direction(_direction)
        {}
    };

#pragma pack(pop)

    //
    // Rendering
    //

    // Background texture
    GameOpenGLVAO mBackgroundTextureVAO;
    GameOpenGLVBO mBackgroundTextureVBO;
    GameOpenGLTexture mBackgroundTexture;
    std::optional<ImageSize> mBackgroundTextureSize; // Also works as indicator

    // Canvas
    GameOpenGLVAO mCanvasVAO;
    GameOpenGLVBO mCanvasVBO;

    // Game visualization
    GameOpenGLVAO mGameVisualizationVAO;
    GameOpenGLVBO mGameVisualizationVBO;
    GameOpenGLTexture mGameVisualizationTexture;
    bool mHasGameVisualization;

    // Structural layer visualization
    GameOpenGLVAO mStructuralLayerVisualizationVAO;
    GameOpenGLVBO mStructuralLayerVisualizationVBO;
    GameOpenGLTexture mStructuralLayerVisualizationTexture;
    bool mHasStructuralLayerVisualization;
    ProgramType mStructuralLayerVisualizationShader;

    // Electrical layer visualization
    GameOpenGLVAO mElectricalLayerVisualizationVAO;
    GameOpenGLVBO mElectricalLayerVisualizationVBO;
    GameOpenGLTexture mElectricalLayerVisualizationTexture;
    bool mHasElectricalLayerVisualization;

    // Ropes layer visualization
    GameOpenGLVAO mRopesVAO;
    GameOpenGLVBO mRopesVBO;
    size_t mRopeCount;

    // Exterior Texture layer visualization
    GameOpenGLVAO mExteriorTextureLayerVisualizationVAO;
    GameOpenGLVBO mExteriorTextureLayerVisualizationVBO;
    GameOpenGLTexture mExteriorTextureLayerVisualizationTexture;
    bool mHasExteriorTextureLayerVisualization;

    // Interior Texture layer visualization
    GameOpenGLVAO mInteriorTextureLayerVisualizationVAO;
    GameOpenGLVBO mInteriorTextureLayerVisualizationVBO;
    GameOpenGLTexture mInteriorTextureLayerVisualizationTexture;
    bool mHasInteriorTextureLayerVisualization;

    // Grid
    GameOpenGLVAO mGridVAO;
    GameOpenGLVBO mGridVBO;
    bool mIsGridEnabled;

    // DebugRegionOverlay
    GameOpenGLVAO mDebugRegionOverlayVAO;
    GameOpenGLVBO mDebugRegionOverlayVBO;
    std::vector<DebugRegionOverlayVertex> mDebugRegionOverlayVertexBuffer;
    bool mIsDebugRegionOverlayBufferDirty;

    // CircleOverlay
    GameOpenGLVAO mCircleOverlayVAO;
    GameOpenGLVBO mCircleOverlayVBO;
    ShipSpaceCoordinates mCircleOverlayCenter;
    vec3f mCircleOverlayColor;
    bool mHasCircleOverlay;

    // RectOverlay
    GameOpenGLVAO mRectOverlayVAO;
    GameOpenGLVBO mRectOverlayVBO;
    std::optional<ShipSpaceRect> mRectOverlayShipSpaceRect;
    std::optional<ImageRect> mRectOverlayExteriorTextureSpaceRect;
    std::optional<ImageRect> mRectOverlayInteriorTextureSpaceRect;
    vec3f mRectOverlayColor;

    // DashedLineOverlay
    GameOpenGLVAO mDashedLineOverlayVAO;
    GameOpenGLVBO mDashedLineOverlayVBO;
    std::vector<std::pair<ShipSpaceCoordinates, ShipSpaceCoordinates>> mDashedLineOverlaySet;
    vec3f mDashedLineOverlayColor;

    // DashedRectangleOverlay
    GameOpenGLVAO mDashedRectangleOverlayVAO;
    GameOpenGLVBO mDashedRectangleOverlayVBO;
    std::optional<std::pair<ShipSpaceCoordinates, ShipSpaceCoordinates>> mDashedRectangleOverlayRect;

    // Waterline markers
    GameOpenGLVAO mWaterlineMarkersVAO;
    GameOpenGLVBO mWaterlineMarkersVBO;
    bool mHasCenterOfBuoyancyWaterlineMarker;
    bool mHasCenterOfMassWaterlineMarker;

    // Waterline
    GameOpenGLVAO mWaterlineVAO;
    GameOpenGLVBO mWaterlineVBO;
    bool mHasWaterline;

    //
    // Textures
    //

    GameOpenGLTexture mMipMappedTextureAtlasOpenGLHandle;
    std::unique_ptr<Render::TextureAtlasMetadata<MipMappedTextureGroups>> mMipMappedTextureAtlasMetadata;

    //
    // Settings from outside
    //

    VisualizationType mPrimaryVisualization;
    float mOtherVisualizationsOpacity;
};

}