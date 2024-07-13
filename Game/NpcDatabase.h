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
    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetFurnitureSubKinds(std::string const & language) const;

    NpcHumanRoleType GetHumanRole(NpcSubKindIdType subKindId) const
    {
        return mHumanKinds.at(subKindId).Role;
    }

    StructuralMaterial const & GetHumanHeadMaterial(NpcSubKindIdType subKindId) const
    {
        return mHumanKinds.at(subKindId).HeadMaterial;
    }

    StructuralMaterial const & GetHumanFeetMaterial(NpcSubKindIdType subKindId) const
    {
        return mHumanKinds.at(subKindId).FeetMaterial;
    }

    float GetHumanSizeMultiplier(NpcSubKindIdType subKindId) const
    {
        return mHumanKinds.at(subKindId).SizeMultiplier;
    }

    HumanTextureFramesType const & GetHumanTextureCoordinatesQuads(NpcSubKindIdType subKindId) const
    {
        return mHumanKinds.at(subKindId).TextureCoordinatesQuads;
    }

    StructuralMaterial const & GetFurnitureMaterial(NpcSubKindIdType subKindId) const
    {
        return mFurnitureKinds.at(subKindId).Material;
    }

    ParticleMeshKindType const & GetFurnitureParticleMeshKindType(NpcSubKindIdType subKindId) const
    {
        return mFurnitureKinds.at(subKindId).ParticleMeshKind;
    }

    float GetFurnitureWidth(NpcSubKindIdType subKindId) const
    {
        return mFurnitureKinds.at(subKindId).Width;
    }

    float GetFurnitureHeight(NpcSubKindIdType subKindId) const
    {
        return mFurnitureKinds.at(subKindId).Height;
    }

    Render::TextureCoordinatesQuad const & GetFurnitureTextureCoordinatesQuad(NpcSubKindIdType subKindId) const
    {
        return mFurnitureKinds.at(subKindId).TextureCoordinatesQuad;
    }

private:

    struct MultiLingualText
    {
        std::map<std::string, std::string> ValuesByLanguage;

        explicit MultiLingualText(std::map<std::string, std::string> && valuesByLanguage)
            : ValuesByLanguage(std::move(valuesByLanguage))
        {}

        std::string Get(std::string const & language) const
        {
            auto searchIt = ValuesByLanguage.find(language);
            if (searchIt != ValuesByLanguage.end())
                return searchIt->second;

            searchIt = ValuesByLanguage.find("");
            assert(searchIt != ValuesByLanguage.end());
            return searchIt->second;
        }
    };

    struct HumanKind
    {
        MultiLingualText Name;
        NpcHumanRoleType Role;

        StructuralMaterial const & HeadMaterial;
        StructuralMaterial const & FeetMaterial;

        float SizeMultiplier;

        HumanTextureFramesType const TextureCoordinatesQuads;
    };

    struct FurnitureKind
    {
        MultiLingualText Name;

        StructuralMaterial const & Material;

        ParticleMeshKindType ParticleMeshKind;
        float Height;
        float Width;

        Render::TextureCoordinatesQuad TextureCoordinatesQuad;
    };

private:

    NpcDatabase(
        std::map<NpcSubKindIdType, HumanKind> && humanKinds,
        std::map<NpcSubKindIdType, FurnitureKind> && furnitureKinds)
        : mHumanKinds(std::move(humanKinds))
        , mFurnitureKinds(std::move(furnitureKinds))
    {}

    static HumanKind ParseHumanKind(
        picojson::object const & kindObject,
        StructuralMaterial const & headMaterial,
        StructuralMaterial const & feetMaterial,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static FurnitureKind ParseFurnitureKind(
        picojson::object const & kindObject,
        MaterialDatabase const & materialDatabase,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static MultiLingualText ParseMultilingualText(
        picojson::object const & containerObject,
        std::string const & textName);

    static Render::TextureCoordinatesQuad ParseTextureCoordinatesQuad(
        picojson::object const & containerObject,
        std::string const & memberName,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    template<typename TNpcSubKindContainer>
    static std::vector<std::tuple<NpcSubKindIdType, std::string>> GetSubKinds(
        TNpcSubKindContainer const & container,
        std::string const & language);

    static ParticleMeshKindType StrToParticleMeshKindType(std::string const & str);

private:

    std::map<NpcSubKindIdType, HumanKind> mHumanKinds;
    std::map<NpcSubKindIdType, FurnitureKind> mFurnitureKinds;
};
