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
    if (lstr == "test")
        return GPUCalcProgramType::Test;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string GPUCalcProgramTypeToStr(GPUCalcProgramType program)
{
    switch (program)
    {
        case GPUCalcProgramType::Test:
            return "Test";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcProgramType");
    }
}

GPUCalcProgramParameterType StrToGPUCalcProgramParameterType(std::string const & str)
{
    if (str == "TODO")
        return GPUCalcProgramParameterType::TODO;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string GPUCalcProgramParameterTypeToStr(GPUCalcProgramParameterType programParameter)
{
    switch (programParameter)
    {
        case GPUCalcProgramParameterType::TODO:
            return "TODO";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcProgramParameterType");
    }
}

GPUCalcVertexAttributeType StrToGPUCalcVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "VertexCoords"))
        return GPUCalcVertexAttributeType::VertexCoords;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

std::string GPUCalcVertexAttributeTypeToStr(GPUCalcVertexAttributeType vertexAttribute)
{
    switch (vertexAttribute)
    {
        case GPUCalcVertexAttributeType::VertexCoords:
            return "VertexCoords";
        default:
            assert(false);
            throw GameException("Unsupported GPUCalcVertexAttributeType");
    }
}