/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>
#include <GameCore/Vectors.h>

#include <cstdint>
#include <string>

namespace Render {

//
// Texture databases
//

// Cloud

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

// World

enum class WorldTextureGroups : uint16_t
{
    Land = 0,
    Ocean,

    _Last = Ocean
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
        else
            throw GameException("Unrecognized World texture group \"" + str + "\"");
    }
};

// Noise

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

enum class NoiseType : uint32_t
{
    Gross = 0,
    Fine = 1,
    Perlin_4_32_043 = 2,
    Perlin_8_1024_073 = 3
};

// Generic Mip-Mapped

enum class GenericMipMappedTextureGroups : uint16_t
{
    AirBubble = 0,
    AntiMatterBombArmor,
    AntiMatterBombSphere,
    AntiMatterBombSphereCloud,
    EngineWake,
    FireExtinguishingBomb,
    ImpactBomb,
    LaserCannon,
    PhysicsProbe,
    PhysicsProbePing,
    PinnedPoint,
    RcBomb,
    RcBombPing,
    SmokeDark,
    SmokeLight,
    TimerBomb,
    TimerBombFuse,

    _Last = TimerBombFuse
};

struct GenericMipMappedTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "GenericMipMappedTexture";

    using TextureGroups = GenericMipMappedTextureGroups;

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
        else if (Utils::CaseInsensitiveEquals(str, "EngineWake"))
            return TextureGroups::EngineWake;
        else if (Utils::CaseInsensitiveEquals(str, "FireExtinguishingBomb"))
            return TextureGroups::FireExtinguishingBomb;
        else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
            return TextureGroups::ImpactBomb;
        else if (Utils::CaseInsensitiveEquals(str, "LaserCannon"))
            return TextureGroups::LaserCannon;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbe"))
            return TextureGroups::PhysicsProbe;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePing"))
            return TextureGroups::PhysicsProbePing;
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
            throw GameException("Unrecognized GenericMipMappedTexture texture group \"" + str + "\"");
    }
};

// Generic Linear

enum class GenericLinearTextureGroups : uint16_t
{
    AutoFocusNotification,
    DayLightCycleNotification,
    Fire,
    PhysicsProbePanel,
    ShiftNotification,
    SoundMuteNotification,
    UVModeNotification,

    _Last = UVModeNotification
};

struct GenericLinearTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "GenericLinearTexture";

    using TextureGroups = GenericLinearTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "AutoFocusNotification"))
            return TextureGroups::AutoFocusNotification;
        else if (Utils::CaseInsensitiveEquals(str, "DayLightCycleNotification"))
            return TextureGroups::DayLightCycleNotification;
        else if (Utils::CaseInsensitiveEquals(str, "Fire"))
            return TextureGroups::Fire;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel"))
            return TextureGroups::PhysicsProbePanel;
        else if (Utils::CaseInsensitiveEquals(str, "ShiftNotification"))
            return TextureGroups::ShiftNotification;
        else if (Utils::CaseInsensitiveEquals(str, "SoundMuteNotification"))
            return TextureGroups::SoundMuteNotification;
        else if (Utils::CaseInsensitiveEquals(str, "UVModeNotification"))
            return TextureGroups::UVModeNotification;
        else
            throw GameException("Unrecognized GenericLinearTexture texture group \"" + str + "\"");
    }
};

// Explosion

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

// Fish

enum class FishTextureGroups : uint16_t
{
    Fish = 0,

    _Last = Fish
};

struct FishTextureDatabaseTraits
{
    static inline std::string DatabaseName = "Fish";

    using TextureGroups = FishTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Fish"))
            return TextureGroups::Fish;
        else
            throw GameException("Unrecognized Fish texture group \"" + str + "\"");
    }
};

// NPC

enum class NpcTextureGroups : uint16_t
{
    Npc = 0,

    _Last = Npc
};

struct NpcTextureDatabaseTraits
{
    static inline std::string DatabaseName = "NPC";

    using TextureGroups = NpcTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "NPC"))
            return TextureGroups::Npc;
        else
            throw GameException("Unrecognized NPC texture group \"" + str + "\"");
    }
};


// Font

enum class FontTextureGroups : uint16_t
{
    Font = 0,

    _Last = Font
};

}
