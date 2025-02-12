/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GPUCalcShaderSets.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>

namespace GPUCalcShaderSets::_detail {

ProgramKind ShaderNameToProgramKind(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "pixel_coords")
        return ProgramKind::PixelCoords;
    else if (lstr == "add")
        return ProgramKind::Add;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramKindToStr(ProgramKind program)
{
    switch (program)
    {
        case ProgramKind::PixelCoords:
            return "PixelCoords";
        case ProgramKind::Add:
            return "Add";
        default:
            assert(false);
            throw GameException("Unsupported ProgramKind");
    }
}

ProgramParameterKind StrToProgramParameterKind(std::string const & str)
{
    if (str == "TextureInput0")
        return ProgramParameterKind::TextureInput0;
    else if (str == "TextureInput1")
        return ProgramParameterKind::TextureInput1;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterKindToStr(ProgramParameterKind programParameter)
{
    switch (programParameter)
    {
        case ProgramParameterKind::TextureInput0:
            return "TextureInput0";
        case ProgramParameterKind::TextureInput1:
            return "TextureInput1";
        default:
            assert(false);
            throw GameException("Unsupported ProgramParameterKindToStr");
    }
}

VertexAttributeKind StrToVertexAttributeKind(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "VertexShaderInput0"))
        return VertexAttributeKind::VertexShaderInput0;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

std::string VertexAttributeKindToStr(VertexAttributeKind vertexAttribute)
{
    switch (vertexAttribute)
    {
        case VertexAttributeKind::VertexShaderInput0:
            return "VertexShaderInput0";
        default:
            assert(false);
            throw GameException("Unsupported VertexAttributeKind");
    }
}

}