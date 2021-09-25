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
        std::function<void()> swapRenderBuffersFunction,
        ResourceLocator const & resourceLocator);

    int GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    int SetZoom(int zoom)
    {
        auto const newZoom = mViewModel.SetZoom(zoom);

        RefreshOrthoMatrix();

        return newZoom;
    }

    float CalculateIdealZoom() const
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

        RefreshOrthoMatrix();

        return newPos;
    }

    void SetShipSize(ShipSpaceSize const & size)
    {
        mViewModel.SetShipSize(size);
    }

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mViewModel.SetDisplayLogicalSize(logicalSize);

        RefreshOrthoMatrix();
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

    // Sticky
    void UploadStructuralTexture(RgbaImageData const & texture);

    // Sticky
    void UpdateStructuralTextureRegion(
        rgbaColor const * regionPixels,
        int xOffset,
        int yOffset,
        int width,
        int height);

    // TODO: update method as well

public:

    void Render();

private:

    void RefreshOrthoMatrix();

private:

    ViewModel mViewModel;
    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::function<void()> const mSwapRenderBuffersFunction;

    //
    // Types
    //

#pragma pack(push, 1)

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

    // Structural texture
    GameOpenGLVAO mStructuralTextureVAO;
    GameOpenGLVBO mStructuralTextureVBO;
    GameOpenGLTexture mStructuralTextureOpenGLHandle;
    bool mHasStructuralTexture;
};

}