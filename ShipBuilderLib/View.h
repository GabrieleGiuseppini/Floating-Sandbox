/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShaderTypes.h"
#include "ShipBuilderTypes.h"
#include "ViewModel.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <GameCore/Colors.h>
#include <GameCore/ImageData.h>

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <array>
#include <functional>
#include <memory>
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
        ShipSpaceSize initialShipSpaceSize,
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor,
        std::function<void()> swapRenderBuffersFunction,
        ResourceLocator const & resourceLocator);

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

    ShipSpaceSize GetVisibleShipSpaceSize() const
    {
        return mViewModel.GetVisibleShipSpaceSize();
    }

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToShipSpace(displayCoordinates);
    }

public:

    void EnableVisualGrid(bool doEnable);

    void SetPrimaryLayer(LayerType value)
    {
        mPrimaryLayer = value;
    }

    float GetOtherLayersOpacity() const
    {
        return mOtherLayersOpacity;
    }

    void SetOtherLayersOpacity(float value)
    {
        mOtherLayersOpacity = value;
    }

    // Sticky, always drawn
    void UploadBackgroundTexture(RgbaImageData && texture);

    //
    // Structural (all sticky)
    //

    void UploadStructuralLayerVisualizationTexture(RgbaImageData const & texture);

    void UpdateStructuralLayerVisualizationTexture(
        rgbaColor const * regionPixels,
        int xOffset,
        int yOffset,
        int width,
        int height);

    //
    // Electrical (all sticky)
    //

    void UploadElectricalLayerVisualizationTexture(RgbaImageData const & texture);

    void UpdateElectricalLayerVisualizationTexture(
        rgbaColor const * regionPixels,
        int xOffset,
        int yOffset,
        int width,
        int height);

    void RemoveElectricalLayerVisualizationTexture();

    bool HasElectricalLayerVisualizationTexture() const
    {
        return mHasElectricalTexture;
    }

    //
    // Ropes (all sticky)
    //

    void UploadRopesLayerVisualization(RopeBuffer const & ropeBuffer);

    void RemoveRopesLayerVisualization();

    bool HasRopesLayerVisualization() const
    {
        return mRopeCount > 0;
    }

    //
    // Overlays (all sticky)
    //

    enum class OverlayMode
    {
        Default,
        Error
    };

    void UploadCircleOverlay(
        ShipSpaceCoordinates const & center,
        OverlayMode mode);

    void RemoveCircleOverlay();

    void UploadRectOverlay(
        ShipSpaceRect const & rect,
        OverlayMode mode);

    void RemoveRectOverlay();

public:

    void Render();

private:

    void OnViewModelUpdated();

    void UpdateCanvas();
    void UpdateGrid();
    void UpdateCircleOverlay();
    void UpdateRectOverlay();

    void RenderRopes();

    vec3f GetOverlayColor(OverlayMode mode) const;

private:

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
        vec2f positionShip; // Ship space
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

#pragma pack(pop)

    //
    // Rendering
    //

    // Background texture
    GameOpenGLVAO mBackgroundTextureVAO;
    GameOpenGLVBO mBackgroundTextureVBO;
    GameOpenGLTexture mBackgroundTextureOpenGLHandle;
    bool mHasBackgroundTexture;

    // Canvas
    GameOpenGLVAO mCanvasVAO;
    GameOpenGLVBO mCanvasVBO;

    // Structural texture
    GameOpenGLVAO mStructuralTextureVAO;
    GameOpenGLVBO mStructuralTextureVBO;
    GameOpenGLTexture mStructuralTextureOpenGLHandle;
    bool mHasStructuralTexture;

    // Electrical texture
    GameOpenGLVAO mElectricalTextureVAO;
    GameOpenGLVBO mElectricalTextureVBO;
    GameOpenGLTexture mElectricalTextureOpenGLHandle;
    bool mHasElectricalTexture;

    // Ropes visualization
    GameOpenGLVAO mRopesVAO;
    GameOpenGLVBO mRopesVBO;
    size_t mRopeCount;

    // Layers opacity
    float mOtherLayersOpacity;

    // Grid
    GameOpenGLVAO mGridVAO;
    GameOpenGLVBO mGridVBO;
    bool mIsGridEnabled;

    // CircleOverlay
    GameOpenGLVAO mCircleOverlayVAO;
    GameOpenGLVBO mCircleOverlayVBO;
    ShipSpaceCoordinates mCircleOverlayCenter;
    vec3f mCircleOverlayColor;
    bool mHasCircleOverlay;

    // RectOverlay
    GameOpenGLVAO mRectOverlayVAO;
    GameOpenGLVBO mRectOverlayVBO;
    ShipSpaceRect mRectOverlayRect;
    vec3f mRectOverlayColor;
    bool mHasRectOverlay;

    //
    // Settings from outside
    //

    LayerType mPrimaryLayer;
};

}