/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderTypes.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

namespace ShipBuilder {

ProgramType ShaderFilenameToProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "canvas")
        return ProgramType::Canvas;
    else if (lstr == "circle_overlay")
        return ProgramType::CircleOverlay;
    else if (lstr == "dashed_line_overlay")
        return ProgramType::DashedLineOverlay;
    else if (lstr == "grid")
        return ProgramType::Grid;
    else if (lstr == "matte")
        return ProgramType::Matte;
    else if (lstr == "mipmapped_texture_quad")
        return ProgramType::MipMappedTextureQuad;
    else if (lstr == "rect_overlay")
        return ProgramType::RectOverlay;
    else if (lstr == "structure_mesh")
        return ProgramType::StructureMesh;
    else if (lstr == "texture")
        return ProgramType::Texture;
    else if (lstr == "texture_ndc")
        return ProgramType::TextureNdc;
    else if (lstr == "waterline")
        return ProgramType::Waterline;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
        case ProgramType::Canvas:
            return "Canvas";
        case ProgramType::CircleOverlay:
            return "CircleOverlay";
        case ProgramType::DashedLineOverlay:
            return "CircleOverlay";
        case ProgramType::Grid:
            return "Grid";
        case ProgramType::MipMappedTextureQuad:
            return "MipMappedTextureQuad";
        case ProgramType::RectOverlay:
            return "RectOverlay";
        case ProgramType::Matte:
            return "Matte";
        case ProgramType::StructureMesh:
            return "StructureMesh";
        case ProgramType::Texture:
            return "Texture";
        case ProgramType::TextureNdc:
            return "TextureNdc";
        case ProgramType::Waterline:
            return "Waterline";
    }

    assert(false);
    throw GameException("Unsupported ProgramType");
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "CanvasBackgroundColor")
        return ProgramParameterType::CanvasBackgroundColor;
    else if (str == "Opacity")
        return ProgramParameterType::Opacity;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "PixelsPerShipParticle")
        return ProgramParameterType::PixelsPerShipParticle;
    else if (str == "PixelSize")
        return ProgramParameterType::PixelSize;
    else if (str == "PixelStep")
        return ProgramParameterType::PixelStep;
    else if (str == "ShipParticleTextureSize")
        return ProgramParameterType::ShipParticleTextureSize;
    else if (str == "BackgroundTextureUnit")
        return ProgramParameterType::BackgroundTextureUnit;
    else if (str == "MipMappedTexturesAtlasTexture")
        return ProgramParameterType::MipMappedTexturesAtlasTexture;
    else if (str == "TextureUnit1")
        return ProgramParameterType::TextureUnit1;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterType::CanvasBackgroundColor:
            return "CanvasBackgroundColor";
        case ProgramParameterType::Opacity:
            return "Opacity";
        case ProgramParameterType::OrthoMatrix:
            return "OrthoMatrix";
        case ProgramParameterType::PixelsPerShipParticle:
            return "PixelsPerShipParticle";
        case ProgramParameterType::PixelSize:
            return "PixelSize";
        case ProgramParameterType::PixelStep:
            return "PixelStep";
        case ProgramParameterType::ShipParticleTextureSize:
            return "ShipParticleTextureSize";
        case ProgramParameterType::BackgroundTextureUnit:
            return "BackgroundTextureUnit";
        case ProgramParameterType::MipMappedTexturesAtlasTexture:
            return "MipMappedTexturesAtlasTexture";
        case ProgramParameterType::TextureUnit1:
            return "TextureUnit1";
    }

    assert(false);
    throw GameException("Unsupported ProgramParameterType");
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Canvas"))
        return VertexAttributeType::Canvas;
    else if (Utils::CaseInsensitiveEquals(str, "CircleOverlay1"))
        return VertexAttributeType::CircleOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "CircleOverlay2"))
        return VertexAttributeType::CircleOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "DashedLineOverlay1"))
        return VertexAttributeType::DashedLineOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "DashedLineOverlay2"))
        return VertexAttributeType::DashedLineOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "DebugRegionOverlay1"))
        return VertexAttributeType::DebugRegionOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "DebugRegionOverlay2"))
        return VertexAttributeType::DebugRegionOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "Grid1"))
        return VertexAttributeType::Grid1;
    else if (Utils::CaseInsensitiveEquals(str, "Grid2"))
        return VertexAttributeType::Grid2;
    else if (Utils::CaseInsensitiveEquals(str, "Matte1"))
        return VertexAttributeType::Matte1;
    else if (Utils::CaseInsensitiveEquals(str, "Matte2"))
        return VertexAttributeType::Matte2;
    else if (Utils::CaseInsensitiveEquals(str, "RectOverlay1"))
        return VertexAttributeType::RectOverlay1;
    else if (Utils::CaseInsensitiveEquals(str, "RectOverlay2"))
        return VertexAttributeType::RectOverlay2;
    else if (Utils::CaseInsensitiveEquals(str, "Texture"))
        return VertexAttributeType::Texture;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNdc"))
        return VertexAttributeType::TextureNdc;
    else if (Utils::CaseInsensitiveEquals(str, "Waterline1"))
        return VertexAttributeType::Waterline1;
    else if (Utils::CaseInsensitiveEquals(str, "Waterline2"))
        return VertexAttributeType::Waterline2;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}