/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <GameCore/Colors.h>
#include <GameCore/GameException.h>
#include <GameCore/Vectors.h>

#include <picojson.h>

#include <optional>
#include <string>

struct StructuralMaterial
{
public:

    enum class MaterialCombustionType
    {
        Combustion,
        Explosion
    };

    enum class MaterialUniqueType : size_t
    {
        Air = 0,
        Rope = 1,

        _Last = Rope
    };

    enum class MaterialSoundType
    {
        AirBubble,
        Cable,
        Cloth,
        Glass,
        Metal,
        Plastic,
        Rubber,
        Wood,
    };

public:

    std::string Name;
    float Strength;
    float NominalMass;
    float Density;
    float BuoyancyVolumeFill;
    float Stiffness;
    vec4f RenderColor;

    std::optional<MaterialUniqueType> UniqueType;

    std::optional<MaterialSoundType> MaterialSound;

    // Water
    bool IsHull;
    float WaterIntake;
    float WaterDiffusionSpeed;
    float WaterRetention;
    float RustReceptivity;

    // Heat
    float IgnitionTemperature; // K
    float MeltingTemperature; // K
    float ThermalConductivity; // W/(m*K)
    float ThermalExpansionCoefficient; // 1/K
    float SpecificHeat; // J/(Kg*K)
    MaterialCombustionType CombustionType;

    // Misc
    float WindReceptivity;
    bool IsLegacyElectrical;

public:

    static StructuralMaterial Create(picojson::object const & structuralMaterialJson);

    static MaterialSoundType StrToMaterialSoundType(std::string const & str);
    static MaterialCombustionType StrToMaterialCombustionType(std::string const & str);

    bool IsUniqueType(MaterialUniqueType uniqueType) const
    {
        return !!UniqueType && (*UniqueType == uniqueType);
    }

    /*
     * Returns the mass of this particle, calculated assuming that the particle is a cubic meter
     * full of a quantity of material equal to the density; for example, an iron truss has a lower
     * density than solid iron.
     */
    float GetMass() const
    {
        return NominalMass * Density;
    }

    /*
     * Returns the heat capacity of the material, in J/K.
     */
    float GetHeatCapacity() const
    {
        return SpecificHeat * GetMass();
    }

    StructuralMaterial(
        std::string name,
        float strength,
        float nominalMass,
        float density,
        float buoyancyVolumeFill,
        float stiffness,
        vec4f renderColor,
        std::optional<MaterialUniqueType> uniqueType,
        std::optional<MaterialSoundType> materialSound,
        // Water
        bool isHull,
        float waterIntake,
        float waterDiffusionSpeed,
        float waterRetention,
        float rustReceptivity,
        // Heat
        float ignitionTemperature,
        float meltingTemperature,
        float thermalConductivity,
        float thermalExpansionCoefficient,
        float specificHeat,
        MaterialCombustionType combustionType,
        // Misc
        float windReceptivity,
        bool isLegacyElectrical)
        : Name(name)
        , Strength(strength)
        , NominalMass(nominalMass)
        , Density(density)
        , BuoyancyVolumeFill(buoyancyVolumeFill)
        , Stiffness(stiffness)
        , RenderColor(renderColor)
        , UniqueType(uniqueType)
        , MaterialSound(materialSound)
        , IsHull(isHull)
        , WaterIntake(waterIntake)
        , WaterDiffusionSpeed(waterDiffusionSpeed)
        , WaterRetention(waterRetention)
        , RustReceptivity(rustReceptivity)
        , IgnitionTemperature(ignitionTemperature)
        , MeltingTemperature(meltingTemperature)
        , ThermalConductivity(thermalConductivity)
        , ThermalExpansionCoefficient(thermalExpansionCoefficient)
        , SpecificHeat(specificHeat)
        , CombustionType(combustionType)
        , WindReceptivity(windReceptivity)
        , IsLegacyElectrical(isLegacyElectrical)
    {}
};

struct ElectricalMaterial
{
public:

    enum class ElectricalElementType
    {
        Lamp,
        Cable,
        Engine,
        EngineController,
        Generator,
        InteractiveToggleSwitch,
        InteractivePushSwitch,
        OtherSink,
        PowerMonitor,
        SmokeEmitter,
        WaterSensingSwitch
    };

    enum class EngineElementType
    {
        SteamEngine
    };

public:

    std::string Name;

    ElectricalElementType ElectricalType;

    bool IsSelfPowered;
    bool ConductsElectricity;

    // Light
    float Luminiscence;
    vec4f LightColor;
    float LightSpread;
    float WetFailureRate; // Number of lamp failures per minute

    // Heat
    float HeatGenerated; // KJ/s
    float MinimumOperatingTemperature; // K
    float MaximumOperatingTemperature; // K

    // Particle Emission
    float ParticleEmissionRate; // Number of particles per second

    // Engine
    EngineElementType EngineType;
    float EngineCCWDirection; // CCW radians at positive power
    float EngineResponsiveness; // Coefficient for RPM recursive function

    // Instancing
    bool IsInstanced; // When true, only one particle may exist with a given (full) color key

public:

    static ElectricalMaterial Create(picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);

    static EngineElementType StrToEngineElementType(std::string const & str);

    ElectricalMaterial(
        std::string name,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        bool conductsElectricity,
        float luminiscence,
        vec4f lightColor,
        float lightSpread,
        float wetFailureRate,
        float heatGenerated,
        float minimumOperatingTemperature,
        float maximumOperatingTemperature,
        float particleEmissionRate,
        EngineElementType engineType,
        float engineCCWDirection,
        float engineResponsiveness,
        bool isInstanced)
        : Name(name)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , ConductsElectricity(conductsElectricity)
        , Luminiscence(luminiscence)
        , LightColor(lightColor)
        , LightSpread(lightSpread)
        , WetFailureRate(wetFailureRate)
        , HeatGenerated(heatGenerated)
        , MinimumOperatingTemperature(minimumOperatingTemperature)
        , MaximumOperatingTemperature(maximumOperatingTemperature)
        , ParticleEmissionRate(particleEmissionRate)
        , EngineType(engineType)
        , EngineCCWDirection(engineCCWDirection)
        , EngineResponsiveness(engineResponsiveness)
        , IsInstanced(isInstanced)
    {
    }
};
