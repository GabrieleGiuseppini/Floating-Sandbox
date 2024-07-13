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
    MaterialDatabase const & materialDatabase,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    picojson::value const root = Utils::ParseJSONFile(resourceLocator.GetNpcDatabaseFilePath());
    if (!root.is<picojson::object>())
    {
        throw GameException("NPC database is not a JSON object");
    }

    picojson::object const & rootObject = root.get<picojson::object>();

    std::map<NpcSubKindIdType, HumanKind> humanKinds;
    std::map<NpcSubKindIdType, FurnitureKind> furnitureKinds;

    //
    // Humans
    //

    {
        auto const humansObject = Utils::GetMandatoryJsonObject(rootObject, "humans");

        auto const humansGlobalObject = Utils::GetMandatoryJsonObject(humansObject, "global");

        StructuralMaterial const & headMaterial = materialDatabase.GetStructuralMaterial(
            Utils::GetMandatoryJsonMember<std::string>(humansGlobalObject, "head_material"));

        StructuralMaterial const & feetMaterial = materialDatabase.GetStructuralMaterial(
            Utils::GetMandatoryJsonMember<std::string>(humansGlobalObject, "feet_material"));

        NpcSubKindIdType nextKindId = 0;
        auto const humanKindsArray = Utils::GetMandatoryJsonArray(humansObject, "kinds");
        for (auto const & humanKindArrayElement : humanKindsArray)
        {
            if (!humanKindArrayElement.is<picojson::object>())
            {
                throw GameException("Human NPC kind array element is not a JSON object");
            }

            HumanKind kind = ParseHumanKind(
                humanKindArrayElement.get<picojson::object>(),
                headMaterial,
                feetMaterial,
                npcTextureAtlas);

            humanKinds.try_emplace(nextKindId, std::move(kind));
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
                materialDatabase,
                npcTextureAtlas);

            furnitureKinds.try_emplace(nextKindId, std::move(kind));
            ++nextKindId;
        }
    }

    //
    // Wrap it up
    //

    return NpcDatabase(
        std::move(humanKinds),
        std::move(furnitureKinds));
}

std::vector<std::tuple<NpcSubKindIdType, std::string>> NpcDatabase::GetHumanSubKinds(std::string const & language) const
{
    return GetSubKinds(mHumanKinds, language);
}

std::vector<std::tuple<NpcSubKindIdType, std::string>> NpcDatabase::GetFurnitureSubKinds(std::string const & language) const
{
    return GetSubKinds(mFurnitureKinds, language);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

NpcDatabase::HumanKind NpcDatabase::ParseHumanKind(
    picojson::object const & kindObject,
    StructuralMaterial const & headMaterial,
    StructuralMaterial const & feetMaterial,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    MultiLingualText name = ParseMultilingualText(kindObject, "name");
    NpcHumanRoleType const role = StrToNpcHumanRoleType(Utils::GetMandatoryJsonMember<std::string>(kindObject, "role"));
    float const sizeMultiplier = Utils::GetOptionalJsonMember<float>(kindObject, "size_multiplier", 1.0f);

    auto const & textureFilenameStemsObject = Utils::GetMandatoryJsonObject(kindObject, "texture_filename_stems");
    HumanTextureFramesType humanTextureFrames({
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "head_f", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "head_b", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "head_s", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "torso_f", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "torso_b", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "torso_s", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "arm_f", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "arm_b", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "arm_s", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "leg_f", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "leg_b", npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, "leg_s", npcTextureAtlas)
        });

    return HumanKind({
        std::move(name),
        role,
        headMaterial,
        feetMaterial,
        sizeMultiplier,
        std::move(humanTextureFrames) });
}

NpcDatabase::FurnitureKind NpcDatabase::ParseFurnitureKind(
    picojson::object const & kindObject,
    MaterialDatabase const & materialDatabase,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    MultiLingualText name = ParseMultilingualText(kindObject, "name");

    StructuralMaterial const & material = materialDatabase.GetStructuralMaterial(
        Utils::GetMandatoryJsonMember<std::string>(kindObject, "material"));

    std::string const & frameFilenameStem = Utils::GetMandatoryJsonMember<std::string>(kindObject, "texture_filename_stem");
    auto const & atlasFrameMetadata = npcTextureAtlas.Metadata.GetFrameMetadata(frameFilenameStem);

    auto const & particleMeshObject = Utils::GetMandatoryJsonObject(kindObject, "particle_mesh");
    ParticleMeshKindType const particleMeshKind = StrToParticleMeshKindType(
        Utils::GetMandatoryJsonMember<std::string>(particleMeshObject, "kind"));

    float height = 0.0f, width = 0.0f;
    switch (particleMeshKind)
    {
        case ParticleMeshKindType::Dipole:
        {
            height = Utils::GetMandatoryJsonMember<float>(particleMeshObject, "height");
            width = 0;

            break;
        }

        case ParticleMeshKindType::Particle:
        {
            height = 0;
            width = 0;

            break;
        }

        case ParticleMeshKindType::Quad:
        {
            height = Utils::GetMandatoryJsonMember<float>(particleMeshObject, "height");

            // Calculate width based off texture frame
            float const textureFrameAspectRatio =
                static_cast<float>(atlasFrameMetadata.FrameMetadata.Size.width)
                / static_cast<float>(atlasFrameMetadata.FrameMetadata.Size.height);
            width = height * textureFrameAspectRatio;

            break;
        }
    }

    Render::TextureCoordinatesQuad textureCoordinatesQuad = Render::TextureCoordinatesQuad({
        atlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        atlasFrameMetadata.TextureCoordinatesTopRight.x,
        atlasFrameMetadata.TextureCoordinatesBottomLeft.y,
        atlasFrameMetadata.TextureCoordinatesTopRight.y });

    return FurnitureKind({
        std::move(name),
        material,
        particleMeshKind,
        height,
        width,
        std::move(textureCoordinatesQuad) });
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

Render::TextureCoordinatesQuad NpcDatabase::ParseTextureCoordinatesQuad(
    picojson::object const & containerObject,
    std::string const & memberName,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    std::string const & frameFilenameStem = Utils::GetMandatoryJsonMember<std::string>(containerObject, memberName);
    auto const & atlasFrameMetadata = npcTextureAtlas.Metadata.GetFrameMetadata(frameFilenameStem);
    return Render::TextureCoordinatesQuad({
        atlasFrameMetadata.TextureCoordinatesBottomLeft.x,
        atlasFrameMetadata.TextureCoordinatesTopRight.x,
        atlasFrameMetadata.TextureCoordinatesBottomLeft.y,
        atlasFrameMetadata.TextureCoordinatesTopRight.y });
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

NpcDatabase::ParticleMeshKindType NpcDatabase::StrToParticleMeshKindType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Dipole"))
        return ParticleMeshKindType::Dipole;
    else if (Utils::CaseInsensitiveEquals(str, "Particle"))
        return ParticleMeshKindType::Particle;
    else if (Utils::CaseInsensitiveEquals(str, "Quad"))
        return ParticleMeshKindType::Quad;
    else
        throw GameException("Unrecognized ParticleMeshKindType \"" + str + "\"");
}