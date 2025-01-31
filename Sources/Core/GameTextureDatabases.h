/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Utils.h"

#include <string>

//
// All the texture databases in the game
//

// Cloud

enum class CloudTextureGroups : uint16_t
{
    Cloud = 0,

    _Last = Cloud
};

struct CloudTextureDatabase
{
    static inline std::string DatabaseName = "Cloud";

    using TextureGroupsType = CloudTextureGroups;

    static CloudTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Cloud"))
            return CloudTextureGroups::Cloud;
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

struct WorldTextureDatabase
{
    static inline std::string DatabaseName = "World";

    using TextureGroupsType = WorldTextureGroups;

    static WorldTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Land"))
            return WorldTextureGroups::Land;
        else if (Utils::CaseInsensitiveEquals(str, "Ocean"))
            return WorldTextureGroups::Ocean;
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

struct NoiseTextureDatabase
{
    static inline std::string DatabaseName = "Noise";

    using TextureGroupsType = NoiseTextureGroups;

    static NoiseTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Noise"))
            return NoiseTextureGroups::Noise;
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

struct GenericMipMappedTextureDatabase
{
    static inline std::string DatabaseName = "GenericMipMappedTexture";

    using TextureGroupsType = GenericMipMappedTextureGroups;

    static GenericMipMappedTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
            return GenericMipMappedTextureGroups::AirBubble;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombArmor"))
            return GenericMipMappedTextureGroups::AntiMatterBombArmor;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphereCloud"))
            return GenericMipMappedTextureGroups::AntiMatterBombSphereCloud;
        else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphere"))
            return GenericMipMappedTextureGroups::AntiMatterBombSphere;
        else if (Utils::CaseInsensitiveEquals(str, "EngineWake"))
            return GenericMipMappedTextureGroups::EngineWake;
        else if (Utils::CaseInsensitiveEquals(str, "FireExtinguishingBomb"))
            return GenericMipMappedTextureGroups::FireExtinguishingBomb;
        else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
            return GenericMipMappedTextureGroups::ImpactBomb;
        else if (Utils::CaseInsensitiveEquals(str, "LaserCannon"))
            return GenericMipMappedTextureGroups::LaserCannon;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbe"))
            return GenericMipMappedTextureGroups::PhysicsProbe;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePing"))
            return GenericMipMappedTextureGroups::PhysicsProbePing;
        else if (Utils::CaseInsensitiveEquals(str, "PinnedPoint"))
            return GenericMipMappedTextureGroups::PinnedPoint;
        else if (Utils::CaseInsensitiveEquals(str, "RCBomb"))
            return GenericMipMappedTextureGroups::RcBomb;
        else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
            return GenericMipMappedTextureGroups::RcBombPing;
        else if (Utils::CaseInsensitiveEquals(str, "SmokeDark"))
            return GenericMipMappedTextureGroups::SmokeDark;
        else if (Utils::CaseInsensitiveEquals(str, "SmokeLight"))
            return GenericMipMappedTextureGroups::SmokeLight;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBomb"))
            return GenericMipMappedTextureGroups::TimerBomb;
        else if (Utils::CaseInsensitiveEquals(str, "TimerBombFuse"))
            return GenericMipMappedTextureGroups::TimerBombFuse;
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

struct GenericLinearTextureDatabase
{
    static inline std::string DatabaseName = "GenericLinearTexture";

    using TextureGroupsType = GenericLinearTextureGroups;

    static GenericLinearTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "AutoFocusNotification"))
            return GenericLinearTextureGroups::AutoFocusNotification;
        else if (Utils::CaseInsensitiveEquals(str, "DayLightCycleNotification"))
            return GenericLinearTextureGroups::DayLightCycleNotification;
        else if (Utils::CaseInsensitiveEquals(str, "Fire"))
            return GenericLinearTextureGroups::Fire;
        else if (Utils::CaseInsensitiveEquals(str, "PhysicsProbePanel"))
            return GenericLinearTextureGroups::PhysicsProbePanel;
        else if (Utils::CaseInsensitiveEquals(str, "ShiftNotification"))
            return GenericLinearTextureGroups::ShiftNotification;
        else if (Utils::CaseInsensitiveEquals(str, "SoundMuteNotification"))
            return GenericLinearTextureGroups::SoundMuteNotification;
        else if (Utils::CaseInsensitiveEquals(str, "UVModeNotification"))
            return GenericLinearTextureGroups::UVModeNotification;
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

struct ExplosionTextureDatabase
{
    static inline std::string DatabaseName = "Explosion";

    using TextureGroupsType = ExplosionTextureGroups;

    static ExplosionTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Explosion"))
            return ExplosionTextureGroups::Explosion;
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

struct FishTextureDatabase
{
    static inline std::string DatabaseName = "Fish";

    using TextureGroupsType = FishTextureGroups;

    static FishTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Fish"))
            return FishTextureGroups::Fish;
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

struct NpcTextureDatabase
{
    static inline std::string DatabaseName = "NPC";

    using TextureGroupsType = NpcTextureGroups;

    static NpcTextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "NPC"))
            return NpcTextureGroups::Npc;
        else
            throw GameException("Unrecognized NPC texture group \"" + str + "\"");
    }
};
