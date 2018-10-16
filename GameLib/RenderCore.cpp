/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderCore.h"

#include "GameException.h"
#include "Utils.h"

#include <cassert>

namespace Render {

ProgramType ShaderFilenameToProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "clouds")
        return ProgramType::Clouds;
    else if (lstr == "generic_textures")
        return ProgramType::GenericTextures;
    else if (lstr == "land")
        return ProgramType::Land;
    else if (lstr == "matte")
        return ProgramType::Matte;
    else if (lstr == "matte_ndc")
        return ProgramType::MatteNDC;
    else if (lstr == "matte_water")
        return ProgramType::MatteWater;
    else if (lstr == "ship_ropes")
        return ProgramType::ShipRopes;
    else if (lstr == "ship_stressed_springs")
        return ProgramType::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return ProgramType::ShipTrianglesColor;
    else if (lstr == "ship_triangles_texture")
        return ProgramType::ShipTrianglesTexture;
    else if (lstr == "text_ndc")
        return ProgramType::TextNDC;
    else if (lstr == "water")
        return ProgramType::Water;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
    case ProgramType::Clouds:
        return "Clouds";
    case ProgramType::GenericTextures:
        return "GenericTextures";
    case ProgramType::Land:
        return "Land";
    case ProgramType::Matte:
        return "Matte";
    case ProgramType::MatteNDC:
        return "MatteNDC";
    case ProgramType::MatteWater:
        return "MatteWater";
    case ProgramType::ShipRopes:
        return "ShipRopes";
    case ProgramType::ShipStressedSprings:
        return "ShipStressedSprings";
    case ProgramType::ShipTrianglesColor:
        return "ShipTrianglesColor";
    case ProgramType::ShipTrianglesTexture:
        return "ShipTrianglesTexture";
    case ProgramType::TextNDC:
        return "TextNDC";
    case ProgramType::Water:
        return "Water";
    default:
        assert(false);
        throw GameException("Unsupported ProgramType");
    }
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "AmbientLightIntensity")
        return ProgramParameterType::AmbientLightIntensity;
    else if (str == "MatteColor")
        return ProgramParameterType::MatteColor;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "TextureScaling")
        return ProgramParameterType::TextureScaling;
    else if (str == "WaterLevelThreshold")
        return ProgramParameterType::WaterLevelThreshold;
    else if (str == "WaterTransparency")
        return ProgramParameterType::WaterTransparency;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
    case ProgramParameterType::AmbientLightIntensity:
        return "AmbientLightIntensity";
    case ProgramParameterType::MatteColor:
        return "MatteColor";
    case ProgramParameterType::OrthoMatrix:
        return "OrthoMatrix";
    case ProgramParameterType::TextureScaling:
        return "TextureScaling";
    case ProgramParameterType::WaterLevelThreshold:
        return "WaterLevelThreshold";
    case ProgramParameterType::WaterTransparency:
        return "WaterTransparency";
    default:
        assert(false);
        throw GameException("Unsupported ProgramParameterType");
    }
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "SharedAttribute1"))
        return VertexAttributeType::SharedAttribute1;
    else if (Utils::CaseInsensitiveEquals(str, "SharedAttribute2"))
        return VertexAttributeType::SharedAttribute2;
    else if (Utils::CaseInsensitiveEquals(str, "SharedAttribute3"))
        return VertexAttributeType::SharedAttribute3;
    else if (Utils::CaseInsensitiveEquals(str, "WaterPosition"))
        return VertexAttributeType::WaterPosition;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTexturePackedData1"))
        return VertexAttributeType::GenericTexturePackedData1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTexturePackedData2"))
        return VertexAttributeType::GenericTexturePackedData2;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureTextureCoordinates"))
        return VertexAttributeType::GenericTextureTextureCoordinates;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointPosition"))
        return VertexAttributeType::ShipPointPosition;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointColor"))
        return VertexAttributeType::ShipPointColor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointLight"))
        return VertexAttributeType::ShipPointLight;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointWater"))
        return VertexAttributeType::ShipPointWater;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointTextureCoordinates"))
        return VertexAttributeType::ShipPointTextureCoordinates;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

std::string VertexAttributeTypeToStr(VertexAttributeType vertexAttribute)
{
    switch (vertexAttribute)
    {
    case VertexAttributeType::SharedAttribute1:
        return "SharedAttribute1";
    case VertexAttributeType::SharedAttribute2:
        return "SharedAttribute2";
    case VertexAttributeType::SharedAttribute3:
        return "SharedAttribute3";
    case VertexAttributeType::WaterPosition:
        return "WaterPosition";
    case VertexAttributeType::GenericTexturePackedData1:
        return "GenericTexturePackedData1";        
    case VertexAttributeType::GenericTexturePackedData2:
        return "GenericTexturePackedData2";
    case VertexAttributeType::GenericTextureTextureCoordinates:
        return "GenericTextureTextureCoordinates";
    case VertexAttributeType::ShipPointPosition:
        return "ShipPointPosition";
    case VertexAttributeType::ShipPointColor:
        return "ShipPointColor";
    case VertexAttributeType::ShipPointLight:
        return "ShipPointLight";
    case VertexAttributeType::ShipPointWater:
        return "ShipPointWater";
    case VertexAttributeType::ShipPointTextureCoordinates:
        return "ShipPointTextureCoordinates";
    default:
        assert(false);
        throw GameException("Unsupported VertexAttributeType");
    }
}

}
