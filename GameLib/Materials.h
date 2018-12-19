/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameException.h"
#include "Vectors.h"

#include <picojson/picojson.h>

#include <optional>
#include <string>

struct StructuralMaterial
{
    std::string Name;
    float Strength;
    float Mass;
    float Stiffness;
    vec4f RenderColor;

    bool IsHull;
    float WaterVolumeFill;
    float WaterDiffusionSpeed;
    float WaterRetention;

    enum class MaterialUniqueType : size_t
    {
        Air = 0,
        Rope = 1,

        _Last = Rope
    };

    std::optional<MaterialUniqueType> UniqueType;

    enum class MaterialSoundType
    {
        AirBubble,
        Cable,
        Cloth,
        Glass,
        Metal,
        Wood,
    };

    std::optional<MaterialSoundType> MaterialSound;

public:

    static StructuralMaterial Create(picojson::object const & structuralMaterialJson);

    static MaterialSoundType StrToMaterialSoundType(std::string const & str);

    bool IsUniqueType(MaterialUniqueType uniqueType) const
    {
        return !!UniqueType && *UniqueType == uniqueType;
    }

private:

    StructuralMaterial(
        std::string name,
        float strength,
        float mass,
        float stiffness,
        vec4f renderColor,
        bool isHull,
        float waterVolumeFill,
        float waterDiffusionSpeed,
        float waterRetention,
        std::optional<MaterialUniqueType> uniqueType,
        std::optional<MaterialSoundType> materialSound)
        : Name(name)
        , Strength(strength)
        , Mass(mass)
        , Stiffness(stiffness)
        , RenderColor(renderColor)
        , IsHull(isHull)
        , WaterVolumeFill(waterVolumeFill)
        , WaterDiffusionSpeed(waterDiffusionSpeed)
        , WaterRetention(waterRetention)
        , UniqueType(uniqueType)
        , MaterialSound(materialSound)
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
    float LightSpread;

    static ElectricalMaterial Create(picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);

private:

    ElectricalMaterial(
        std::string name,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        float luminiscence,
        float lightSpread)
        : Name(name)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , Luminiscence(luminiscence)
        , LightSpread(lightSpread)
    {
    }
};
