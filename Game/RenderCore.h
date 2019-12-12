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
// Texture databases
//

struct CloudTextureDatabaseTraits
{
    static inline std::string DatabaseName = "Cloud";

    using TextureGroups = CloudTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Cloud"))
            return TextureGroups::Cloud;
        else
            throw GameException("Unrecognized Cloud texture group \"" + str + "\"");
    }
};

struct WorldTextureDatabaseTraits
{
    static inline std::string DatabaseName = "World";

    using TextureGroups = WorldTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Land"))
            return TextureGroups::Land;
        else if (Utils::CaseInsensitiveEquals(str, "Ocean"))
            return TextureGroups::Ocean;
        else if (Utils::CaseInsensitiveEquals(str, "WorldBorder"))
            return TextureGroups::WorldBorder;
        else
            throw GameException("Unrecognized World texture group \"" + str + "\"");
    }
};

struct NoiseTextureDatabaseTraits
{
    static inline std::string DatabaseName = "Noise";

    using TextureGroups = NoiseTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Noise"))
            return TextureGroups::Noise;
        else
            throw GameException("Unrecognized Noise texture group \"" + str + "\"");
    }
};

struct GenericTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "GenericTexture";

    using TextureGroups = GenericTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
            return TextureGroups::AirBubble;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombArmor"))
            return TextureGroups::AntiMatterBombArmor;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphereCloud"))
            return TextureGroups::AntiMatterBombSphereCloud;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphere"))
            return TextureGroups::AntiMatterBombSphere;
        else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
            return TextureGroups::ImpactBomb;
        else if (Utils::CaseInsensitiveEquals(str, "PinnedPoint"))
            return TextureGroups::PinnedPoint;
        else if (Utils::CaseInsensitiveEquals(str, "RCBomb"))
            return TextureGroups::RcBomb;
        else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
            return TextureGroups::RcBombPing;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBomb"))
            return TextureGroups::TimerBomb;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombDefuse"))
            return TextureGroups::TimerBombDefuse;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombFuse"))
            return TextureGroups::TimerBombFuse;
        else
            throw GameException("Unrecognized GenericTexture texture group \"" + str + "\"");
    }
};

struct ExplosionTextureDatabaseTraits
{
    static inline std::string DatabaseName = "Explosion";

    using TextureGroups = ExplosionTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Explosion"))
            return TextureGroups::Explosion;
        else
            throw GameException("Unrecognized Explosion texture group \"" + str + "\"");
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
