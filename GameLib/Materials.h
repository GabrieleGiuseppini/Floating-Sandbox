/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameException.h"
#include "Vectors.h"

#include <picojson/picojson.h>

#include <string>

struct StructuralMaterial
{
    std::string Name;
    float Strength;
    float Mass;
    float Stiffness;
    vec4f RenderColor;
    bool IsHull;
    bool IsRope;

    enum class SoundElementType
    {
        Cable,
        Glass,
        Metal,
        Wood,
    };

    SoundElementType SoundType;

    StructuralMaterial(
        std::string name,
        float strength,
        float mass,
        float stiffness,
        vec4f renderColor,
        bool isHull,
        bool isRope,
        SoundElementType soundType)
        : Name(name)
        , Strength(strength)
        , Mass(mass)
        , Stiffness(stiffness)
        , RenderColor(renderColor)
        , IsHull(isHull)
        , IsRope(isRope)
        , SoundType(soundType)
    {}

    static StructuralMaterial Create(picojson::object const & structuralMaterialJson);

    static SoundElementType StrToSoundElementType(std::string const & str);
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

    ElectricalMaterial(
        std::string name,
        ElectricalElementType electricalType,
        bool isSelfPowered)
        : Name(name)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
    {
    }

    static ElectricalMaterial Create(picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);
};
