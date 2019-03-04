/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Materials.h"

#include <GameCore/Utils.h>

StructuralMaterial StructuralMaterial::Create(picojson::object const & structuralMaterialJson)
{
    std::string name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        float strength = static_cast<float>(Utils::GetMandatoryJsonMember<double>(structuralMaterialJson, "strength"));

        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float mass = static_cast<float>(Utils::GetMandatoryJsonMember<double>(massJson, "nominal_mass"))
            * static_cast<float>(Utils::GetMandatoryJsonMember<double>(massJson, "density"));

        float stiffness = static_cast<float>(Utils::GetOptionalJsonMember<double>(structuralMaterialJson, "stiffness", 1.0));

        vec4f renderColor =
            Utils::Hex2RgbColor(
                Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "render_color"))
            .toVec4f(1.0f);

        bool isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
        float waterVolumeFill = static_cast<float>(Utils::GetMandatoryJsonMember<double>(structuralMaterialJson, "water_volume_fill"));
        float waterIntake = static_cast<float>(Utils::GetOptionalJsonMember<double>(structuralMaterialJson, "water_intake", 1.0));
        float waterDiffusionSpeed = static_cast<float>(Utils::GetMandatoryJsonMember<double>(structuralMaterialJson, "water_diffusion_speed"));
        float waterRetention = static_cast<float>(Utils::GetMandatoryJsonMember<double>(structuralMaterialJson, "water_retention"));

        float windReceptivity = static_cast<float>(Utils::GetOptionalJsonMember<double>(structuralMaterialJson, "wind_receptivity", 0.0));

        std::optional<MaterialUniqueType> uniqueType;
        if (name == "Air")
            uniqueType = MaterialUniqueType::Air;
        else if (name == "Rope")
            uniqueType = MaterialUniqueType::Rope;

        std::optional<std::string> materialSoundStr = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "sound_type");
        std::optional<MaterialSoundType> materialSound;
        if (!!materialSoundStr)
            materialSound = StrToMaterialSoundType(*materialSoundStr);

        return StructuralMaterial(
            name,
            strength,
            mass,
            stiffness,
            renderColor,
            isHull,
            waterVolumeFill,
            waterIntake,
            waterDiffusionSpeed,
            waterRetention,
            windReceptivity,
            uniqueType,
            materialSound);
    }
    catch (GameException const & ex)
    {
        throw GameException(std::string("Error parsing structural material \"") + name + "\": " + ex.what());
    }
}

StructuralMaterial::MaterialSoundType StructuralMaterial::StrToMaterialSoundType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "airbubble")
        return MaterialSoundType::AirBubble;
    else if (lstr == "cable")
        return MaterialSoundType::Cable;
    else if (lstr == "cloth")
        return MaterialSoundType::Cloth;
    else if (lstr == "glass")
        return MaterialSoundType::Glass;
    else if (lstr == "metal")
        return MaterialSoundType::Metal;
    else if (lstr == "wood")
        return MaterialSoundType::Wood;
    else
        throw GameException("Unrecognized MaterialSoundType \"" + str + "\"");
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
        float luminiscence = 0.0f;
        float lightSpread = 0.0f;
        float wetFailureRate = 0.0f;
        if (ElectricalElementType::Lamp == electricalType)
        {
            isSelfPowered = Utils::GetMandatoryJsonMember<bool>(electricalMaterialJson, "is_self_powered");
            luminiscence = static_cast<float>(Utils::GetMandatoryJsonMember<double>(electricalMaterialJson, "luminiscence"));
            lightSpread = static_cast<float>(Utils::GetMandatoryJsonMember<double>(electricalMaterialJson, "light_spread"));
            wetFailureRate = static_cast<float>(Utils::GetMandatoryJsonMember<double>(electricalMaterialJson, "wet_failure_rate"));

            if (lightSpread < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the light_spread parameter must be greater than or equal 0.0");
            if (wetFailureRate < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the wet_failure_rate parameter must be greater than or equal 0.0");
        }

        return ElectricalMaterial(
            name,
            electricalType,
            isSelfPowered,
            luminiscence,
            lightSpread,
            wetFailureRate);
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