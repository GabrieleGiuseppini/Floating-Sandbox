/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <GameCore/Colors.h>
#include <GameCore/GameException.h>
#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <picojson.h>

#include <optional>
#include <string>

struct MaterialPaletteCoordinatesType
{
    std::string Category;
    std::string SubCategory;
    unsigned int SubCategoryOrdinal; // Ordinal in SubCategory
};

struct StructuralMaterial
{
public:

    static MaterialLayerType constexpr MaterialLayer = MaterialLayerType::Structural;

    enum class MaterialCombustionType
    {
        Combustion,
        Explosion
    };

    enum class MaterialUniqueType : size_t // There's an array indexed by this
    {
        Air = 0,
        Glass,
        Rope,
        Water,

        _Last = Water
    };

    enum class MaterialSoundType
    {
        AirBubble,
        Cable,
        Chain,
        Cloth,
        Gas,
        Glass,
        Human,
        Lego,
        Metal,
        Piano,
        Plastic,
        Rubber,
        RubberBand,
        Wood,
    };

public:

    MaterialColorKey ColorKey;

    std::string Name;
    rgbaColor RenderColor;
    float Strength;
    float NominalMass;
    float Density;
    float BuoyancyVolumeFill; // Hull has usually 0.0 (as it never gets water yet we want it to sink), and non-hull usually \1.0 (as it gets water)
    float Stiffness;
    float StrainThresholdFraction;
    float ElasticityCoefficient;
    float KineticFrictionCoefficient;
    float StaticFrictionCoefficient;

    std::optional<MaterialUniqueType> UniqueType;

    std::optional<MaterialSoundType> MaterialSound;

    std::optional<std::string> MaterialTextureName;
    float Opacity;

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
    float ExplosiveCombustionForce; // KN
    float ExplosiveCombustionForceRadius; // m
    float ExplosiveCombustionHeat; // KJoules/sec
    float ExplosiveCombustionHeatRadius; // m

    // Misc
    float WindReceptivity;
    float WaterReactivity; // When > 0, material explodes with this quantity of water threshold
    bool IsLegacyElectrical;

    // Palette
    std::optional<MaterialPaletteCoordinatesType> PaletteCoordinates;

public:

    static StructuralMaterial Create(
        MaterialColorKey const & colorKey,
        unsigned int ordinal,
        rgbColor const & baseRenderColor,
        picojson::object const & structuralMaterialJson);

    static MaterialCombustionType StrToMaterialCombustionType(std::string const & str);
    static MaterialUniqueType StrToMaterialUniqueType(std::string const & str);
    static MaterialSoundType StrToMaterialSoundType(std::string const & str);

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
        MaterialColorKey const & colorKey,
        std::string name,
        rgbaColor const & renderColor,
        float strength,
        float nominalMass,
        float density,
        float buoyancyVolumeFill,
        float stiffness,
        float strainThresholdFraction,
        float elasticityCoefficient,
        float kineticFrictionCoefficient,
        float staticFrictionCoefficient,
        std::optional<MaterialUniqueType> uniqueType,
        std::optional<MaterialSoundType> materialSound,
        std::optional<std::string> materialTextureName,
        float opacity,
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
        float explosiveCombustionForce,
        float explosiveCombustionForceRadius,
        float explosiveCombustionHeat,
        float explosiveCombustionHeatRadius,
        // Misc
        float windReceptivity,
        float waterReactivity,
        bool isLegacyElectrical,
        // Palette
        std::optional<MaterialPaletteCoordinatesType> paletteCoordinates)
        : ColorKey(colorKey)
        , Name(name)
        , RenderColor(renderColor)
        , Strength(strength)
        , NominalMass(nominalMass)
        , Density(density)
        , BuoyancyVolumeFill(buoyancyVolumeFill)
        , Stiffness(stiffness)
        , StrainThresholdFraction(strainThresholdFraction)
        , ElasticityCoefficient(elasticityCoefficient)
        , KineticFrictionCoefficient(kineticFrictionCoefficient)
        , StaticFrictionCoefficient(staticFrictionCoefficient)
        , UniqueType(uniqueType)
        , MaterialSound(materialSound)
        , MaterialTextureName(materialTextureName)
        , Opacity(opacity)
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
        , ExplosiveCombustionForce(explosiveCombustionForce)
        , ExplosiveCombustionForceRadius(explosiveCombustionForceRadius)
        , ExplosiveCombustionHeat(explosiveCombustionHeat)
        , ExplosiveCombustionHeatRadius(explosiveCombustionHeatRadius)
        , WindReceptivity(windReceptivity)
        , WaterReactivity(waterReactivity)
        , IsLegacyElectrical(isLegacyElectrical)
        , PaletteCoordinates(paletteCoordinates)
    {}

    // For tests
    StructuralMaterial(
        MaterialColorKey const & colorKey,
        std::string name,
        rgbaColor const & renderColor)
        : ColorKey(colorKey)
        , Name(name)
        , RenderColor(renderColor)
        , Strength(1.0f)
        , NominalMass(1.0f)
        , Density(1.0f)
        , BuoyancyVolumeFill(1.0f)
        , Stiffness(1.0f)
        , StrainThresholdFraction(0.5f)
        , UniqueType(std::nullopt)
        , MaterialSound(std::nullopt)
        , MaterialTextureName(std::nullopt)
        , Opacity(1.0f)
        , IsHull(false)
        , WaterIntake(1.0f)
        , WaterDiffusionSpeed(1.0f)
        , WaterRetention(1.0f)
        , RustReceptivity(1.0f)
        , IgnitionTemperature(200.0f)
        , MeltingTemperature(200.0f)
        , ThermalConductivity(1.0f)
        , ThermalExpansionCoefficient(1.0f)
        , SpecificHeat(1.0f)
        , CombustionType(MaterialCombustionType::Combustion)
        , ExplosiveCombustionForce(0.0f)
        , ExplosiveCombustionForceRadius(1.0f)
        , ExplosiveCombustionHeat(0.0f)
        , ExplosiveCombustionHeatRadius(1.0f)
        , WindReceptivity(1.0f)
        , WaterReactivity(0.0f)
        , IsLegacyElectrical(false)
        , PaletteCoordinates(std::nullopt)
    {}
};

struct ElectricalMaterial
{
public:

    static MaterialLayerType constexpr MaterialLayer = MaterialLayerType::Electrical;

    enum class ElectricalElementType
    {
        Cable,
        Engine,
        EngineController,
        EngineTransmission,
        Generator,
        InteractiveSwitch,
        Lamp,
        OtherSink,
        PowerMonitor,
        ShipSound,
        SmokeEmitter,
        WaterPump,
        WaterSensingSwitch,
        WatertightDoor
    };

    enum class EngineElementType
    {
        Diesel,
        Jet,
        Outboard,
        Steam
    };

    enum class EngineControllerElementType
    {
        Telegraph,
        JetThrottle,
        JetThrust
    };

    enum class InteractiveSwitchElementType
    {
        Push,
        Toggle
    };

    enum class ShipSoundElementType
    {
        Bell1,
        Bell2,
        QueenMaryHorn,
        FourFunnelLinerWhistle,
        TripodHorn,
        PipeWhistle,
        LakeFreighterHorn,
        ShieldhallSteamSiren,
        QueenElizabeth2Horn,
        SSRexWhistle,
        Klaxon1,
        NuclearAlarm1,
        EvacuationAlarm1,
        EvacuationAlarm2
    };

public:

    MaterialColorKey ColorKey;

    std::string Name;
    rgbColor RenderColor;

    ElectricalElementType ElectricalType;

    bool IsSelfPowered;
    bool ConductsElectricity;

    // Lamp
    float Luminiscence;
    vec4f LightColor;
    float LightSpread;
    float WetFailureRate; // Number of lamp failures per minute
    float ExternalPressureBreakageThreshold; // KPa

    // Heat
    float HeatGenerated; // KJ/s
    float MinimumOperatingTemperature; // K
    float MaximumOperatingTemperature; // K

    // Particle Emission
    float ParticleEmissionRate; // Number of particles per second

    // Instancing
    bool IsInstanced; // When true, only one particle may exist with a given (full) color key

    // Engine
    EngineElementType EngineType;
    float EngineCCWDirection; // CCW radians at positive power
    float EnginePower; // Thrust at max RPM, HP
    float EngineResponsiveness; // Coefficient for RPM recursive function

    // Engine Controller
    EngineControllerElementType EngineControllerType;

    // Interactive switch
    InteractiveSwitchElementType InteractiveSwitchType;

    // Ship sound
    ShipSoundElementType ShipSoundType;

    // Water pump
    float WaterPumpNominalForce;

    // Palette
    std::optional<MaterialPaletteCoordinatesType> PaletteCoordinates;

public:

    static ElectricalMaterial Create(
        MaterialColorKey const & colorKey,
        unsigned int ordinal,
        rgbColor const & renderColor,
        picojson::object const & electricalMaterialJson);

    static ElectricalElementType StrToElectricalElementType(std::string const & str);

    static InteractiveSwitchElementType StrToInteractiveSwitchElementType(std::string const & str);

    static EngineElementType StrToEngineElementType(std::string const & str);

    static EngineControllerElementType StrToEngineControllerElementType(std::string const & str);

    static ShipSoundElementType StrToShipSoundElementType(std::string const & str);

    ElectricalMaterial(
        MaterialColorKey const & colorKey,
        std::string name,
        rgbColor const & renderColor,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        bool conductsElectricity,
        float luminiscence,
        vec4f lightColor,
        float lightSpread,
        float wetFailureRate,
        float externalPressureBreakageThreshold,
        float heatGenerated,
        float minimumOperatingTemperature,
        float maximumOperatingTemperature,
        float particleEmissionRate,
        bool isInstanced,
        EngineElementType engineType,
        float engineCCWDirection,
        float enginePower,
        float engineResponsiveness,
        EngineControllerElementType engineControllerType,
        InteractiveSwitchElementType interactiveSwitchType,
        ShipSoundElementType shipSoundType,
        float waterPumpNominalForce,
        std::optional<MaterialPaletteCoordinatesType> paletteCoordinates)
        : ColorKey(colorKey)
        , Name(name)
        , RenderColor(renderColor)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , ConductsElectricity(conductsElectricity)
        , Luminiscence(luminiscence)
        , LightColor(lightColor)
        , LightSpread(lightSpread)
        , WetFailureRate(wetFailureRate)
        , ExternalPressureBreakageThreshold(externalPressureBreakageThreshold)
        , HeatGenerated(heatGenerated)
        , MinimumOperatingTemperature(minimumOperatingTemperature)
        , MaximumOperatingTemperature(maximumOperatingTemperature)
        , ParticleEmissionRate(particleEmissionRate)
        //
        , IsInstanced(isInstanced)
        , EngineType(engineType)
        , EngineCCWDirection(engineCCWDirection)
        , EnginePower(enginePower)
        , EngineResponsiveness(engineResponsiveness)
        , EngineControllerType(engineControllerType)
        , InteractiveSwitchType(interactiveSwitchType)
        , ShipSoundType(shipSoundType)
        , WaterPumpNominalForce(waterPumpNominalForce)
        , PaletteCoordinates(paletteCoordinates)
    {
    }

    // For tests
    ElectricalMaterial(
        MaterialColorKey const & colorKey,
        std::string name,
        rgbColor const & renderColor,
        bool isInstanced)
        : ColorKey(colorKey)
        , Name(name)
        , RenderColor(renderColor)
        , ElectricalType(ElectricalElementType::Cable)
        , IsSelfPowered(false)
        , ConductsElectricity(true)
        , Luminiscence(1.0f)
        , LightColor(vec4f::zero())
        , LightSpread(1.0f)
        , WetFailureRate(0.0f)
        , ExternalPressureBreakageThreshold(100000.0f)
        , HeatGenerated(0.0f)
        , MinimumOperatingTemperature(0.0f)
        , MaximumOperatingTemperature(1000.0f)
        , ParticleEmissionRate(1.0f)
        //
        , IsInstanced(isInstanced)
        , EngineType(EngineElementType::Diesel)
        , EngineCCWDirection(1.0f)
        , EnginePower(1.0f)
        , EngineResponsiveness(1.0f)
        , EngineControllerType(EngineControllerElementType::Telegraph)
        , InteractiveSwitchType(InteractiveSwitchElementType::Push)
        , ShipSoundType(ShipSoundElementType::Bell1)
        , WaterPumpNominalForce(0.0f)
        , PaletteCoordinates(std::nullopt)
    {
    }

    std::string MakeInstancedElementLabel(ElectricalElementInstanceIndex instanceIndex) const;
};
