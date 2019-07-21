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
    float Stiffness;
    vec4f RenderColor;

    std::optional<MaterialUniqueType> UniqueType;

    std::optional<MaterialSoundType> MaterialSound;

    // Water
    bool IsHull;
    float WaterVolumeFill;
    float WaterIntake;
    float WaterDiffusionSpeed;
    float WaterRetention;
    float RustReceptivity;

    // Heat
    float IgnitionTemperature; // K
    float MeltingTemperature; // K
    float ThermalConductivity; // W/(m*K)
    float SpecificHeat; // J/(Kg*K)
    MaterialCombustionType CombustionType;

    // Misc
    float WindReceptivity;

public:

    static StructuralMaterial Create(picojson::object const & structuralMaterialJson);

    static MaterialSoundType StrToMaterialSoundType(std::string const & str);
    static MaterialCombustionType StrToMaterialCombustionType(std::string const & str);

    bool IsUniqueType(MaterialUniqueType uniqueType) const
    {
        return !!UniqueType && *UniqueType == uniqueType;
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
        float stiffness,
        vec4f renderColor,
        std::optional<MaterialUniqueType> uniqueType,
        std::optional<MaterialSoundType> materialSound,
        // Water
        bool isHull,
        float waterVolumeFill,
        float waterIntake,
        float waterDiffusionSpeed,
        float waterRetention,
        float rustReceptivity,
        // Heat
        float ignitionTemperature,
        float meltingTemperature,
        float thermalConductivity,
        float specificHeat,
        MaterialCombustionType combustionType,
        // Misc
        float windReceptivity)
        : Name(name)
        , Strength(strength)
        , NominalMass(nominalMass)
        , Density(density)
        , Stiffness(stiffness)
        , RenderColor(renderColor)
        , UniqueType(uniqueType)
        , MaterialSound(materialSound)
        , IsHull(isHull)
        , WaterIntake(waterIntake)
        , WaterVolumeFill(waterVolumeFill)
        , WaterDiffusionSpeed(waterDiffusionSpeed)
        , WaterRetention(waterRetention)
        , RustReceptivity(rustReceptivity)
        , IgnitionTemperature(ignitionTemperature)
        , MeltingTemperature(meltingTemperature)
        , ThermalConductivity(thermalConductivity)
        , SpecificHeat(specificHeat)
        , CombustionType(combustionType)
        , WindReceptivity(windReceptivity)
    {}
};

struct ElectricalMaterial
{
    std::string Name;

    enum class ElectricalElementType
    {
        Lamp,
        Cable,
        Generator,
        OtherSink
    };

    ElectricalElementType ElectricalType;

    bool IsSelfPowered;

    // Light
    float Luminiscence;
    vec4f LightColor;
    float LightSpread;
    float WetFailureRate; // Number of lamp failures per minute

    // Heat
    float HeatGenerated; // KJ/s
    float MinimumOperatingTemperature; // K
    float MaximumOperatingTemperature; // K

    static ElectricalMaterial Create(picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);

    ElectricalMaterial(
        std::string name,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        float luminiscence,
        vec4f lightColor,
        float lightSpread,
        float wetFailureRate,
        float heatGenerated,
        float minimumOperatingTemperature,
        float maximumOperatingTemperature)
        : Name(name)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , Luminiscence(luminiscence)
        , LightColor(lightColor)
        , LightSpread(lightSpread)
        , WetFailureRate(wetFailureRate)
        , HeatGenerated(heatGenerated)
        , MinimumOperatingTemperature(minimumOperatingTemperature)
        , MaximumOperatingTemperature(maximumOperatingTemperature)
    {
    }
};
