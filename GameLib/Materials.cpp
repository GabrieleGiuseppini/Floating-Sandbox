/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Materials.h"

#include "Utils.h"

#include <array>

StructuralMaterial StructuralMaterial::Create(picojson::object const & structuralMaterialJson)
{
    std::string name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float mass = static_cast<float>(Utils::GetMandatoryJsonMember<double>(massJson, "nominal_mass"))
            * static_cast<float>(Utils::GetMandatoryJsonMember<double>(massJson, "density"));

        float strength = static_cast<float>(Utils::GetOptionalJsonMember<double>(structuralMaterialJson, "strength", 1.0));
        float stiffness = static_cast<float>(Utils::GetOptionalJsonMember<double>(structuralMaterialJson, "stiffness", 1.0));

        std::array<uint8_t, 3u> renderColorRgb = Utils::Hex2RgbColor(
            Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "render_color"));
        vec4f renderColor = Utils::RgbaToVec({ renderColorRgb[0], renderColorRgb[1], renderColorRgb[2], 255 });

        bool isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
        bool isRope = Utils::GetOptionalJsonMember<bool>(structuralMaterialJson, "is_rope", false);

        std::string soundTypeStr = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "sound_type");
        SoundElementType soundType = StrToSoundElementType(soundTypeStr);

        return StructuralMaterial(
            name,
            strength,
            mass,
            stiffness,
            renderColor,
            isHull,
            isRope,
            soundType);
    }
    catch (GameException const & ex)
    {
        throw GameException(std::string("Error parsing structural material \"") + name + "\": " + ex.what());
    }
}

StructuralMaterial::SoundElementType StructuralMaterial::StrToSoundElementType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "cable")
        return SoundElementType::Cable;
    else if (lstr == "glass")
        return SoundElementType::Glass;
    else if (lstr == "metal")
        return SoundElementType::Metal;
    else if (lstr == "wood")
        return SoundElementType::Wood;
    else
        throw GameException("Unrecognized SoundElementType \"" + str + "\"");
}

ElectricalMaterial ElectricalMaterial::Create(picojson::object const & electricalMaterialJson)
{
    std::string name = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "name");

    try
    {
        std::string electricalTypeStr = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "electrical_type");
        ElectricalElementType electricalType = StrToElectricalElementType(electricalTypeStr);

        // Lamps properties
        bool isSelfPowered = false;
        if (ElectricalElementType::Lamp == electricalType)
        {
            isSelfPowered = Utils::GetMandatoryJsonMember<bool>(electricalMaterialJson, "is_self_powered");
        }

        return ElectricalMaterial(
            name,
            electricalType,
            isSelfPowered);
    }
    catch (GameException const & ex)
    {
        throw GameException(std::string("Error parsing electrical material \"") + name + "\": " + ex.what());
    }
}

ElectricalMaterial::ElectricalElementType ElectricalMaterial::StrToElectricalElementType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "lamp")
        return ElectricalElementType::Lamp;
    else if (lstr == "cable")
        return ElectricalElementType::Cable;
    else if (lstr == "generator")
        return ElectricalElementType::Generator;
    else
        throw GameException("Unrecognized ElectricalElementType \"" + str + "\"");
}