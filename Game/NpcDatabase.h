/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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

    // Only movable
    NpcDatabase(NpcDatabase const & other) = delete;
    NpcDatabase(NpcDatabase && other) = default;
    NpcDatabase & operator=(NpcDatabase const & other) = delete;
    NpcDatabase & operator=(NpcDatabase && other) = default;

    static NpcDatabase Load(
        ResourceLocator const & resourceLocator,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetHumanSubKinds(std::string const & language) const;
    std::vector<std::tuple<NpcSubKindIdType, std::string>> GetFurnitureSubKinds(std::string const & language) const;

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

    struct HumanRole
    {
        MultiLingualText Name;
        float SizeMultiplier;
    };

    struct FurnitureKind
    {
        MultiLingualText Name;
    };

private:

    NpcDatabase(
        std::map<NpcSubKindIdType, HumanRole> && humanRoles,
        std::map<NpcSubKindIdType, FurnitureKind> && furnitureKinds)
        : mHumanRoles(std::move(humanRoles))
        , mFurnitureKinds(std::move(furnitureKinds))
    {}

    static HumanRole ParseHumanRole(
        picojson::object const & roleObject,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static FurnitureKind ParseFurnitureKind(
        picojson::object const & kindObject,
        Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas);

    static MultiLingualText ParseMultilingualText(
        picojson::object const & containerObject,
        std::string const & textName);

    template<typename TNpcSubKindContainer>
    static std::vector<std::tuple<NpcSubKindIdType, std::string>> GetSubKinds(
        TNpcSubKindContainer const & container,
        std::string const & language);

private:

    std::map<NpcSubKindIdType, HumanRole> mHumanRoles;
    std::map<NpcSubKindIdType, FurnitureKind> mFurnitureKinds;
};
