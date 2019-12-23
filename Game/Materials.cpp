/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Materials.h"

#include <GameCore/Utils.h>

StructuralMaterial StructuralMaterial::Create(picojson::object const & structuralMaterialJson)
{
    std::string const name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        float const strength = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "strength");

        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float const nominalMass = Utils::GetMandatoryJsonMember<float>(massJson, "nominal_mass");
        float const density = Utils::GetMandatoryJsonMember<float>(massJson, "density");

        float const stiffness = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "stiffness", 1.0);

        vec4f const renderColor =
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

        bool const isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
        float const waterVolumeFill = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_volume_fill");
        float const waterIntake = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_intake", 1.0);
        float const waterDiffusionSpeed = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_diffusion_speed");
        float const waterRetention = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "water_retention");
        float const rustReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "rust_receptivity", 1.0);

        // Heat

        float const ignitionTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "ignition_temperature");
        float const meltingTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "melting_temperature");
        float const thermalConductivity = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "thermal_conductivity");
        float const thermalExpansionCoefficient = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "thermal_expansion_coefficient", 0.0);
        float const specificHeat = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "specific_heat");
        MaterialCombustionType const combustionType = StrToMaterialCombustionType(Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "combustion_type"));

        // Misc

        float const windReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "wind_receptivity", 0.0);
        bool isLegacyElectrical = Utils::GetOptionalJsonMember<bool>(structuralMaterialJson, "is_legacy_electrical", false);

        return StructuralMaterial(
            name,
            strength,
            nominalMass,
            density,
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
            thermalExpansionCoefficient,
            specificHeat,
            combustionType,
            // Misc
            windReceptivity,
            isLegacyElectrical);
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

        // Heat
        float heatGenerated = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "heat_generated");
        float minimumOperatingTemperature = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "minimum_operating_temperature");
        float maximumOperatingTemperature = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "maximum_operating_temperature");

        // Particle emitter properties
        float particleEmissionRate = 0.0f;
        if (ElectricalElementType::SmokeEmitter == electricalType)
        {
            particleEmissionRate = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "particle_emission_rate");

            if (particleEmissionRate < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"particle_emission_rate\" parameter must be greater than or equal 0.0");
        }

        return ElectricalMaterial(
            name,
            electricalType,
            isSelfPowered,
            luminiscence,
            lightColor,
            lightSpread,
            wetFailureRate,
            heatGenerated,
            minimumOperatingTemperature,
            maximumOperatingTemperature,
            particleEmissionRate);
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
    else if (lstr == "othersink")
        return ElectricalElementType::OtherSink;
    else if (lstr == "smokeemitter")
        return ElectricalElementType::SmokeEmitter;
    else
        throw GameException("Unrecognized ElectricalElementType \"" + str + "\"");
}