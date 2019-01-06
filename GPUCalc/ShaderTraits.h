/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <numeric>
#include <string>

enum class GPUCalcProgramType
{
    Test = 0,

    _Last = Test
};

GPUCalcProgramType ShaderFilenameToGPUCalcProgramType(std::string const & str);

std::string GPUCalcProgramTypeToStr(GPUCalcProgramType program);

enum class GPUCalcProgramParameterType : uint8_t
{
    TODO = 0,
};

GPUCalcProgramParameterType StrToGPUCalcProgramParameterType(std::string const & str);

std::string GPUCalcProgramParameterTypeToStr(GPUCalcProgramParameterType programParameter);

enum class GPUCalcVertexAttributeType : GLuint
{
    VertexCoords = 0,
};

GPUCalcVertexAttributeType StrToGPUCalcVertexAttributeType(std::string const & str);

std::string GPUCalcVertexAttributeTypeToStr(GPUCalcVertexAttributeType vertexAttribute);

struct GPUCalcShaderManagerTraits
{
    using ProgramType = GPUCalcProgramType;
    using ProgramParameterType = GPUCalcProgramParameterType;
    using VertexAttributeType = GPUCalcVertexAttributeType;

    static constexpr auto ShaderFilenameToProgramType = ShaderFilenameToGPUCalcProgramType;
    static constexpr auto ProgramTypeToStr = GPUCalcProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = StrToGPUCalcProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = GPUCalcProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = StrToGPUCalcVertexAttributeType;
    static constexpr auto VertexAttributeTypeToStr = GPUCalcVertexAttributeTypeToStr;
};
