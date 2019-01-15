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
    PixelCoords = 0,
    Add = 1,

    _Last = Add
};

GPUCalcProgramType ShaderFilenameToGPUCalcProgramType(std::string const & str);

std::string GPUCalcProgramTypeToStr(GPUCalcProgramType program);

enum class GPUCalcProgramParameterType : uint8_t
{
    // Textures
    TextureInput0,                  // 0
    TextureInput1,                  // 1

    _FirstTexture = TextureInput0,
    _LastTexture = TextureInput1
};

GPUCalcProgramParameterType StrToGPUCalcProgramParameterType(std::string const & str);

std::string GPUCalcProgramParameterTypeToStr(GPUCalcProgramParameterType programParameter);

enum class GPUCalcVertexAttributeType : GLuint
{
    VertexShaderInput0 = 0,
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
