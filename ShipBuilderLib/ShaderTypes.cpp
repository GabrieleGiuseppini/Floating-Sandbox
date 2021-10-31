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
    else if (lstr == "grid")
        return ProgramType::Grid;
    else if (lstr == "texture")
        return ProgramType::Texture;
    else if (lstr == "texture_ndc")
        return ProgramType::TextureNdc;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
        case ProgramType::Canvas:
            return "Canvas";
        case ProgramType::Grid:
            return "Grid";
        case ProgramType::Texture:
            return "Texture";
        case ProgramType::TextureNdc:
            return "TextureNdc";
    }

    assert(false);
    throw GameException("Unsupported ProgramType");
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "Opacity")
        return ProgramParameterType::Opacity;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "PixelSize")
        return ProgramParameterType::PixelSize;
    else if (str == "PixelStep")
        return ProgramParameterType::PixelStep;
    else if (str == "BackgroundTextureUnit")
        return ProgramParameterType::BackgroundTextureUnit;
    else if (str == "TextureUnit1")
        return ProgramParameterType::TextureUnit1;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterType::Opacity:
            return "Opacity";
        case ProgramParameterType::OrthoMatrix:
            return "OrthoMatrix";
        case ProgramParameterType::PixelSize:
            return "PixelSize";
        case ProgramParameterType::PixelStep:
            return "PixelStep";
        case ProgramParameterType::BackgroundTextureUnit:
            return "BackgroundTextureUnit";
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
    else if (Utils::CaseInsensitiveEquals(str, "Grid"))
        return VertexAttributeType::Grid;
    else if (Utils::CaseInsensitiveEquals(str, "Texture"))
        return VertexAttributeType::Texture;
    else if (Utils::CaseInsensitiveEquals(str, "TextureNdc"))
        return VertexAttributeType::TextureNdc;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}