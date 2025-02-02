/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "MaterialDatabase.h"
#include "Materials.h"

#include <Render/GameTextureDatabases.h>

#include <Core/GameTypes.h>
#include <Core/IAssetManager.h>
#include <Core/TextureAtlas.h>

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
        float FrictionSurfaceAdjustment; // To account for diminished surface of structures in contact with floor
    };

    struct HumanTextureGeometryType
    {
        float HeadLengthFraction; // Fraction of dipole length; can overshoot 1.0 - leg+torso for e.g. hats
        float HeadWHRatio; // To recover head quad width from height
        float TorsoLengthFraction; // Fraction of dipole length
        float TorsoWHRatio; // To recover torso quad width from height
        float ArmLengthFraction; // Fraction of dipole length
        float ArmWHRatio; // To recover arm quad width from height
        float LegLengthFraction; // Fraction of dipole length
        float LegWHRatio; // To recover leg quad width from height
    };

    struct FurnitureGeometryType
    {
        float Width;
        float Height;
    };

    struct HumanTextureFramesType
    {
        TextureCoordinatesQuad HeadFront;
        TextureCoordinatesQuad HeadBack;
        TextureCoordinatesQuad HeadSide;

        TextureCoordinatesQuad TorsoFront;
        TextureCoordinatesQuad TorsoBack;
        TextureCoordinatesQuad TorsoSide;

        TextureCoordinatesQuad ArmFront;
        TextureCoordinatesQuad ArmBack;
        TextureCoordinatesQuad ArmSide;

        TextureCoordinatesQuad LegFront;
        TextureCoordinatesQuad LegBack;
        TextureCoordinatesQuad LegSide;
    };

public:

    // Only movable
    NpcDatabase(NpcDatabase const & other) = delete;
    NpcDatabase(NpcDatabase && other) = default;
    NpcDatabase & operator=(NpcDatabase const & other) = delete;
    NpcDatabase & operator=(NpcDatabase && other) = default;

    static NpcDatabase Load(
        IAssetManager const & assetManager,
        MaterialDatabase const & materialDatabase,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas);

    // Humans

    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetHumanSubKinds(
        NpcHumanRoleType role,
        std::string const & language) const;

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

    float GetHumanBodyWidthRandomizationSensitivity(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).BodyWidthRandomizationSensitivity;
    }

    HumanTextureFramesType const & GetHumanTextureCoordinatesQuads(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).TextureCoordinatesQuads;
    }

    HumanTextureGeometryType const & GetHumanTextureGeometry(NpcSubKindIdType subKindId) const
    {
        return mHumanSubKinds.at(subKindId).TextureGeometry;
    }

    // Furniture

    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetFurnitureSubKinds(
        NpcFurnitureRoleType role,
        std::string const & language) const;

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

    FurnitureGeometryType const & GetFurnitureGeometry(NpcSubKindIdType subKindId) const
    {
        return mFurnitureSubKinds.at(subKindId).Geometry;
    }

    TextureCoordinatesQuad const & GetFurnitureTextureCoordinatesQuad(NpcSubKindIdType subKindId) const
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
        float BodyWidthRandomizationSensitivity;

        HumanTextureFramesType const TextureCoordinatesQuads;
        HumanTextureGeometryType TextureGeometry;
    };

    struct FurnitureSubKind
    {
        std::string Name;
        NpcFurnitureRoleType Role;
        rgbColor RenderColor;

        StructuralMaterial const & Material;

        std::vector<ParticleAttributesType> ParticleAttributes;

        ParticleMeshKindType ParticleMeshKind;

        FurnitureGeometryType Geometry;

        TextureCoordinatesQuad TextureCoordinatesQuad;
    };

private:

    struct DefaultHumanTextureGeometryType
    {
        float HeadLengthFraction;
        std::optional<float> HeadWHRatio;
        float TorsoLengthFraction;
        std::optional<float> TorsoWHRatio;
        float ArmLengthFraction;
        std::optional<float> ArmWHRatio;
        float LegLengthFraction;
        std::optional<float> LegWHRatio;
    };

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
        DefaultHumanTextureGeometryType const & defaultTextureGeometry,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas);

    static DefaultHumanTextureGeometryType ParseDefaultHumanTextureGeometry(
        picojson::object const & containerObject);

    static HumanTextureGeometryType ParseHumanTextureGeometry(
        picojson::object const & containerObject,
        DefaultHumanTextureGeometryType const & defaults,
        picojson::object const & textureFilenameStemsContainerObject,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas,
        std::string const & subKindName);

    static ImageSize GetFrameSize(
        picojson::object const & containerObject,
        std::string const & frameNameMemberName,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas);

    static FurnitureSubKind ParseFurnitureSubKind(
        picojson::object const & subKindObject,
        MaterialDatabase const & materialDatabase,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas);

    static ParticleAttributesType MakeParticleAttributes(
        picojson::object const & containerObject,
        std::string const & particleAttributesOverrideMemberName,
        ParticleAttributesType const & defaultParticleAttributes);

    static ParticleAttributesType MakeParticleAttributes(
        picojson::object const & particleAttributesOverrideJsonObject,
        ParticleAttributesType const & defaultParticleAttributes);

    static ParticleAttributesType MakeDefaultParticleAttributes(StructuralMaterial const & baseMaterial);

    static TextureCoordinatesQuad ParseTextureCoordinatesQuad(
        picojson::object const & containerObject,
        std::string const & memberName,
        TextureAtlas<GameTextureDatabases::NpcTextureDatabase> const & npcTextureAtlas);

    template<typename TNpcSubKindContainer, typename TNpcRoleType>
    static std::vector<std::tuple<NpcSubKindIdType, std::string>> GetSubKinds(
        TNpcSubKindContainer const & container,
        StringTable const & stringTable,
        TNpcRoleType role,
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
