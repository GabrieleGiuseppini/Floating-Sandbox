/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NpcDatabase.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

NpcDatabase NpcDatabase::Load(
    ResourceLocator const & resourceLocator,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    auto const npcDatabaseFilePath = resourceLocator.GetFishSpeciesDatabaseFilePath();

    picojson::value const root = Utils::ParseJSONFile(npcDatabaseFilePath);
    if (!root.is<picojson::object>())
    {
        throw GameException("NPC database is not a JSON object");
    }

    picojson::object const & rootObject = root.get<picojson::object>();

    std::map<NpcSubKindIdType, HumanRole> humanRoles;
    std::map<NpcSubKindIdType, FurnitureKind> furnitureKinds;

    //
    // Humans
    //

    {
        auto const humansObject = Utils::GetMandatoryJsonObject(rootObject, "humans");

        auto const humansGlobalObject = Utils::GetMandatoryJsonObject(humansObject, "global");
        // TODOHERE

        NpcSubKindIdType nextKindId = 0;
        auto const humanRolesArray = Utils::GetMandatoryJsonArray(humansObject, "roles");
        for (auto const & humanRoleArrayElement : humanRolesArray)
        {
            if (!humanRoleArrayElement.is<picojson::object>())
            {
                throw GameException("Human NPC role array element is not a JSON object");
            }

            HumanRole role = ParseHumanRole(
                humanRoleArrayElement.get<picojson::object>(),
                npcTextureAtlas);

            humanRoles.try_emplace(nextKindId, std::move(role));
            ++nextKindId;
        }
    }

    //
    // Furniture
    //

    {

        auto const furnitureObject = Utils::GetMandatoryJsonObject(rootObject, "furniture");

        NpcSubKindIdType nextKindId = 0;
        auto const furnitureKindsArray = Utils::GetMandatoryJsonArray(furnitureObject, "kinds");
        for (auto const & furnitureKindArrayElement : furnitureKindsArray)
        {
            if (!furnitureKindArrayElement.is<picojson::object>())
            {
                throw GameException("Furniture NPC kind array element is not a JSON object");
            }

            FurnitureKind kind = ParseFurnitureKind(
                furnitureKindArrayElement.get<picojson::object>(),
                npcTextureAtlas);

            furnitureKinds.try_emplace(nextKindId, std::move(kind));
            ++nextKindId;
        }
    }

    //
    // Wrap it up
    //

    return NpcDatabase(
        std::move(humanRoles),
        std::move(furnitureKinds));
}

std::vector<std::tuple<NpcSubKindIdType, std::string>> NpcDatabase::GetHumanSubKinds(std::string const & language) const
{
    return GetSubKinds(mHumanRoles, language);
}

std::vector<std::tuple<NpcSubKindIdType, std::string>> NpcDatabase::GetFurnitureSubKinds(std::string const & language) const
{
    return GetSubKinds(mFurnitureKinds, language);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

NpcDatabase::HumanRole NpcDatabase::ParseHumanRole(
    picojson::object const & roleObject,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    // TODOHERE
}

NpcDatabase::FurnitureKind NpcDatabase::ParseFurnitureKind(
    picojson::object const & kindObject,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    // TODOHERE
}

NpcDatabase::MultiLingualText NpcDatabase::ParseMultilingualText(
    picojson::object const & containerObject,
    std::string const & textName)
{
    std::map<std::string, std::string> valuesByLanguageMap;

    std::string defaultValue;

    auto const entriesArray = Utils::GetMandatoryJsonArray(containerObject, textName);
    for (auto const & entryElement : entriesArray)
    {
        if (!entryElement.is<picojson::object>())
        {
            throw GameException("Multi-lingual text element for text \"" + textName + "\" in NPC database is not a JSON object");
        }

        auto const & entry = entryElement.get<picojson::object>();

        std::string const & language = Utils::GetMandatoryJsonMember<std::string>(entry, "language");
        std::string const & value = Utils::GetMandatoryJsonMember<std::string>(entry, "value");

        if (language.empty())
        {
            throw GameException("Language \"" + language + "\" is invalid for text \"" + textName + "\" in NPC database");
        }

        if (value.empty())
        {
            throw GameException("Value \"" + value + "\" is invalid for text \"" + textName + "\" in NPC database");
        }

        auto const [_, isInserted] = valuesByLanguageMap.try_emplace(Utils::ToLower(language), value);
        if (!isInserted)
        {
            throw GameException("Language \"" + language + "\" is duplicated for text \"" + textName + "\" in NPC database");
        }

        // Check if this could be a default
        if (Utils::ToLower(language) == "en")
            defaultValue = value;
    }

    if (valuesByLanguageMap.size() == 0)
    {
        throw GameException("Multi-lingual text element for text \"" + textName + "\" in NPC database is empty");
    }

    // Ensure we have a default

    if (defaultValue.empty())
    {
        // Take first one arbitrarily
        defaultValue = valuesByLanguageMap.begin()->second;
    }

    auto const [_, isInserted] = valuesByLanguageMap.try_emplace("", defaultValue);
    assert(isInserted);

    // Wrap it up

    return MultiLingualText(std::move(valuesByLanguageMap));
}

template<typename TNpcSubKindContainer>
std::vector<std::tuple<NpcSubKindIdType, std::string>> NpcDatabase::GetSubKinds(
    TNpcSubKindContainer const & container,
    std::string const & language)
{
    std::vector<std::tuple<NpcSubKindIdType, std::string>> kinds;

    for (auto const & it : container)
    {
        kinds.emplace_back(it.first, it.second.Name.Get(language));
    }

    return kinds;
}
