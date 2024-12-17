/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Materials.h"

#include "GameParameters.h"

#include <GameCore/Utils.h>

#include <sstream>

namespace /* anonymous */ {

    MaterialPaletteCoordinatesType DeserializePaletteCoordinates(picojson::object const & paletteCoordinatesJson)
    {
        MaterialPaletteCoordinatesType paletteCoordinates;
        paletteCoordinates.Category = Utils::GetMandatoryJsonMember<std::string>(paletteCoordinatesJson, "category");
        paletteCoordinates.SubCategory = Utils::GetMandatoryJsonMember<std::string>(paletteCoordinatesJson, "sub_category");
        paletteCoordinates.SubCategoryOrdinal = static_cast<unsigned int>(Utils::GetMandatoryJsonMember<int64_t>(paletteCoordinatesJson, "sub_category_ordinal"));

        return paletteCoordinates;
    }

}

StructuralMaterial StructuralMaterial::Create(
    MaterialColorKey const & colorKey,
    unsigned int ordinal,
    rgbColor const & baseRenderColor,
    picojson::object const & structuralMaterialJson)
{
    std::string const name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        float const strength = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "strength");

        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float const nominalMass = Utils::GetMandatoryJsonMember<float>(massJson, "nominal_mass");
        float const density = Utils::GetMandatoryJsonMember<float>(massJson, "density");
        float const buoyancyVolumeFill = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "buoyancy_volume_fill", 1.0f);
        float const stiffness = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "stiffness", 1.0);
        float const strainThresholdFraction = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "strain_threshold_fraction", 0.5f);
        float const elasticityCoefficient = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "elasticity_coefficient", 0.5f);
        float const kineticFrictionCoefficient = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "friction_kinetic_coefficient", 0.25f);
        float const staticFrictionCoefficient = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "friction_static_coefficient", 0.25f);

        std::optional<MaterialUniqueType> uniqueType;
        {
            std::optional<std::string> const uniqueTypeStr = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "unique_type");
            if (uniqueTypeStr.has_value() && ordinal == 0) // Assign unique type arbitrarily to first of series of colors
            {
                uniqueType = StrToMaterialUniqueType(*uniqueTypeStr);
            }
        }

        std::optional<MaterialSoundType> materialSound;
        {
            std::optional<std::string> const materialSoundStr = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "sound_type");
            if (materialSoundStr.has_value())
            {
                materialSound = StrToMaterialSoundType(*materialSoundStr);
            }
        }

        std::optional<std::string> const materialTextureName = Utils::GetOptionalJsonMember<std::string>(structuralMaterialJson, "texture_name");
        float const opacity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "opacity", 1.0f);

        // Water

        bool const isHull = Utils::GetMandatoryJsonMember<bool>(structuralMaterialJson, "is_hull");
        float const waterIntake = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_intake", 1.0);
        float const waterDiffusionSpeed = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_diffusion_speed", 0.5f);
        float const waterRetention = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_retention", 0.05f);
        float const rustReceptivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "rust_receptivity", 1.0);

        // Heat

        float const ignitionTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "ignition_temperature");
        float const meltingTemperature = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "melting_temperature");
        float const thermalConductivity = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "thermal_conductivity", 50.0f);
        float const thermalExpansionCoefficient = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "thermal_expansion_coefficient", 0.0);
        float const specificHeat = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "specific_heat", 100.0f);
        MaterialCombustionType const combustionType = StrToMaterialCombustionType(Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "combustion_type"));
        float const explosiveCombustionForce = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_force", 1.0);
        float const explosiveCombustionForceRadius = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_force_radius", 0.0);
        float const explosiveCombustionHeat = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_heat", 0.0);
        float const explosiveCombustionHeatRadius = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "explosive_combustion_heat_radius", explosiveCombustionForceRadius);

        // Misc

        float const windReceptivity = Utils::GetMandatoryJsonMember<float>(structuralMaterialJson, "wind_receptivity");
        float const waterReactivityThreshold = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "water_reactivity_threshold", 0.0f);
        bool isLegacyElectrical = Utils::GetOptionalJsonMember<bool>(structuralMaterialJson, "is_legacy_electrical", false);
        bool isExemptFromPalette = Utils::GetOptionalJsonMember<bool>(structuralMaterialJson, "is_exempt_from_palette", false);

        // Palette coordinates

        std::optional<MaterialPaletteCoordinatesType> paletteCoordinates;
        auto const & paletteCoordinatesJson = Utils::GetOptionalJsonObject(structuralMaterialJson, "palette_coordinates");
        if (!isExemptFromPalette)
        {
            if (!paletteCoordinatesJson.has_value())
            {
                throw GameException(std::string("Non-exempt structural material \"") + name + "\" doesn't have palette_coordinates member");
            }

            paletteCoordinates = DeserializePaletteCoordinates(*paletteCoordinatesJson);
            paletteCoordinates->SubCategoryOrdinal += ordinal;
        }

        return StructuralMaterial(
            colorKey,
            name,
            rgbaColor(baseRenderColor, static_cast<rgbaColor::data_type>(255.0f * opacity)), // renderColor
            strength,
            nominalMass,
            density,
            buoyancyVolumeFill,
            stiffness,
            strainThresholdFraction,
            elasticityCoefficient,
            kineticFrictionCoefficient,
            staticFrictionCoefficient,
            uniqueType,
            materialSound,
            materialTextureName,
            opacity,
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
            explosiveCombustionForce,
            explosiveCombustionForceRadius,
            explosiveCombustionHeat,
            explosiveCombustionHeatRadius,
            // Misc
            windReceptivity,
            waterReactivityThreshold,
            isLegacyElectrical,
            // Palette
            paletteCoordinates);
    }
    catch (GameException const & ex)
    {
        throw GameException(std::string("Error parsing structural material \"") + name + "\": " + ex.what());
    }
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

StructuralMaterial::MaterialUniqueType StructuralMaterial::StrToMaterialUniqueType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Air"))
        return MaterialUniqueType::Air;
    if (Utils::CaseInsensitiveEquals(str, "Glass"))
        return MaterialUniqueType::Glass;
    else if (Utils::CaseInsensitiveEquals(str, "Rope"))
        return MaterialUniqueType::Rope;
    else if (Utils::CaseInsensitiveEquals(str, "Water"))
        return MaterialUniqueType::Water;
    else
        throw GameException("Unrecognized MaterialUniqueType \"" + str + "\"");
}

StructuralMaterial::MaterialSoundType StructuralMaterial::StrToMaterialSoundType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
        return MaterialSoundType::AirBubble;
    else if (Utils::CaseInsensitiveEquals(str, "Cable"))
        return MaterialSoundType::Cable;
    else if (Utils::CaseInsensitiveEquals(str, "Chain"))
        return MaterialSoundType::Chain;
    else if (Utils::CaseInsensitiveEquals(str, "Cloth"))
        return MaterialSoundType::Cloth;
    else if (Utils::CaseInsensitiveEquals(str, "Gas"))
        return MaterialSoundType::Gas;
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
    else if (Utils::CaseInsensitiveEquals(str, "RubberBand"))
        return MaterialSoundType::RubberBand;
    else if (Utils::CaseInsensitiveEquals(str, "Wood"))
        return MaterialSoundType::Wood;
    else
        throw GameException("Unrecognized MaterialSoundType \"" + str + "\"");
}

ElectricalMaterial ElectricalMaterial::Create(
    MaterialColorKey const & colorKey,
    unsigned int ordinal,
    rgbColor const & renderColor,
    picojson::object const & electricalMaterialJson)
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
        float externalPressureBreakageThreshold = 100000.0f;
        if (ElectricalElementType::Lamp == electricalType)
        {
            luminiscence = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "luminiscence");
            lightColor =
                Utils::Hex2RgbColor(
                    Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "light_color"))
                .toVec4f(1.0f);

            lightSpread = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "light_spread");
            wetFailureRate = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "wet_failure_rate");
            externalPressureBreakageThreshold = Utils::GetMandatoryJsonMember<float>(electricalMaterialJson, "external_pressure_breakage_threshold");

            if (luminiscence < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"luminiscence\" parameter must be greater than or equal 0.0");
            if (luminiscence > 1.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"luminiscence\" parameter must be less than or equal 1.0");
            if (lightSpread < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"light_spread\" parameter must be greater than or equal 0.0");
            if (wetFailureRate < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"wet_failure_rate\" parameter must be greater than or equal 0.0");
            if (externalPressureBreakageThreshold < 0.0f)
                throw GameException("Error loading electrical material \"" + name + "\": the value of the \"externalPressureBreakageThreshold\" parameter must be greater than or equal 0.0");
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

        // Engine controller properties
        EngineControllerElementType engineControllerType = EngineControllerElementType::Telegraph; // Arbitrary
        if (ElectricalElementType::EngineController == electricalType)
        {
            std::string engineControllerTypeStr = Utils::GetMandatoryJsonMember<std::string>(electricalMaterialJson, "engine_controller_type");
            engineControllerType = StrToEngineControllerElementType(engineControllerTypeStr);
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

        // Palette coordinates
        std::optional<MaterialPaletteCoordinatesType> paletteCoordinates;
        auto const & paletteCoordinatesJson = Utils::GetOptionalJsonObject(electricalMaterialJson, "palette_coordinates");
        if (paletteCoordinatesJson.has_value())
        {
            paletteCoordinates = DeserializePaletteCoordinates(*paletteCoordinatesJson);
            paletteCoordinates->SubCategoryOrdinal += ordinal;
        }

        return ElectricalMaterial(
            colorKey,
            name,
            renderColor,
            electricalType,
            isSelfPowered,
            conductsElectricity,
            luminiscence,
            lightColor,
            lightSpread,
            wetFailureRate,
            externalPressureBreakageThreshold,
            heatGenerated,
            minimumOperatingTemperature,
            maximumOperatingTemperature,
            particleEmissionRate,
            isInstanced,
            engineType,
            engineDirection,
            enginePower,
            engineResponsiveness,
            engineControllerType,
            interactiveSwitchType,
            shipSoundType,
            waterPumpNominalForce,
            paletteCoordinates);
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
    else if (Utils::CaseInsensitiveEquals(str, "EngineTransmission"))
        return ElectricalElementType::EngineTransmission;
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
    else if (Utils::CaseInsensitiveEquals(str, "Jet"))
        return EngineElementType::Jet;
    else if (Utils::CaseInsensitiveEquals(str, "Outboard"))
        return EngineElementType::Outboard;
    else if (Utils::CaseInsensitiveEquals(str, "Steam"))
        return EngineElementType::Steam;
    else
        throw GameException("Unrecognized EngineElementType \"" + str + "\"");
}

ElectricalMaterial::EngineControllerElementType ElectricalMaterial::StrToEngineControllerElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Telegraph"))
        return EngineControllerElementType::Telegraph;
    else if (Utils::CaseInsensitiveEquals(str, "JetThrottle"))
        return EngineControllerElementType::JetThrottle;
    else if (Utils::CaseInsensitiveEquals(str, "JetThrust"))
        return EngineControllerElementType::JetThrust;
    else
        throw GameException("Unrecognized EngineElementType \"" + str + "\"");
}

ElectricalMaterial::ShipSoundElementType ElectricalMaterial::StrToShipSoundElementType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Bell1"))
        return ShipSoundElementType::Bell1;
    else if (Utils::CaseInsensitiveEquals(str, "Bell2"))
        return ShipSoundElementType::Bell2;
    else if (Utils::CaseInsensitiveEquals(str, "QueenMaryHorn"))
        return ShipSoundElementType::QueenMaryHorn;
    else if (Utils::CaseInsensitiveEquals(str, "FourFunnelLinerWhistle"))
        return ShipSoundElementType::FourFunnelLinerWhistle;
    else if (Utils::CaseInsensitiveEquals(str, "TripodHorn"))
        return ShipSoundElementType::TripodHorn;
    else if (Utils::CaseInsensitiveEquals(str, "PipeWhistle"))
        return ShipSoundElementType::PipeWhistle;
    else if (Utils::CaseInsensitiveEquals(str, "LakeFreighterHorn"))
        return ShipSoundElementType::LakeFreighterHorn;
    else if (Utils::CaseInsensitiveEquals(str, "ShieldhallSteamSiren"))
        return ShipSoundElementType::ShieldhallSteamSiren;
    else if (Utils::CaseInsensitiveEquals(str, "QueenElizabeth2Horn"))
        return ShipSoundElementType::QueenElizabeth2Horn;
    else if (Utils::CaseInsensitiveEquals(str, "SSRexWhistle"))
        return ShipSoundElementType::SSRexWhistle;
    else if (Utils::CaseInsensitiveEquals(str, "Klaxon1"))
        return ShipSoundElementType::Klaxon1;
    else if (Utils::CaseInsensitiveEquals(str, "NuclearAlarm1"))
        return ShipSoundElementType::NuclearAlarm1;
    else if (Utils::CaseInsensitiveEquals(str, "EvacuationAlarm1"))
        return ShipSoundElementType::EvacuationAlarm1;
    else if (Utils::CaseInsensitiveEquals(str, "EvacuationAlarm2"))
        return ShipSoundElementType::EvacuationAlarm2;
    else
        throw GameException("Unrecognized ShipSoundElementType \"" + str + "\"");
}

std::string ElectricalMaterial::MakeInstancedElementLabel(ElectricalElementInstanceIndex instanceIndex) const
{
    assert(IsInstanced);

    std::stringstream ss;

    switch (ElectricalType)
    {
        case ElectricalElementType::Engine:
        {
            switch(EngineType)
            {
                case EngineElementType::Jet:
                {
                    ss << "JetEngine #" << static_cast<int>(instanceIndex);
                    break;
                }

                default:
                {
                    ss << "Engine #" << static_cast<int>(instanceIndex);
                    break;
                }
            }

            break;
        }

        case ElectricalElementType::EngineController:
        {
            switch (EngineControllerType)
            {
                case EngineControllerElementType::JetThrottle:
                {
                    ss << "Jet Throttle #" << static_cast<int>(instanceIndex);
                    break;
                }

                case EngineControllerElementType::JetThrust:
                {
                    ss << "Jet Thrust #" << static_cast<int>(instanceIndex);
                    break;
                }

                case EngineControllerElementType::Telegraph:
                {
                    ss << "Engine Telegraph #" << static_cast<int>(instanceIndex);
                    break;
                }
            }

            break;
        }

        case ElectricalElementType::Generator:
        {
            ss << "Generator #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::InteractiveSwitch:
        {
            ss << "Switch " << " #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::PowerMonitor:
        {
            ss << "Monitor #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::ShipSound:
        {
            switch (ShipSoundType)
            {
                case ShipSoundElementType::Bell1:
                case ShipSoundElementType::Bell2:
                {
                    ss << "Bell #" << static_cast<int>(instanceIndex);
                    break;
                }

                case ShipSoundElementType::QueenMaryHorn:
                case ShipSoundElementType::FourFunnelLinerWhistle:
                case ShipSoundElementType::TripodHorn:
                case ShipSoundElementType::PipeWhistle:
                case ShipSoundElementType::LakeFreighterHorn:
                case ShipSoundElementType::ShieldhallSteamSiren:
                case ShipSoundElementType::QueenElizabeth2Horn:
                case ShipSoundElementType::SSRexWhistle:
                {
                    ss << "Horn #" << static_cast<int>(instanceIndex);
                    break;
                }

                case ShipSoundElementType::Klaxon1:
                case ShipSoundElementType::NuclearAlarm1:
                case ShipSoundElementType::EvacuationAlarm1:
                case ShipSoundElementType::EvacuationAlarm2:
                {
                    ss << "Alarm #" << static_cast<int>(instanceIndex);
                    break;
                }
            }

            break;
        }

        case ElectricalElementType::WaterPump:
        {
            ss << "Pump #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::WaterSensingSwitch:
        {
            ss << "WaterSwitch " << " #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::WatertightDoor:
        {
            ss << "WaterDoor " << " #" << static_cast<int>(instanceIndex);
            break;
        }

        case ElectricalElementType::Cable:
        case ElectricalElementType::EngineTransmission:
        case ElectricalElementType::Lamp:
        case ElectricalElementType::OtherSink:
        case ElectricalElementType::SmokeEmitter:
        {
            // These are never instanced
            assert(false);
        }
    }

    return ss.str();
}
