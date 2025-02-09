/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <OpenGLCore/GameOpenGL.h>

#include <cstdint>
#include <string>

namespace ShipBuilder {

//
// Shaders
//

enum class ProgramKind
{
    Canvas = 0,
    CircleOverlay,
    DashedLineOverlay,
    Grid,
    Matte,
    MipMappedTextureQuad,
    RectOverlay,
    StructureMesh,
    Texture,
    TextureNdc,
    Waterline,

    _Last = Waterline
};

namespace _detail
{

ProgramKind ShaderNameToProgramKind(std::string const & str);

std::string ProgramKindToStr(ProgramKind program);

}

enum class ProgramParameterKind : uint8_t
{
    CanvasBackgroundColor = 0,
    Opacity,
    OrthoMatrix,
    PixelsPerShipParticle,
    PixelSize,
    PixelStep,
    ShipParticleTextureSize,

    // Texture units
    BackgroundTextureUnit,
    MipMappedTexturesAtlasTexture,
    TextureUnit1,

    _FirstTexture = BackgroundTextureUnit,
    _LastTexture = TextureUnit1
};

namespace _detail
{

ProgramParameterKind StrToProgramParameterKind(std::string const & str);

std::string ProgramParameterKindToStr(ProgramParameterKind programParameter);

}

/*
 * This enum serves merely to associate a vertex attribute index to each vertex attribute name.
 */
enum class VertexAttributeKind : GLuint
{
    Canvas = 0,

    CircleOverlay1 = 0,
    CircleOverlay2 = 1,

    DashedLineOverlay1 = 0,
    DashedLineOverlay2 = 1,

    DebugRegionOverlay1 = 0,
    DebugRegionOverlay2 = 1,

    Grid1 = 0,
    Grid2 = 1,

    Matte1 = 0,
    Matte2 = 1,

    RectOverlay1 = 0,
    RectOverlay2 = 1,

    Texture = 0,

    TextureNdc = 0,

    Waterline1 = 0,
    Waterline2 = 1
};

namespace _detail
{

VertexAttributeKind StrToVertexAttributeKind(std::string const & str);

}

struct ShaderSet
{
    using ProgramKindType = ShipBuilder::ProgramKind;
    using ProgramParameterKindType = ShipBuilder::ProgramParameterKind;
    using VertexAttributeKindType = ShipBuilder::VertexAttributeKind;

    static inline std::string ShaderSetName = "ShipBuilder";

    static constexpr auto ShaderNameToProgramKind = _detail::ShaderNameToProgramKind;
    static constexpr auto ProgramKindToStr = _detail::ProgramKindToStr;
    static constexpr auto StrToProgramParameterKind = _detail::StrToProgramParameterKind;
    static constexpr auto ProgramParameterKindToStr = _detail::ProgramParameterKindToStr;
    static constexpr auto StrToVertexAttributeKind = _detail::StrToVertexAttributeKind;
};

}
