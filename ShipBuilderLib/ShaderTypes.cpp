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
    if (lstr == "test")
        return ProgramType::Test;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
    case ProgramType::Test:
        return "Test";
    }

    assert(false);
    throw GameException("Unsupported ProgramType");
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
    case ProgramParameterType::OrthoMatrix:
        return "OrthoMatrix";
    }

    assert(false);
    throw GameException("Unsupported ProgramParameterType");
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Test"))
        return VertexAttributeType::Test;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}