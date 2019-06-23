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
    float Mass;
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

    StructuralMaterial(
        std::string name,
        float strength,
        float mass,
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
        MaterialCombustionType combustionType,
        // Misc
        float windReceptivity)
        : Name(name)
        , Strength(strength)
        , Mass(mass)
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
        Generator
    };

    ElectricalElementType ElectricalType;

    bool IsSelfPowered;

    // Light
    float Luminiscence;
    vec4f LightColor;
    float LightSpread;
    float WetFailureRate; // Number of lamp failures per minute

    static ElectricalMaterial Create(picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);

    ElectricalMaterial(
        std::string name,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        float luminiscence,
        vec4f lightColor,
        float lightSpread,
        float wetFailureRate)
        : Name(name)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , Luminiscence(luminiscence)
        , LightColor(lightColor)
        , LightSpread(lightSpread)
        , WetFailureRate(wetFailureRate)
    {
    }
};
