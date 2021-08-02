/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Materials.h"

#include <GameCore/Utils.h>

StructuralMaterial StructuralMaterial::Create(
    rgbColor const & renderColor,
    picojson::object const & structuralMaterialJson)
{
    std::string const name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        float const strength = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "strength");

        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float const nominalMass = Utils::GetMandatoryJsonMember<float>(massJson, "nominal_mass");
        float const density = Utils::GetMandatoryJsonMember<float>(massJson, "density");
        float const buoyancyVolumeFill = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "buoyancy_volume_fill");

        float const stiffness = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "stiffness", 1.0);

        std::optional<MaterialUniqueType> uniqueType;
        if (name == "Air")
            uniqueType = MaterialUniqueType::Air;
        else if (name == "Rope")
            uniqueType = MaterialUniqueType::Rope;
        else if (name == "Water")
            uniqueType = MaterialUniqueType::Water;

        std::optional<std::string> const materialSoundStr = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "sound_type");
        std::optional<MaterialSoundType> materialSound;
        if (!!materialSoundStr)
            materialSound = StrToMaterialSoundType(*materialSoundStr);

        std::optional<std::string> const materialTextureName = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "texture_name");

        // Water

        bool const isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
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
        float const explosiveCombustionRadius = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_radius", 0.0);
        float const explosiveCombustionStrength = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_strength", 1.0);

        // Misc

        float const windReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "wind_receptivity", 0.0);
        bool isLegacyElectrical = Utils::GetOptionalJsonMember<bool>(structuralMaterialJson, "is_legacy_electrical", false);

        return StructuralMaterial(
            name,
            strength,
            nominalMass,
            density,
            buoyancyVolumeFill,
            stiffness,
            renderColor.toVec4f(1.0f),
            uniqueType,
            materialSound,
            materialTextureName,
            isHull,
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
            explosiveCombustionRadius,
            explosiveCombustionStrength,
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
    if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
        return MaterialSoundType::AirBubble;
    else if (Utils::CaseInsensitiveEquals(str, "Cable"))
        return MaterialSoundType::Cable;
    else if (Utils::CaseInsensitiveEquals(str, "Cloth"))
        return MaterialSoundType::Cloth;
    else if (Utils::CaseInsensitiveEquals(str, "Glass"))
        return MaterialSoundType::Glass;
    else if (Utils::CaseInsensitiveEquals(str, "Lego"))
        return MaterialSoundType::Lego;
    else if (Utils::CaseInsensitiveEquals(str, "Metal"))
        return MaterialSoundType::Metal;
    else if (Utils::CaseInsensitiveEquals(str, "Plastic"))
        return MaterialSoundType::Plastic;
    else if (Utils::CaseInsensitiveEquals(str, "Rubber"))
        return MaterialSoundType::Rubber;
    else if (Utils::CaseInsensitiveEquals(str, "Wood"))
        return MaterialSoundType::Wood;
    else
        throw GameException("Unrecognized MaterialSoundType \"" + str + "\"");
}

StructuralMaterial::MaterialCombustionType StructuralMaterial::StrToMaterialCombustionType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Combustion"))
        return MaterialCombustionType::Combustion;
    else if (Utils::CaseInsensitiveEquals(str, "Explosion"))
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

        bool isSelfPowered = Utils::GetOptionalJsonMember<bool>(electricalMaterialJson, "is_self_powered", false);
        bool conductsElectricity = Utils::GetMandatoryJsonMember<bool>(electricalMaterialJson, "conducts_electricity");

        // Lamps properties
        float luminiscence = 0.0f;
        vec4f lightColor = vec4f::zero();
        float lightSpread = 0.0f;
        float wetFailureRate = 0.0f;
        if (ElectricalElementType::Lamp == electricalType)
        {
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

        // Instancing
        bool isInstanced = Utils::GetOptionalJsonMember<bool>(electricalMaterialJson, "is_instanced", false);

        // Engine properties
        EngineElementType engineType = EngineElementType::Steam; // Arbitrary
        float engineDirection = 0.0f;
        float enginePower = 0.0f;
        float engineResponsiveness = 1.0f;
        if (ElectricalElementType::Engine == electricalType)
        {
            std::string engineTypeStr = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "engine_type");
            engineType = StrToEngineElementType(engineTypeStr);

            engineDirection = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "engine_direction");

            enginePower = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "engine_power");

            engineResponsiveness = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "engine_responsiveness");

            if (engineResponsiveness <= 0.0f || engineResponsiveness > 1.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"engine_responsiveness\" parameter must be greater than 0.0 and lower than or equal 1.0");
        }

        // Interactive Switch properties
        InteractiveSwitchElementType interactiveSwitchType = InteractiveSwitchElementType::Push; // Arbitrary
        if (ElectricalElementType::InteractiveSwitch == electricalType)
        {
            std::string interactiveSwitchTypeStr = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "interactive_switch_type");
            interactiveSwitchType = StrToInteractiveSwitchElementType(interactiveSwitchTypeStr);
        }

        // Ship Sound properties
        ShipSoundElementType shipSoundType = ShipSoundElementType::Bell1; // Arbitrary
        if (ElectricalElementType::ShipSound == electricalType)
        {
            std::string shipSoundTypeStr = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "ship_sound_type");
            shipSoundType = StrToShipSoundElementType(shipSoundTypeStr);
        }

        // Water pump properties
        float waterPumpNominalForce = 0.0f;
        if (ElectricalElementType::WaterPump == electricalType)
        {
            waterPumpNominalForce = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "water_pump_nominal_force");
        }

        return ElectricalMaterial(
            name,
            electricalType,
            isSelfPowered,
            conductsElectricity,
            luminiscence,
            lightColor,
            lightSpread,
            wetFailureRate,
            heatGenerated,
            minimumOperatingTemperature,
            maximumOperatingTemperature,
            particleEmissionRate,
            isInstanced,
            engineType,
            engineDirection,
            enginePower,
            engineResponsiveness,
            interactiveSwitchType,
            shipSoundType,
            waterPumpNominalForce);
    }
    catch (GameException const & ex)
    {
        throw GameException(std::string("Error parsing electrical material \"") + name + "\": " + ex.what());
    }
}

ElectricalMaterial::ElectricalElementType ElectricalMaterial::StrToElectricalElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Cable"))
        return ElectricalElementType::Cable;
    else if (Utils::CaseInsensitiveEquals(str, "Engine"))
        return ElectricalElementType::Engine;
    else if (Utils::CaseInsensitiveEquals(str, "EngineController"))
        return ElectricalElementType::EngineController;
    else if (Utils::CaseInsensitiveEquals(str, "Generator"))
        return ElectricalElementType::Generator;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveSwitch"))
        return ElectricalElementType::InteractiveSwitch;
    else if (Utils::CaseInsensitiveEquals(str, "Lamp"))
            return ElectricalElementType::Lamp;
    else if (Utils::CaseInsensitiveEquals(str, "OtherSink"))
        return ElectricalElementType::OtherSink;
    else if (Utils::CaseInsensitiveEquals(str, "PowerMonitor"))
        return ElectricalElementType::PowerMonitor;
    else if (Utils::CaseInsensitiveEquals(str, "ShipSound"))
        return ElectricalElementType::ShipSound;
    else if (Utils::CaseInsensitiveEquals(str, "SmokeEmitter"))
        return ElectricalElementType::SmokeEmitter;
    else if (Utils::CaseInsensitiveEquals(str, "WaterPump"))
        return ElectricalElementType::WaterPump;
    else if (Utils::CaseInsensitiveEquals(str, "WaterSensingSwitch"))
        return ElectricalElementType::WaterSensingSwitch;
    else if (Utils::CaseInsensitiveEquals(str, "WatertightDoor"))
        return ElectricalElementType::WatertightDoor;
    else
        throw GameException("Unrecognized ElectricalElementType \"" + str + "\"");
}

ElectricalMaterial::InteractiveSwitchElementType ElectricalMaterial::StrToInteractiveSwitchElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Push"))
        return InteractiveSwitchElementType::Push;
    else if (Utils::CaseInsensitiveEquals(str, "Toggle"))
        return InteractiveSwitchElementType::Toggle;
    else
        throw GameException("Unrecognized InteractiveSwitchElementType \"" + str + "\"");
}

ElectricalMaterial::EngineElementType ElectricalMaterial::StrToEngineElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Diesel"))
        return EngineElementType::Diesel;
    else if (Utils::CaseInsensitiveEquals(str, "Outboard"))
        return EngineElementType::Outboard;
    else if (Utils::CaseInsensitiveEquals(str, "Steam"))
        return EngineElementType::Steam;
    else
        throw GameException("Unrecognized EngineElementType \"" + str + "\"");
}

ElectricalMaterial::ShipSoundElementType ElectricalMaterial::StrToShipSoundElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Bell1"))
        return ShipSoundElementType::Bell1;
    else if (Utils::CaseInsensitiveEquals(str, "Bell2"))
        return ShipSoundElementType::Bell2;
    else if (Utils::CaseInsensitiveEquals(str, "Horn1"))
        return ShipSoundElementType::Horn1;
    else if (Utils::CaseInsensitiveEquals(str, "Horn2"))
        return ShipSoundElementType::Horn2;
    else if (Utils::CaseInsensitiveEquals(str, "Horn3"))
        return ShipSoundElementType::Horn3;
    else if (Utils::CaseInsensitiveEquals(str, "Horn4"))
        return ShipSoundElementType::Horn4;
    else if (Utils::CaseInsensitiveEquals(str, "Klaxon1"))
        return ShipSoundElementType::Klaxon1;
    else if (Utils::CaseInsensitiveEquals(str, "NuclearAlarm1"))
        return ShipSoundElementType::NuclearAlarm1;
    else
        throw GameException("Unrecognized ShipSoundElementType \"" + str + "\"");
}