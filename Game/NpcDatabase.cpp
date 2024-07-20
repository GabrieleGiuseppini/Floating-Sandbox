/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NpcDatabase.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>

static char HeadFKeyName[] = "head_f";
static char HeadBKeyName[] = "head_b";
static char HeadSKeyName[] = "head_s";
static char TorsoFKeyName[] = "torso_f";
static char TorsoBKeyName[] = "torso_b";
static char TorsoSKeyName[] = "torso_s";
static char ArmFKeyName[] = "arm_f";
static char ArmBKeyName[] = "arm_b";
static char ArmSKeyName[] = "arm_s";
static char LegFKeyName[] = "leg_f";
static char LegBKeyName[] = "leg_b";
static char LegSKeyName[] = "leg_s";

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
    auto const dimensions = CalculateHumanDimensions(textureFilenameStemsObject, npcTextureAtlas);
    HumanTextureFramesType humanTextureFrames({
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, HeadFKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, HeadBKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, HeadSKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, TorsoFKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, TorsoBKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, TorsoSKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, ArmFKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, ArmBKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, ArmSKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, LegFKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, LegBKeyName, npcTextureAtlas),
        ParseTextureCoordinatesQuad(textureFilenameStemsObject, LegSKeyName, npcTextureAtlas)
        });

    return HumanKind({
        std::move(name),
        role,
        headMaterial,
        feetMaterial,
        sizeMultiplier,
        dimensions,
        humanTextureFrames });
}

NpcDatabase::HumanDimensionsType NpcDatabase::CalculateHumanDimensions(
    picojson::object const & containerObject,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    // Head
    //
    // - Fixed width; expected B/F/S to be the same

    auto const headFSize = GetFrameSize(containerObject, HeadFKeyName, npcTextureAtlas);
    float const headHWRatio = static_cast<float>(headFSize.height) / static_cast<float>(headFSize.width);

    // Torso
    //
    // - Fixed height; expected B/F/S to be the same

    auto const torsoFSize = GetFrameSize(containerObject, TorsoFKeyName, npcTextureAtlas);
    float const torsoWHRatio = static_cast<float>(torsoFSize.width) / static_cast<float>(torsoFSize.height);

    // Arm
    //
    // - Fixed height; expected B/F/S to be the same

    auto const armFSize = GetFrameSize(containerObject, ArmFKeyName, npcTextureAtlas);
    float const armWHRatio = static_cast<float>(armFSize.width) / static_cast<float>(armFSize.height);

    // Leg
    //
    // - Fixed height; expected B/F/S to be the same

    auto const legFSize = GetFrameSize(containerObject, LegFKeyName, npcTextureAtlas);
    float const legWHRatio = static_cast<float>(legFSize.width) / static_cast<float>(legFSize.height);

    return HumanDimensionsType({
        headHWRatio,
        torsoWHRatio,
        armWHRatio,
        legWHRatio });
}

ImageSize NpcDatabase::GetFrameSize(
    picojson::object const & containerObject,
    std::string const & frameNameMemberName,
    Render::TextureAtlas<Render::NpcTextureGroups> const & npcTextureAtlas)
{
    std::string const & frameFilenameStem = Utils::GetMandatoryJsonMember<std::string>(containerObject, frameNameMemberName);
    auto const & atlasFrameMetadata = npcTextureAtlas.Metadata.GetFrameMetadata(frameFilenameStem);
    return atlasFrameMetadata.FrameMetadata.Size;
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

    FurnitureDimensionsType dimensions{ 0.0f, 0.0f };
    switch (particleMeshKind)
    {
        case ParticleMeshKindType::Dipole:
        {
            dimensions = FurnitureDimensionsType({
                0,
                Utils::GetMandatoryJsonMember<float>(particleMeshObject, "height") });

            break;
        }

        case ParticleMeshKindType::Particle:
        {
            dimensions = FurnitureDimensionsType({
                0,
                0 });

            break;
        }

        case ParticleMeshKindType::Quad:
        {
            float const height = Utils::GetMandatoryJsonMember<float>(particleMeshObject, "height");

            // Calculate width based off texture frame
            float const textureFrameAspectRatio =
                static_cast<float>(atlasFrameMetadata.FrameMetadata.Size.width)
                / static_cast<float>(atlasFrameMetadata.FrameMetadata.Size.height);
            float const width = height * textureFrameAspectRatio;

            dimensions = FurnitureDimensionsType({
                width,
                height });

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
        dimensions,
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