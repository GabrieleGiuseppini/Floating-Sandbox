/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipBuilderShaderSets.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>

namespace ShipBuilder::_detail {

ProgramKind ShaderNameToProgramKind(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "canvas")
        return ProgramKind::Canvas;
    else if (lstr == "circle_overlay")
        return ProgramKind::CircleOverlay;
    else if (lstr == "dashed_line_overlay")
        return ProgramKind::DashedLineOverlay;
    else if (lstr == "grid")
        return ProgramKind::Grid;
    else if (lstr == "matte")
        return ProgramKind::Matte;
    else if (lstr == "mipmapped_texture_quad")
        return ProgramKind::MipMappedTextureQuad;
    else if (lstr == "rect_overlay")
        return ProgramKind::RectOverlay;
    else if (lstr == "structure_mesh")
        return ProgramKind::StructureMesh;
    else if (lstr == "texture")
        return ProgramKind::Texture;
    else if (lstr == "texture_ndc")
        return ProgramKind::TextureNdc;
    else if (lstr == "waterline")
        return ProgramKind::Waterline;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramKindToStr(ProgramKind program)
{
    switch (program)
    {
        case ProgramKind::Canvas:
            return "Canvas";
        case ProgramKind::CircleOverlay:
            return "CircleOverlay";
        case ProgramKind::DashedLineOverlay:
            return "CircleOverlay";
        case ProgramKind::Grid:
            return "Grid";
        case ProgramKind::MipMappedTextureQuad:
            return "MipMappedTextureQuad";
        case ProgramKind::RectOverlay:
            return "RectOverlay";
        case ProgramKind::Matte:
            return "Matte";
        case ProgramKind::StructureMesh:
            return "StructureMesh";
        case ProgramKind::Texture:
            return "Texture";
        case ProgramKind::TextureNdc:
            return "TextureNdc";
        case ProgramKind::Waterline:
            return "Waterline";
    }

    assert(false);
    throw GameException("Unsupported ProgramKind");
}

ProgramParameterKind StrToProgramParameterKind(std::string const & str)
{
    if (str == "CanvasBackgroundColor")
        return ProgramParameterKind::CanvasBackgroundColor;
    else if (str == "Opacity")
        return ProgramParameterKind::Opacity;
    else if (str == "OrthoMatrix")
        return ProgramParameterKind::OrthoMatrix;
    else if (str == "PixelsPerShipParticle")
        return ProgramParameterKind::PixelsPerShipParticle;
    else if (str == "PixelSize")
        return ProgramParameterKind::PixelSize;
    else if (str == "PixelStep")
        return ProgramParameterKind::PixelStep;
    else if (str == "ShipParticleTextureSize")
        return ProgramParameterKind::ShipParticleTextureSize;
    else if (str == "BackgroundTextureUnit")
        return ProgramParameterKind::BackgroundTextureUnit;
    else if (str == "MipMappedTexturesAtlasTexture")
        return ProgramParameterKind::MipMappedTexturesAtlasTexture;
    else if (str == "TextureUnit1")
        return ProgramParameterKind::TextureUnit1;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterKindToStr(ProgramParameterKind programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterKind::CanvasBackgroundColor:
            return "CanvasBackgroundColor";
        case ProgramParameterKind::Opacity:
            return "Opacity";
        case ProgramParameterKind::OrthoMatrix:
            return "OrthoMatrix";
        case ProgramParameterKind::PixelsPerShipParticle:
            return "PixelsPerShipParticle";
        case ProgramParameterKind::PixelSize:
            return "PixelSize";
        case ProgramParameterKind::PixelStep:
            return "PixelStep";
        case ProgramParameterKind::ShipParticleTextureSize:
            return "ShipParticleTextureSize";
        case ProgramParameterKind::BackgroundTextureUnit:
            return "BackgroundTextureUnit";
        case ProgramParameterKind::MipMappedTexturesAtlasTexture:
            return "MipMappedTexturesAtlasTexture";
        case ProgramParameterKind::TextureUnit1:
            return "TextureUnit1";
    }

    assert(false);
    throw GameException("Unsupported ProgramParameterKind");
}

VertexAttributeKind StrToVertexAttributeKind(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Canvas"))
        return VertexAttributeKind::Canvas;
    else if (Utils::CaseInsensitiveEquals(str, "CircleOverlay1"))
        return VertexAttributeKind::CircleOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "CircleOverlay2"))
        return VertexAttributeKind::CircleOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "DashedLineOverlay1"))
        return VertexAttributeKind::DashedLineOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "DashedLineOverlay2"))
        return VertexAttributeKind::DashedLineOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "DebugRegionOverlay1"))
        return VertexAttributeKind::DebugRegionOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "DebugRegionOverlay2"))
        return VertexAttributeKind::DebugRegionOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "Grid1"))
        return VertexAttributeKind::Grid1;
    else if (Utils::CaseInsensitiveEquals(str, "Grid2"))
        return VertexAttributeKind::Grid2;
    else if (Utils::CaseInsensitiveEquals(str, "Matte1"))
        return VertexAttributeKind::Matte1;
    else if (Utils::CaseInsensitiveEquals(str, "Matte2"))
        return VertexAttributeKind::Matte2;
    else if (Utils::CaseInsensitiveEquals(str, "RectOverlay1"))
        return VertexAttributeKind::RectOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "RectOverlay2"))
        return VertexAttributeKind::RectOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "Texture"))
        return VertexAttributeKind::Texture;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNdc"))
        return VertexAttributeKind::TextureNdc;
    else if (Utils::CaseInsensitiveEquals(str, "Waterline1"))
        return VertexAttributeKind::Waterline1;
    else if (Utils::CaseInsensitiveEquals(str, "Waterline2"))
        return VertexAttributeKind::Waterline2;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}