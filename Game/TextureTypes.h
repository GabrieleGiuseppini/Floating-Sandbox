/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cstdint>
#include <string>

namespace Render {

//
// Texture databases
//

enum class CloudTextureGroups : uint16_t
{
    Cloud = 0,

    _Last = Cloud
};

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

enum class WorldTextureGroups : uint16_t
{
    Land = 0,
    Ocean,
    WorldBorder,

    _Last = WorldBorder
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

enum class NoiseTextureGroups : uint16_t
{
    Noise = 0,

    _Last = Noise
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

enum class GenericTextureGroups : uint16_t
{
    AirBubble = 0,
    AntiMatterBombArmor,
    AntiMatterBombSphere,
    AntiMatterBombSphereCloud,
    Fire,
    ImpactBomb,
    PinnedPoint,
    RcBomb,
    RcBombPing,
    SmokeDark,
    SmokeLight,
    TimerBomb,
    TimerBombFuse,

    _Last = TimerBombFuse
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
        else if (Utils::CaseInsensitiveEquals(str, "Fire"))
            return TextureGroups::Fire;
        else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
            return TextureGroups::ImpactBomb;
        else if (Utils::CaseInsensitiveEquals(str, "PinnedPoint"))
            return TextureGroups::PinnedPoint;
        else if (Utils::CaseInsensitiveEquals(str, "RCBomb"))
            return TextureGroups::RcBomb;
        else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
            return TextureGroups::RcBombPing;
        else if (Utils::CaseInsensitiveEquals(str, "SmokeDark"))
            return TextureGroups::SmokeDark;
        else if (Utils::CaseInsensitiveEquals(str, "SmokeLight"))
            return TextureGroups::SmokeLight;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBomb"))
            return TextureGroups::TimerBomb;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombFuse"))
            return TextureGroups::TimerBombFuse;
        else
            throw GameException("Unrecognized GenericTexture texture group \"" + str + "\"");
    }
};

enum class ExplosionTextureGroups : uint16_t
{
    Explosion = 0,

    _Last = Explosion
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

}
