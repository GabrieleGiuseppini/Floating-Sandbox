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
 */
class View
{
public:

    View(
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

    WorkSpaceCoordinates const & GetCameraWorkSpacePosition() const
    {
        return mViewModel.GetCameraWorkSpacePosition();
    }

    WorkSpaceCoordinates SetCameraWorkSpacePosition(WorkSpaceCoordinates const & pos)
    {
        auto const newPos = mViewModel.SetCameraWorkSpacePosition(pos);

        RefreshOrthoMatrix();

        return newPos;
    }

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mViewModel.SetDisplayLogicalSize(logicalSize);

        RefreshOrthoMatrix();
    }

    WorkSpaceSize GetCameraPanRange() const
    {
        return mViewModel.GetCameraPanRange();
    }

    WorkSpaceCoordinates DisplayLogicalToWorkSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.DisplayLogicalToWorkSpace(displayCoordinates);
    }

public:

    //
    // Structural Render Color Texture
    //

    // Sticky, always drawn
    void UploadStructuralRenderColorTexture(RgbaImageData const & texture);

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
        vec2f positionWork; // Work space
        vec2f textureCoords; // Texture space

        TextureVertex() = default;

        TextureVertex(
            vec2f const & _positionWork,
            vec2f _textureCoords)
            : positionWork(_positionWork)
            , textureCoords(_textureCoords)
        {}
    };

#pragma pack(pop)


    //
    // Rendering
    //

    // Structural Render Color Texture

    GameOpenGLVAO mStructuralRenderTextureColorVAO;
    GameOpenGLVBO mStructuralRenderTextureColorVBO;
    GameOpenGLTexture mStructuralRenderTextureOpenGLHandle;
    bool mHasStructuralRenderColorTexture;
};

}