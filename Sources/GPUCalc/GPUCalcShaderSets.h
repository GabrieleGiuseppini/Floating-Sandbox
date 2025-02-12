/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-06
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <numeric>
#include <string>

namespace GPUCalcShaderSets {

enum class ProgramKind
{
    PixelCoords = 0,
    Add = 1,

    _Last = Add
};

namespace _detail {

ProgramKind ShaderNameToProgramKind(std::string const & str);

std::string ProgramKindToStr(ProgramKind program);

}

enum class ProgramParameterKind : uint8_t
{
    // Textures
    TextureInput0,                  // 0
    TextureInput1,                  // 1

    _FirstTexture = TextureInput0,
    _LastTexture = TextureInput1
};

namespace _detail {

ProgramParameterKind StrToProgramParameterKind(std::string const & str);

std::string ProgramParameterKindToStr(ProgramParameterKind programParameter);

}

enum class VertexAttributeKind : std::uint32_t
{
    VertexShaderInput0 = 0,
};

namespace _detail {

VertexAttributeKind StrToVertexAttributeKind(std::string const & str);

std::string VertexAttributeKindToStr(VertexAttributeKind vertexAttribute);

}

struct ShaderSet
{
    using ProgramKindType = ProgramKind;
    using ProgramParameterKindType = ProgramParameterKind;
    using VertexAttributeKindType = VertexAttributeKind;

    static inline std::string ShaderSetName = "GPUCalc";

    static constexpr auto ShaderNameToProgramKind = _detail::ShaderNameToProgramKind;
    static constexpr auto ProgramKindToStr = _detail::ProgramKindToStr;
    static constexpr auto StrToProgramParameterKind = _detail::StrToProgramParameterKind;
    static constexpr auto ProgramParameterKindToStr = _detail::ProgramParameterKindToStr;
    static constexpr auto StrToVertexAttributeKind = _detail::StrToVertexAttributeKind;
};

}