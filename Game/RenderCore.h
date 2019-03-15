/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <GameCore/Vectors.h>

#include <numeric>
#include <string>

/*
 * Definitions of render-related types and constants that are private
 * to the rendering library but shared among the rendering compilation units.
 */

namespace Render {

//
// Shaders
//

enum class ProgramType
{
    Clouds = 0,
    CrossOfLight,
    LandFlat,
    LandTexture,
    Matte,
    MatteNDC,
    MatteOcean,
    OceanDepth,
    OceanFlat,
    OceanTexture,
    ShipGenericTextures,
    ShipPointsColor,
    ShipRopes,
    ShipSpringsColor,
    ShipSpringsTexture,
    ShipStressedSprings,
    ShipTrianglesColor,
    ShipTrianglesTexture,
    ShipVectors,
    Stars,
    TextNDC,

    _Last = TextNDC
};

ProgramType ShaderFilenameToProgramType(std::string const & str);

std::string ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType : uint8_t
{
    AmbientLightIntensity = 0,
    LandFlatColor,
    MatteColor,
    OceanTransparency,
    OceanDepthColorStart,
    OceanDepthColorEnd,
    OceanFlatColor,
    OrthoMatrix,
    StarTransparency,
    TextureScaling,
    ViewportSize,
    WaterContrast,
    WaterLevelThreshold,

    // Textures
    SharedTexture,                  // 0, for programs that don't use a dedicated unit and hence will keep binding different textures
    CloudTexture,                   // 1
    GenericTexturesAtlasTexture,    // 2
    LandTexture,                    // 3
    OceanTexture,                   // 4

    _FirstTexture = SharedTexture,
    _LastTexture = OceanTexture
};

ProgramParameterType StrToProgramParameterType(std::string const & str);

std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

/*
 * This enum serves merely to associate a vertex attribute index to each vertex attribute name.
 */
enum class VertexAttributeType : GLuint
{
    //
    // World
    //

    Star = 0,

    Cloud = 0,

    Land = 0,

    Ocean = 0,

    CrossOfLight1 = 0,
    CrossOfLight2 = 1,

    //
    // Ship
    //

    ShipPointPosition = 0,
    ShipPointColor = 1,
    ShipPointLight = 2,
    ShipPointWater = 3,
    ShipPointPlaneId = 4,
    ShipPointTextureCoordinates = 5,

    GenericTexture1 = 0,
    GenericTexture2 = 1,
    GenericTexture3 = 2,

    VectorArrow = 0,

    //
    // Text
    //

    Text1 = 0,
    Text2 = 1
};

VertexAttributeType StrToVertexAttributeType(std::string const & str);

struct ShaderManagerTraits
{
    using ProgramType = Render::ProgramType;
    using ProgramParameterType = Render::ProgramParameterType;
    using VertexAttributeType = Render::VertexAttributeType;

    static constexpr auto ShaderFilenameToProgramType = Render::ShaderFilenameToProgramType;
    static constexpr auto ProgramTypeToStr = Render::ProgramTypeToStr;
    static constexpr auto StrToProgramParameterType = Render::StrToProgramParameterType;
    static constexpr auto ProgramParameterTypeToStr = Render::ProgramParameterTypeToStr;
    static constexpr auto StrToVertexAttributeType = Render::StrToVertexAttributeType;
};


//
// Text
//

/*
 * Describes a vertex of a text quad, with all the information necessary to the shader.
 */
#pragma pack(push)
struct TextQuadVertex
{
    float positionNdcX;
    float positionNdcY;
    float textureCoordinateX;
    float textureCoordinateY;
    float transparency;

    TextQuadVertex(
        float _positionNdcX,
        float _positionNdcY,
        float _textureCoordinateX,
        float _textureCoordinateY,
        float _transparency)
        : positionNdcX(_positionNdcX)
        , positionNdcY(_positionNdcY)
        , textureCoordinateX(_textureCoordinateX)
        , textureCoordinateY(_textureCoordinateY)
        , transparency(_transparency)
    {}
};
#pragma pack(pop)


//
// Statistics
//

struct RenderStatistics
{
    std::uint64_t LastRenderedShipRopes;
    std::uint64_t LastRenderedShipSprings;
    std::uint64_t LastRenderedShipTriangles;
    std::uint64_t LastRenderedShipPlanes;
    std::uint64_t LastRenderedShipGenericTextures;
    std::uint64_t LastRenderedShipEphemeralPoints;

    RenderStatistics()
    {
        Reset();
    }

    void Reset()
    {
        LastRenderedShipRopes = 0;
        LastRenderedShipSprings = 0;
        LastRenderedShipTriangles = 0;
        LastRenderedShipPlanes = 0;
        LastRenderedShipGenericTextures = 0;
        LastRenderedShipEphemeralPoints = 0;
    }
};

}
