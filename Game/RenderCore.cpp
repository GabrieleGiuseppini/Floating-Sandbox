/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderCore.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

namespace Render {

ProgramType ShaderFilenameToProgramType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "clouds")
        return ProgramType::Clouds;
    else if (lstr == "cross_of_light")
        return ProgramType::CrossOfLight;
    else if (lstr == "land_flat")
        return ProgramType::LandFlat;
    else if (lstr == "land_texture")
        return ProgramType::LandTexture;
    else if (lstr == "matte")
        return ProgramType::Matte;
    else if (lstr == "matte_ndc")
        return ProgramType::MatteNDC;
    else if (lstr == "matte_ocean")
        return ProgramType::MatteOcean;
    else if (lstr == "ocean_depth")
        return ProgramType::OceanDepth;
    else if (lstr == "ocean_flat")
        return ProgramType::OceanFlat;
    else if (lstr == "ocean_texture")
        return ProgramType::OceanTexture;
    else if (lstr == "ship_generic_textures")
        return ProgramType::ShipGenericTextures;
    else if (lstr == "ship_points_color")
        return ProgramType::ShipPointsColor;
    else if (lstr == "ship_ropes")
        return ProgramType::ShipRopes;
    else if (lstr == "ship_springs_color")
        return ProgramType::ShipSpringsColor;
    else if (lstr == "ship_springs_texture")
        return ProgramType::ShipSpringsTexture;
    else if (lstr == "ship_stressed_springs")
        return ProgramType::ShipStressedSprings;
    else if (lstr == "ship_triangles_color")
        return ProgramType::ShipTrianglesColor;
    else if (lstr == "ship_triangles_texture")
        return ProgramType::ShipTrianglesTexture;
    else if (lstr == "ship_vectors")
        return ProgramType::ShipVectors;
    else if (lstr == "stars")
        return ProgramType::Stars;
    else if (lstr == "text_ndc")
        return ProgramType::TextNDC;
    else
        throw GameException("Unrecognized program \"" + str + "\"");
}

std::string ProgramTypeToStr(ProgramType program)
{
    switch (program)
    {
    case ProgramType::Clouds:
        return "Clouds";
    case ProgramType::CrossOfLight:
        return "CrossOfLight";
    case ProgramType::LandFlat:
        return "LandFlat";
    case ProgramType::LandTexture:
        return "LandTexture";
    case ProgramType::Matte:
        return "Matte";
    case ProgramType::MatteNDC:
        return "MatteNDC";
    case ProgramType::MatteOcean:
        return "MatteOcean";
    case ProgramType::OceanDepth:
        return "OceanDepth";
    case ProgramType::OceanFlat:
        return "OceanFlat";
    case ProgramType::OceanTexture:
        return "OceanTexture";
    case ProgramType::ShipGenericTextures:
        return "ShipGenericTextures";
    case ProgramType::ShipPointsColor:
        return "ShipPointsColor";
    case ProgramType::ShipRopes:
        return "ShipRopes";
    case ProgramType::ShipSpringsColor:
        return "ShipSpringsColor";
    case ProgramType::ShipSpringsTexture:
        return "ShipSpringsTexture";
    case ProgramType::ShipStressedSprings:
        return "ShipStressedSprings";
    case ProgramType::ShipTrianglesColor:
        return "ShipTrianglesColor";
    case ProgramType::ShipTrianglesTexture:
        return "ShipTrianglesTexture";
    case ProgramType::ShipVectors:
        return "ShipVectors";
    case ProgramType::Stars:
        return "Stars";
    case ProgramType::TextNDC:
        return "TextNDC";
    default:
        assert(false);
        throw GameException("Unsupported ProgramType");
    }
}

ProgramParameterType StrToProgramParameterType(std::string const & str)
{
    if (str == "AmbientLightIntensity")
        return ProgramParameterType::AmbientLightIntensity;
    else if (str == "LandFlatColor")
        return ProgramParameterType::LandFlatColor;
    else if (str == "MatteColor")
        return ProgramParameterType::MatteColor;
    else if (str == "OceanTransparency")
        return ProgramParameterType::OceanTransparency;
    else if (str == "OceanDepthColorStart")
        return ProgramParameterType::OceanDepthColorStart;
    else if (str == "OceanDepthColorEnd")
        return ProgramParameterType::OceanDepthColorEnd;
    else if (str == "OceanFlatColor")
        return ProgramParameterType::OceanFlatColor;
    else if (str == "OrthoMatrix")
        return ProgramParameterType::OrthoMatrix;
    else if (str == "StarTransparency")
        return ProgramParameterType::StarTransparency;
    else if (str == "TextureScaling")
        return ProgramParameterType::TextureScaling;
    else if (str == "ViewportSize")
        return ProgramParameterType::ViewportSize;
    else if (str == "WaterContrast")
        return ProgramParameterType::WaterContrast;
    else if (str == "WaterLevelThreshold")
        return ProgramParameterType::WaterLevelThreshold;
    // Textures
    else if (str == "SharedTexture")
        return ProgramParameterType::SharedTexture;
    else if (str == "CloudTexture")
        return ProgramParameterType::CloudTexture;
    else if (str == "GenericTexturesAtlasTexture")
        return ProgramParameterType::GenericTexturesAtlasTexture;
    else if (str == "LandTexture")
        return ProgramParameterType::LandTexture;
    else if (str == "OceanTexture")
        return ProgramParameterType::OceanTexture;
    else
        throw GameException("Unrecognized program parameter \"" + str + "\"");
}

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter)
{
    switch (programParameter)
    {
    case ProgramParameterType::AmbientLightIntensity:
        return "AmbientLightIntensity";
    case ProgramParameterType::LandFlatColor:
        return "LandFlatColor";
    case ProgramParameterType::MatteColor:
        return "MatteColor";
    case ProgramParameterType::OceanTransparency:
        return "OceanTransparency";
    case ProgramParameterType::OceanDepthColorStart:
        return "OceanDepthColorStart";
    case ProgramParameterType::OceanDepthColorEnd:
        return "OceanDepthColorEnd";
    case ProgramParameterType::OceanFlatColor:
        return "OceanFlatColor";
    case ProgramParameterType::OrthoMatrix:
        return "OrthoMatrix";
    case ProgramParameterType::StarTransparency:
        return "StarTransparency";
    case ProgramParameterType::TextureScaling:
        return "TextureScaling";
    case ProgramParameterType::ViewportSize:
        return "ViewportSize";
    case ProgramParameterType::WaterContrast:
        return "WaterContrast";
    case ProgramParameterType::WaterLevelThreshold:
        return "WaterLevelThreshold";
    // Textures
    case ProgramParameterType::SharedTexture:
        return "SharedTexture";
    case ProgramParameterType::CloudTexture:
        return "CloudTexture";
    case ProgramParameterType::GenericTexturesAtlasTexture:
        return "GenericTexturesAtlasTexture";
    case ProgramParameterType::LandTexture:
        return "LandTexture";
    case ProgramParameterType::OceanTexture:
        return "OceanTexture";
    default:
        assert(false);
        throw GameException("Unsupported ProgramParameterType");
    }
}

VertexAttributeType StrToVertexAttributeType(std::string const & str)
{
    // World
    if (Utils::CaseInsensitiveEquals(str, "Star"))
        return VertexAttributeType::Star;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud"))
        return VertexAttributeType::Cloud;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return VertexAttributeType::Land;
    else if (Utils::CaseInsensitiveEquals(str, "Ocean"))
        return VertexAttributeType::Ocean;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight1"))
        return VertexAttributeType::CrossOfLight1;
    else if (Utils::CaseInsensitiveEquals(str, "CrossOfLight2"))
        return VertexAttributeType::CrossOfLight2;
    // Ship
    else if (Utils::CaseInsensitiveEquals(str, "GenericTexture1"))
        return VertexAttributeType::GenericTexture1;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTexture2"))
        return VertexAttributeType::GenericTexture2;
    else if (Utils::CaseInsensitiveEquals(str, "GenericTexture3"))
        return VertexAttributeType::GenericTexture3;
    else if (Utils::CaseInsensitiveEquals(str, "VectorArrow"))
        return VertexAttributeType::VectorArrow;
    // TODOHERE: continue with ship
    // Text
    else if (Utils::CaseInsensitiveEquals(str, "Text1"))
        return VertexAttributeType::Text1;
    else if (Utils::CaseInsensitiveEquals(str, "Text2"))
        return VertexAttributeType::Text2;
    // TODO:OLD
    if (Utils::CaseInsensitiveEquals(str, "SharedAttribute0"))
        return VertexAttributeType::SharedAttribute0;
    else if (Utils::CaseInsensitiveEquals(str, "SharedAttribute1"))
        return VertexAttributeType::SharedAttribute1;
    else if (Utils::CaseInsensitiveEquals(str, "SharedAttribute2"))
        return VertexAttributeType::SharedAttribute2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointPosition"))
        return VertexAttributeType::ShipPointPosition;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointColor"))
        return VertexAttributeType::ShipPointColor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointLight"))
        return VertexAttributeType::ShipPointLight;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointWater"))
        return VertexAttributeType::ShipPointWater;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointPlaneId"))
        return VertexAttributeType::ShipPointPlaneId;
    else if (Utils::CaseInsensitiveEquals(str, "ShipPointTextureCoordinates"))
        return VertexAttributeType::ShipPointTextureCoordinates;
    else
        throw GameException("Unrecognized vertex attribute \"" + str + "\"");
}

}