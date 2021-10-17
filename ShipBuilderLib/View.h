/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShaderTypes.h"
#include "ShipBuilderTypes.h"
#include "ViewModel.h"

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
        bool isGridEnabled,
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

public:

    void Render();

private:

    void OnViewModelUpdated();

    void UpdateCanvasBorder();
    void UpdateGrid();

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

        GridVertex() = default;

        GridVertex(
            vec2f const & _positionShip,
            vec2f _positionPixel)
            : positionShip(_positionShip)
            , positionPixel(_positionPixel)
        {}
    };

    struct CanvasBorderVertex
    {
        vec2f positionShip; // Ship space

        CanvasBorderVertex() = default;

        CanvasBorderVertex(
            float _x,
            float _y)
            : positionShip(_x, _y)
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

#pragma pack(pop)

    //
    // Rendering
    //

    // Background texture
    GameOpenGLVAO mBackgroundTextureVAO;
    GameOpenGLVBO mBackgroundTextureVBO;
    GameOpenGLTexture mBackgroundTextureOpenGLHandle;
    bool mHasBackgroundTexture;

    // Canvas border
    GameOpenGLVAO mCanvasBorderVAO;
    GameOpenGLVBO mCanvasBorderVBO;

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

    // Grid
    GameOpenGLVAO mGridVAO;
    GameOpenGLVBO mGridVBO;
    bool mIsGridEnabled;
};

}