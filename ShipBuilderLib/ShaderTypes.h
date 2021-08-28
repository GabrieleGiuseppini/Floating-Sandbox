/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <cstdint>
#include <string>

namespace ShipBuilder {

//
// Shaders
//

enum class ProgramType
{
    Test = 0,

    _Last = Test
};

ProgramType ShaderFilenameToProgramType(std::string const & str);

std::string ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType : uint8_t
{
    OrthoMatrix = 0,

    _FirstTexture = OrthoMatrix,
    _LastTexture = OrthoMatrix
};

ProgramParameterType StrToProgramParameterType(std::string const & str);

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

/*
 * This enum serves merely to associate a vertex attribute index to each vertex attribute name.
 */
enum class VertexAttributeType : GLuint
{
    Test = 0,
};

VertexAttributeType StrToVertexAttributeType(std::string const & str);

struct ShaderManagerTraits
{
    using ProgramType = ShipBuilder::ProgramType;
    using ProgramParameterType = ShipBuilder::ProgramParameterType;
    using VertexAttributeType = ShipBuilder::VertexAttributeType;

    static constexpr auto ShaderFilenameToProgramType = ShipBuilder::ShaderFilenameToProgramType;
    static constexpr auto ProgramTypeToStr = ShipBuilder::ProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = ShipBuilder::StrToProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = ShipBuilder::ProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = ShipBuilder::StrToVertexAttributeType;
};

}
