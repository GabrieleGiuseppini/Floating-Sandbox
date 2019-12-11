/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameOpenGL/GameOpenGL.h>

#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Utils.h>
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
    FireExtinguisherSpray,
    HeatBlasterFlameCool,
    HeatBlasterFlameHeat,
    LandFlat,
    LandTexture,
	Lightning,
    OceanDepth,
    OceanFlat,
    OceanTexture,
	Rain,
    ShipExplosions,
    ShipFlamesBackground1,
    ShipFlamesBackground2,
    ShipFlamesForeground1,
    ShipFlamesForeground2,
    ShipGenericTextures,
    ShipPointsColor,
    ShipPointsColorWithTemperature,
    ShipRopes,
    ShipRopesWithTemperature,
    ShipSparkles,
    ShipSpringsColor,
    ShipSpringsColorWithTemperature,
    ShipSpringsTexture,
    ShipSpringsTextureWithTemperature,
    ShipStressedSprings,
    ShipTrianglesColor,
    ShipTrianglesColorWithTemperature,
    ShipTrianglesDecay,
    ShipTrianglesTexture,
    ShipTrianglesTextureWithTemperature,
    ShipVectors,
    Stars,
    TextNDC,
    WorldBorder,

    _Last = WorldBorder
};

ProgramType ShaderFilenameToProgramType(std::string const & str);

std::string ProgramTypeToStr(ProgramType program);

enum class ProgramParameterType : uint8_t
{
    EffectiveAmbientLightIntensity = 0,
    FlameSpeed,
    FlameWindRotationAngle,
    HeatOverlayTransparency,
    LandFlatColor,
    MatteColor,
    OceanTransparency,
    OceanDarkeningRate,
    OceanDepthColorStart,
    OceanDepthColorEnd,
    OceanFlatColor,
    OrthoMatrix,
	RainDensity,
    StarTransparency,
    TextureScaling,
    Time,
    ViewportSize,
    WaterColor,
    WaterContrast,
    WaterLevelThreshold,

    // Textures
    SharedTexture,                  // 0, for programs that don't use a dedicated unit and hence will keep binding different textures
    CloudsAtlasTexture,             // 1
    ExplosionsAtlasTexture,         // 2
    GenericTexturesAtlasTexture,    // 3
    LandTexture,                    // 4
    NoiseTexture1,                  // 5
    NoiseTexture2,                  // 6
    OceanTexture,                   // 7
    WorldBorderTexture,             // 8

    _FirstTexture = SharedTexture,
    _LastTexture = WorldBorderTexture
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

	Lightning1 = 0,
	Lightning2 = 1,

    Cloud1 = 0,
    Cloud2 = 1,

    Land = 0,

    Ocean = 0,

    CrossOfLight1 = 0,
    CrossOfLight2 = 1,

	Rain = 0,

    FireExtinguisherSpray = 0,

    HeatBlasterFlame = 0,

    WorldBorder = 0,

    //
    // Ship
    //

    ShipPointAttributeGroup1 = 0,   // Position, TextureCoordinates
    ShipPointAttributeGroup2 = 1,   // Light, Water, PlaneId, Decay
    ShipPointColor = 2,
    ShipPointTemperature = 3,

    Explosion1 = 0,
    Explosion2 = 1,
    Explosion3 = 2,

    Sparkle1 = 0,
    Sparkle2 = 1,

    GenericTexture1 = 0,
    GenericTexture2 = 1,
    GenericTexture3 = 2,

    Flame1 = 0,
    Flame2 = 1,

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
// Textures
//

struct CloudTextureDatabaseTraits
{
    using TextureGroupType = CloudTextureGroups;

    static TextureGroupType StrToTextureGroupType(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Cloud"))
            return TextureGroupType::Cloud;
        else
            throw GameException("Unrecognized Cloud TextureGroupType \"" + str + "\"");
    }
};

struct WorldTextureDatabaseTraits
{
    using TextureGroupType = WorldTextureGroups;

    static TextureGroupType StrToTextureGroupType(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Land"))
            return TextureGroupType::Land;
        else if (Utils::CaseInsensitiveEquals(str, "Ocean"))
            return TextureGroupType::Ocean;
        else if (Utils::CaseInsensitiveEquals(str, "Border"))
            return TextureGroupType::Border;
        else
            throw GameException("Unrecognized World TextureGroupType \"" + str + "\"");
    }
};

struct NoiseTextureDatabaseTraits
{
    using TextureGroupType = NoiseTextureGroups;

    static TextureGroupType StrToTextureGroupType(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Noise"))
            return TextureGroupType::Noise;
        else
            throw GameException("Unrecognized Noise TextureGroupType \"" + str + "\"");
    }
};

struct GenericTextureTextureDatabaseTraits
{
    using TextureGroupType = GenericTextureGroups;

    static TextureGroupType StrToTextureGroupType(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
            return TextureGroupType::AirBubble;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombArmor"))
            return TextureGroupType::AntiMatterBombArmor;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphereCloud"))
            return TextureGroupType::AntiMatterBombSphereCloud;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphere"))
            return TextureGroupType::AntiMatterBombSphere;
        else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
            return TextureGroupType::ImpactBomb;
        else if (Utils::CaseInsensitiveEquals(str, "PinnedPoint"))
            return TextureGroupType::PinnedPoint;
        else if (Utils::CaseInsensitiveEquals(str, "RCBomb"))
            return TextureGroupType::RcBomb;
        else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
            return TextureGroupType::RcBombPing;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBomb"))
            return TextureGroupType::TimerBomb;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombDefuse"))
            return TextureGroupType::TimerBombDefuse;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombFuse"))
            return TextureGroupType::TimerBombFuse;
        else
            throw GameException("Unrecognized GenericTexture TextureGroupType \"" + str + "\"");
    }
};

struct ExplosionTextureDatabaseTraits
{
    using TextureGroupType = ExplosionTextureGroups;

    static TextureGroupType StrToTextureGroupType(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Explosion"))
            return TextureGroupType::Explosion;
        else
            throw GameException("Unrecognized Explosion TextureGroupType \"" + str + "\"");
    }
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
    float alpha;

    TextQuadVertex(
        float _positionNdcX,
        float _positionNdcY,
        float _textureCoordinateX,
        float _textureCoordinateY,
        float _alpha)
        : positionNdcX(_positionNdcX)
        , positionNdcY(_positionNdcY)
        , textureCoordinateX(_textureCoordinateX)
        , textureCoordinateY(_textureCoordinateY)
        , alpha(_alpha)
    {}
};
#pragma pack(pop)


//
// Statistics
//

struct RenderStatistics
{
    std::uint64_t LastRenderedShipPoints;
    std::uint64_t LastRenderedShipRopes;
    std::uint64_t LastRenderedShipSprings;
    std::uint64_t LastRenderedShipTriangles;
    std::uint64_t LastRenderedShipPlanes;
    std::uint64_t LastRenderedShipFlames;
    std::uint64_t LastRenderedShipGenericTextures;

    RenderStatistics()
    {
        Reset();
    }

    void Reset()
    {
        LastRenderedShipPoints = 0;
        LastRenderedShipRopes = 0;
        LastRenderedShipSprings = 0;
        LastRenderedShipTriangles = 0;
        LastRenderedShipPlanes = 0;
        LastRenderedShipFlames = 0;
        LastRenderedShipGenericTextures = 0;
    }
};

}
