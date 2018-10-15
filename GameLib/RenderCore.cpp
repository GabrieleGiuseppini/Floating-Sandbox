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
    if (Utils::CaseInsensitiveEquals(str, "SharedPosition"))
        return VertexAttributeType::SharedPosition;
    else if (Utils::CaseInsensitiveEquals(str, "SharedTextureCoordinates"))
        return VertexAttributeType::SharedTextureCoordinates;
    else if (Utils::CaseInsensitiveEquals(str, "Shared1XFloat"))
        return VertexAttributeType::Shared1XFloat;
    else if (Utils::CaseInsensitiveEquals(str, "WaterPosition"))
        return VertexAttributeType::WaterPosition;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureCenterPosition"))
        return VertexAttributeType::GenericTextureCenterPosition;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureVertexOffset"))
        return VertexAttributeType::GenericTextureVertexOffset;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureTextureCoordinates"))
        return VertexAttributeType::GenericTextureTextureCoordinates;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureRotationAngle"))
        return VertexAttributeType::GenericTextureRotationAngle;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureScale"))
        return VertexAttributeType::GenericTextureScale;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureTransparency"))
        return VertexAttributeType::GenericTextureTransparency;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTextureAmbientLightSensitivity"))
        return VertexAttributeType::GenericTextureAmbientLightSensitivity;
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
    case VertexAttributeType::SharedPosition:
        return "SharedPosition";
    case VertexAttributeType::SharedTextureCoordinates:
        return "SharedTextureCoordinates";
    case VertexAttributeType::Shared1XFloat:
        return "Shared1XFloat";
    case VertexAttributeType::WaterPosition:
        return "WaterPosition";
    case VertexAttributeType::GenericTextureCenterPosition:
        return "GenericTextureCenterPosition";
    case VertexAttributeType::GenericTextureVertexOffset:
        return "GenericTextureVertexOffset";
    case VertexAttributeType::GenericTextureTextureCoordinates:
        return "GenericTextureTextureCoordinates";
    case VertexAttributeType::GenericTextureRotationAngle:
        return "GenericTextureRotationAngle";
    case VertexAttributeType::GenericTextureScale:
        return "GenericTextureScale";
    case VertexAttributeType::GenericTextureTransparency:
        return "GenericTextureTransparency";
    case VertexAttributeType::GenericTextureAmbientLightSensitivity:
        return "GenericTextureAmbientLightSensitivity";
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
