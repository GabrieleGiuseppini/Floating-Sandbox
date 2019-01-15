/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderTraits.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

GPUCalcProgramType ShaderFilenameToGPUCalcProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "pixel_coords")
        return GPUCalcProgramType::PixelCoords;
    else if (lstr == "add")
        return GPUCalcProgramType::Add;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string GPUCalcProgramTypeToStr(GPUCalcProgramType program)
{
    switch (program)
    {
        case GPUCalcProgramType::PixelCoords:
            return "PixelCoords";
        case GPUCalcProgramType::Add:
            return "Add";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcProgramType");
    }
}

GPUCalcProgramParameterType StrToGPUCalcProgramParameterType(std::string const & str)
{
    if (str == "TextureInput0")
        return GPUCalcProgramParameterType::TextureInput0;
    else if (str == "TextureInput1")
        return GPUCalcProgramParameterType::TextureInput1;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string GPUCalcProgramParameterTypeToStr(GPUCalcProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case GPUCalcProgramParameterType::TextureInput0:
            return "TextureInput0";
        case GPUCalcProgramParameterType::TextureInput1:
            return "TextureInput1";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcProgramParameterType");
    }
}

GPUCalcVertexAttributeType StrToGPUCalcVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "VertexShaderInput0"))
        return GPUCalcVertexAttributeType::VertexShaderInput0;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

std::string GPUCalcVertexAttributeTypeToStr(GPUCalcVertexAttributeType vertexAttribute)
{
    switch (vertexAttribute)
    {
        case GPUCalcVertexAttributeType::VertexShaderInput0:
            return "VertexShaderInput0";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcVertexAttributeType");
    }
}