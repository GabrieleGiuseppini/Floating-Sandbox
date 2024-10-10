/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "RenderTypes.h"
#include "ResourceLocator.h"
#include "TextureAtlas.h"
#include "TextureTypes.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <cassert>
#include <map>
#include <string>
#include <vector>

/*
 * Information over the different sub-kinds of NPCs.
 */
class NpcDatabase
{
public:

    enum class ParticleMeshKindType
    {
        Particle,
        Dipole,
        Quad
    };

    struct ParticleAttributesType
    {
        float BuoyancyVolumeFill;
        float SpringReductionFraction;
        float SpringDampingCoefficient;
    };

    struct HumanDimensionsType
    {
        float HeadWFactor; // Multiplier of "standard" head width
        float HeadWidthToHeightFactor; // To recover head texture quad height from its physical width
        float TorsoHeightToWidthFactor; // To recover torso texture quad width from its physical height
        float ArmHeightToWidthFactor; // To recover arm texture quad width from its physical height
        float LegHeightToWidthFactor; // To recover leg texture quad width from its physical height
    };

    struct FurnitureDimensionsType
    {
        float Width;
        float Height;
    };

    struct HumanTextureFramesType
    {
        Render::TextureCoordinatesQuad HeadFront;
        Render::TextureCoordinatesQuad HeadBack;
        Render::TextureCoordinatesQuad HeadSide;

        Render::TextureCoordinatesQuad TorsoFront;
        Render::TextureCoordinatesQuad TorsoBack;
        Render::TextureCoordinatesQuad TorsoSide;

        Render::TextureCoordinatesQuad ArmFront;
        Render::TextureCoordinatesQuad ArmBack;
        Render::TextureCoordinatesQuad ArmSide;

        Render::TextureCoordinatesQuad LegFront;
        Render::TextureCoordinatesQuad LegBack;
        Render::TextureCoordinatesQuad LegSide;
    };

public:

    // Only movable
    NpcDatabase(NpcDatabase const & other) = delete;
    NpcDatabase(NpcDatabase && other) = default;
    NpcDatabase & operator=(NpcDatabase const & other) = delete;
    NpcDatabase & operator=(NpcDatabase && other) = default;

    static NpcDatabase Load(
        ResourceLocator const & resourceLocator,
        MaterialDatabase const & materialDatabase,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetHumanSubKinds(std::string const & language) const;

    std::vector<std::vector<NpcSubKindIdType>> const & GetHumanSubKindIdsByRole() const
    {
        return mHumanSubKindIdsByRole;
    }

    NpcHumanRoleType GetHumanRole(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).Role;
    }

    rgbColor GetHumanRenderColor(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).RenderColor;
    }

    StructuralMaterial const & GetHumanHeadMaterial(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).HeadMaterial;
    }

    StructuralMaterial const & GetHumanFeetMaterial(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).FeetMaterial;
    }

    ParticleAttributesType const & GetHumanHeadParticleAttributes(NpcSubKindIdType subKindId) const
    {
        assert(mHumanSubKinds.at(subKindId).ParticleAttributes.size() == 2);
        return mHumanSubKinds.at(subKindId).ParticleAttributes[1];
    }

    ParticleAttributesType const & GetHumanFeetParticleAttributes(NpcSubKindIdType subKindId) const
    {
        assert(mHumanSubKinds.at(subKindId).ParticleAttributes.size() == 2);
        return mHumanSubKinds.at(subKindId).ParticleAttributes[0];
    }

    float GetHumanSizeMultiplier(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).SizeMultiplier;
    }

    HumanDimensionsType const & GetHumanDimensions(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).Dimensions;
    }

    float GetHumanBodyWidthRandomizationSensitivity(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).BodyWidthRandomizationSensitivity;
    }

    HumanTextureFramesType const & GetHumanTextureCoordinatesQuads(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).TextureCoordinatesQuads;
    }

    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetFurnitureSubKinds(std::string const & language) const;

    std::vector<std::vector<NpcSubKindIdType>> const & GetFurnitureSubKindIdsByRole() const
    {
        return mFurnitureSubKindIdsByRole;
    }

    NpcFurnitureRoleType GetFurnitureRole(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).Role;
    }

    rgbColor GetFurnitureRenderColor(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).RenderColor;
    }

    StructuralMaterial const & GetFurnitureMaterial(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).Material;
    }

    ParticleAttributesType const & GetFurnitureParticleAttributes(NpcSubKindIdType subKindId, int particleOrdinal) const
    {
        assert(particleOrdinal < mFurnitureSubKinds.at(subKindId).ParticleAttributes.size());
        return mFurnitureSubKinds.at(subKindId).ParticleAttributes[particleOrdinal];
    }

    ParticleMeshKindType const & GetFurnitureParticleMeshKindType(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).ParticleMeshKind;
    }

    FurnitureDimensionsType const & GetFurnitureDimensions(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).Dimensions;
    }

    Render::TextureCoordinatesQuad const & GetFurnitureTextureCoordinatesQuad(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).TextureCoordinatesQuad;
    }

private:

    struct HumanSubKind
    {
        std::string Name;
        NpcHumanRoleType Role;
        rgbColor RenderColor;

        StructuralMaterial const & HeadMaterial;
        StructuralMaterial const & FeetMaterial;

        std::array<ParticleAttributesType, 2> ParticleAttributes;

        float SizeMultiplier;

        HumanDimensionsType Dimensions;
        float BodyWidthRandomizationSensitivity;
        HumanTextureFramesType const TextureCoordinatesQuads;
    };

    struct FurnitureSubKind
    {
        std::string Name;
        NpcFurnitureRoleType Role;
        rgbColor RenderColor;

        StructuralMaterial const & Material;

        std::vector<ParticleAttributesType> ParticleAttributes;

        ParticleMeshKindType ParticleMeshKind;

        FurnitureDimensionsType Dimensions;

        Render::TextureCoordinatesQuad TextureCoordinatesQuad;
    };

private:

    struct StringEntry
    {
        std::string Language;
        std::string Value;

        StringEntry(
            std::string const & language,
            std::string const & value)
            : Language(language)
            , Value(value)
        {}
    };

    // Keyed by name
    using StringTable = std::unordered_map<std::string, std::vector<StringEntry>>;

    NpcDatabase(
        std::map<NpcSubKindIdType, HumanSubKind> && humanSubKinds,
        std::map<NpcSubKindIdType, FurnitureSubKind> && furnitureSubKinds,
        StringTable && stringTable);

    static HumanSubKind ParseHumanSubKind(
        picojson::object const & subKindObject,
        StructuralMaterial const & headMaterial,
        StructuralMaterial const & feetMaterial,
        ParticleAttributesType const & globalHeadParticleAttributes,
        ParticleAttributesType const & globalFeetParticleAttributes,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static HumanDimensionsType CalculateHumanDimensions(
        picojson::object const & containerObject,
        float headTextureBaseWidth,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas,
        std::string const & subKindName);

    static ImageSize GetFrameSize(
        picojson::object const & containerObject,
        std::string const & frameNameMemberName,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static FurnitureSubKind ParseFurnitureSubKind(
        picojson::object const & subKindObject,
        MaterialDatabase const & materialDatabase,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static ParticleAttributesType MakeParticleAttributes(
        picojson::object const & containerObject,
        std::string const & particleAttributesOverrideMemberName,
        ParticleAttributesType const & defaultParticleAttributes);

    static ParticleAttributesType MakeParticleAttributes(
        picojson::object const & particleAttributesOverrideJsonObject,
        ParticleAttributesType const & defaultParticleAttributes);

    static ParticleAttributesType MakeDefaultParticleAttributes(StructuralMaterial const & baseMaterial);

    static Render::TextureCoordinatesQuad ParseTextureCoordinatesQuad(
        picojson::object const & containerObject,
        std::string const & memberName,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    template<typename TNpcSubKindContainer>
    static std::vector<std::tuple<NpcSubKindIdType, std::string>> GetSubKinds(
        TNpcSubKindContainer const & container,
        StringTable const & stringTable,
        std::string const & language);

    static ParticleMeshKindType StrToParticleMeshKindType(std::string const & str);

    static StringTable ParseStringTable(
        picojson::object const & containerObject,
        std::map<NpcSubKindIdType, HumanSubKind> const & humanSubKinds,
        std::map<NpcSubKindIdType, FurnitureSubKind> const & furnitureSubKinds);

private:

    std::map<NpcSubKindIdType, HumanSubKind> mHumanSubKinds;
    std::map<NpcSubKindIdType, FurnitureSubKind> mFurnitureSubKinds;
    StringTable mStringTable;

    std::vector<std::vector<NpcSubKindIdType>> mHumanSubKindIdsByRole;
    std::vector<std::vector<NpcSubKindIdType>> mFurnitureSubKindIdsByRole;
};
