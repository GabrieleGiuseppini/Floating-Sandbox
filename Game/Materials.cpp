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
        float strength = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "strength");

        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float mass = Utils::GetMandatoryJsonMember<float>(massJson, "nominal_mass")
            * Utils::GetMandatoryJsonMember<float>(massJson, "density");

        float stiffness = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "stiffness", 1.0);

        vec4f renderColor =
            Utils::Hex2RgbColor(
                Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "render_color"))
            .toVec4f(1.0f);

        std::optional<MaterialUniqueType> uniqueType;
        if (name == "Air")
            uniqueType = MaterialUniqueType::Air;
        else if (name == "Rope")
            uniqueType = MaterialUniqueType::Rope;

        std::optional<std::string> materialSoundStr = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "sound_type");
        std::optional<MaterialSoundType> materialSound;
        if (!!materialSoundStr)
            materialSound = StrToMaterialSoundType(*materialSoundStr);

        // Water

        bool isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
        float waterVolumeFill = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_volume_fill");
        float waterIntake = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_intake", 1.0);
        float waterDiffusionSpeed = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_diffusion_speed");
        float waterRetention = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_retention");
        float rustReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "rust_receptivity", 1.0);

        // Heat

        float ignitionTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "ignition_temperature");
        float meltingTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "melting_temperature");
        float thermalConductivity = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "thermal_conductivity");
        float specificHeat = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "specific_heat");
        MaterialCombustionType combustionType = StrToMaterialCombustionType(Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "combustion_type"));

        // Misc

        float windReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "wind_receptivity", 0.0);

        return StructuralMaterial(
            name,
            strength,
            mass,
            stiffness,
            renderColor,
            uniqueType,
            materialSound,
            isHull,
            waterVolumeFill,
            waterIntake,
            waterDiffusionSpeed,
            waterRetention,
            rustReceptivity,
            // Heat
            ignitionTemperature,
            meltingTemperature,
            thermalConductivity,
            specificHeat,
            combustionType,
            // Misc
            windReceptivity);
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
    else if (lstr == "plastic")
        return MaterialSoundType::Plastic;
    else if (lstr == "rubber")
        return MaterialSoundType::Rubber;
    else if (lstr == "wood")
        return MaterialSoundType::Wood;
    else
        throw GameException("Unrecognized MaterialSoundType \"" + str + "\"");
}

StructuralMaterial::MaterialCombustionType StructuralMaterial::StrToMaterialCombustionType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);


    if (lstr == "combustion")
        return MaterialCombustionType::Combustion;
    else if (lstr == "explosion")
        return MaterialCombustionType::Explosion;
    else
        throw GameException("Unrecognized MaterialCombustionType \"" + str + "\"");
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
        vec4f lightColor = vec4f::zero();
        float lightSpread = 0.0f;
        float wetFailureRate = 0.0f;
        if (ElectricalElementType::Lamp == electricalType)
        {
            isSelfPowered = Utils::GetMandatoryJsonMember<bool>(electricalMaterialJson, "is_self_powered");
            luminiscence = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "luminiscence");
            lightColor =
                Utils::Hex2RgbColor(
                    Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "light_color"))
                .toVec4f(1.0f);

            lightSpread = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "light_spread");
            wetFailureRate = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "wet_failure_rate");

            if (luminiscence < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"luminiscence\" parameter must be greater than or equal 0.0");
            if (luminiscence > 1.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"luminiscence\" parameter must be less than or equal 1.0");
            if (lightSpread < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"light_spread\" parameter must be greater than or equal 0.0");
            if (wetFailureRate < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"wet_failure_rate\" parameter must be greater than or equal 0.0");
        }

        return ElectricalMaterial(
            name,
            electricalType,
            isSelfPowered,
            luminiscence,
            lightColor,
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