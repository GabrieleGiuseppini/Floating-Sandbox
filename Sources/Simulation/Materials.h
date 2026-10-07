/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/Colors.h>
#include <Core/GameExceptions.h>
#include <Core/GameTypes.h>
#include <Core/Utils.h>
#include <Core/Vectors.h>

#include <picojson.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

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
        Explosion,
        FireExtinguishingExplosion
    };

    enum class MaterialUniqueType : size_t // There's an array indexed by this
    {
        Air = 0,
        Ash,
        Glass,
        Rope,
        SiltCloud,
        SmokeHeavy,
        SmokeLight,
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

    struct VariantOverridesType
    {
        std::string Name;
        rgbaColor RenderColor;
        float Strength;
        float Density;
        float IgnitionTemperature;
        float MeltingTemperature;
        float RotReceptivity;
        float RustReceptivity;
        float WaterSolubility;

        VariantOverridesType(
            std::string const & name,
            rgbaColor const & renderColor,
            float strength,
            float density,
            float ignitionTemperature,
            float meltingTemperature,
            float rotReceptivity,
            float rustReceptivity,
            float waterSolubility)
            : Name(name)
            , RenderColor(renderColor)
            , Strength(strength)
            , Density(density)
            , IgnitionTemperature(ignitionTemperature)
            , MeltingTemperature(meltingTemperature)
            , RotReceptivity(rotReceptivity)
            , RustReceptivity(rustReceptivity)
            , WaterSolubility(waterSolubility)
        { }

        VariantOverridesType(VariantOverridesType const & other) = default;

        bool operator==(VariantOverridesType const & other) const
        {
            return
                Name == other.Name
                && RenderColor == other.RenderColor
                && Strength == other.Strength
                && Density == other.Density
                && IgnitionTemperature == other.IgnitionTemperature
                && MeltingTemperature == other.MeltingTemperature
                && RotReceptivity == other.RotReceptivity
                && RustReceptivity == other.RustReceptivity
                && WaterSolubility == other.WaterSolubility;
        }

        bool operator<(VariantOverridesType const & other) const
        {
            return std::tie(
                Name,
                RenderColor,
                Strength,
                Density,
                IgnitionTemperature,
                MeltingTemperature,
                RotReceptivity,
                RustReceptivity,
                WaterSolubility) < std::tie(
                    other.Name,
                    other.RenderColor,
                    other.Strength,
                    other.Density,
                    other.IgnitionTemperature,
                    other.MeltingTemperature,
                    other.RotReceptivity,
                    other.RustReceptivity,
                    other.WaterSolubility);
        }

        size_t CalculateHash() const
        {
            std::size_t h = 0;
            Utils::HashCombine(h, Name);
            Utils::HashCombine(h, RenderColor);
            Utils::HashCombine(h, Strength);
            Utils::HashCombine(h, Density);
            Utils::HashCombine(h, IgnitionTemperature);
            Utils::HashCombine(h, MeltingTemperature);
            Utils::HashCombine(h, RotReceptivity);
            Utils::HashCombine(h, RustReceptivity);
            Utils::HashCombine(h, WaterSolubility);
            return h;
        }

        picojson::value Serialize() const;
        static VariantOverridesType Deserialize(picojson::object const & overridesJson);
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
    float SpringTension;
    float ElasticityCoefficient;
    float KineticFrictionCoefficient;
    float StaticFrictionCoefficient;
    float LiftCoefficient;

    std::optional<MaterialUniqueType> UniqueType;

    std::optional<MaterialSoundType> MaterialSound;

    std::optional<std::string> MaterialTextureName;
    float Opacity;

    // Water
    bool IsHull;
    float WaterIntake;
    float WaterDiffusionSpeed;
    float WaterRetention;
    float RotReceptivity; // Strength decay and browning when underwater|flooded
    float RustReceptivity; // Rust when flooded
    float WaterSolubility; // Strength decay when underwater|flooded (no rendering)

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
    bool ExplodesOnDamage;
    float WindReceptivity;
    float WaterReactivity; // When > 0, material explodes with this quantity of water threshold
    bool IsLegacyElectrical;

    // Overrides
    //
    // If set, this is a custom material; and viceversa.
    // The material itself already has these properties; this
    // member is for convenience
    std::optional<VariantOverridesType> VariantOverrides;

    // Palette
    std::optional<MaterialPaletteCoordinatesType> PaletteCoordinates;

public:

    static StructuralMaterial Create(
        MaterialColorKey const & colorKey,
        unsigned int ordinal,
        rgbColor const & baseRenderColor,
        picojson::object const & structuralMaterialJson);

    StructuralMaterial(StructuralMaterial const & other) = default;

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
        float springTension,
        float elasticityCoefficient,
        float kineticFrictionCoefficient,
        float staticFrictionCoefficient,
        float liftCoefficient,
        std::optional<MaterialUniqueType> uniqueType,
        std::optional<MaterialSoundType> materialSound,
        std::optional<std::string> materialTextureName,
        float opacity,
        // Water
        bool isHull,
        float waterIntake,
        float waterDiffusionSpeed,
        float waterRetention,
        float rotReceptivity,
        float rustReceptivity,
        float waterSolubility,
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
        bool explodesOnDamage,
        float windReceptivity,
        float waterReactivity,
        bool isLegacyElectrical,
        // Overrides
        std::optional<VariantOverridesType> variantOverrides,
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
        , SpringTension(springTension)
        , ElasticityCoefficient(elasticityCoefficient)
        , KineticFrictionCoefficient(kineticFrictionCoefficient)
        , StaticFrictionCoefficient(staticFrictionCoefficient)
        , LiftCoefficient(liftCoefficient)
        , UniqueType(uniqueType)
        , MaterialSound(materialSound)
        , MaterialTextureName(materialTextureName)
        , Opacity(opacity)
        , IsHull(isHull)
        , WaterIntake(waterIntake)
        , WaterDiffusionSpeed(waterDiffusionSpeed)
        , WaterRetention(waterRetention)
        , RotReceptivity(rotReceptivity)
        , RustReceptivity(rustReceptivity)
        , WaterSolubility(waterSolubility)
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
        , ExplodesOnDamage(explodesOnDamage)
        , WindReceptivity(windReceptivity)
        , WaterReactivity(waterReactivity)
        , IsLegacyElectrical(isLegacyElectrical)
        , VariantOverrides(variantOverrides)
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
        , SpringTension(1.0f)
        , ElasticityCoefficient(1.0f)
        , KineticFrictionCoefficient(1.0f)
        , StaticFrictionCoefficient(1.0f)
        , LiftCoefficient(0.0f)
        , UniqueType(std::nullopt)
        , MaterialSound(std::nullopt)
        , MaterialTextureName(std::nullopt)
        , Opacity(1.0f)
        , IsHull(false)
        , WaterIntake(1.0f)
        , WaterDiffusionSpeed(1.0f)
        , WaterRetention(1.0f)
        , RotReceptivity(1.0f)
        , RustReceptivity(0.0f)
        , WaterSolubility(0.0f)
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
        , ExplodesOnDamage(false)
        , WindReceptivity(1.0f)
        , WaterReactivity(0.0f)
        , IsLegacyElectrical(false)
        , VariantOverrides(std::nullopt)
        , PaletteCoordinates(std::nullopt)
    {}

    std::unique_ptr<StructuralMaterial> MakeCustomMaterial(VariantOverridesType const & overrides) const;
};

namespace std
{
    template <>
    struct hash<StructuralMaterial::VariantOverridesType>
    {
        std::size_t operator()(StructuralMaterial::VariantOverridesType const & c) const
        {
            return c.CalculateHash();
        }
    };
}

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
        ThermalSwitch,
        TimerSwitch,
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
        SteamWhistle,
        SuperWhistle,
        IndustrialHorn,
        Klaxon1,
        NuclearAlarm1,
        EvacuationAlarm1,
        EvacuationAlarm2
    };

    enum class SmokeEmitterSmokeElementType
    {
        Black,
        White
    };

    struct VariantOverridesType
    {
        std::string Name;
        float HeatGenerated;
        float Luminiscence;
        float LightSpread;
        float EnginePower;
        float WaterPumpNominalForce;
        float TimerDurationSeconds;

        VariantOverridesType(
            std::string const & name,
            float heatGenerated,
            float luminiscence,
            float lightSpread,
            float enginePower,
            float waterPumpNominalForce,
            float timerDurationSeconds)
            : Name(name)
            , HeatGenerated(heatGenerated)
            , Luminiscence(luminiscence)
            , LightSpread(lightSpread)
            , EnginePower(enginePower)
            , WaterPumpNominalForce(waterPumpNominalForce)
            , TimerDurationSeconds(timerDurationSeconds)
        {
        }

        VariantOverridesType(VariantOverridesType const & other) = default;

        bool operator==(VariantOverridesType const & other) const
        {
            return
                Name == other.Name
                && HeatGenerated == other.HeatGenerated
                && Luminiscence == other.Luminiscence
                && LightSpread == other.LightSpread
                && EnginePower == other.EnginePower
                && WaterPumpNominalForce == other.WaterPumpNominalForce
                && TimerDurationSeconds == other.TimerDurationSeconds;
        }

        bool operator<(VariantOverridesType const & other) const
        {
            return std::tie(
                Name,
                HeatGenerated,
                Luminiscence,
                LightSpread,
                EnginePower,
                WaterPumpNominalForce,
                TimerDurationSeconds) < std::tie(
                    other.Name,
                    other.HeatGenerated,
                    other.Luminiscence,
                    other.LightSpread,
                    other.EnginePower,
                    other.WaterPumpNominalForce,
                    other.TimerDurationSeconds);
        }

        size_t CalculateHash() const
        {
            std::size_t h = 0;
            Utils::HashCombine(h, Name);
            Utils::HashCombine(h, HeatGenerated);
            Utils::HashCombine(h, Luminiscence);
            Utils::HashCombine(h, LightSpread);
            Utils::HashCombine(h, EnginePower);
            Utils::HashCombine(h, WaterPumpNominalForce);
            Utils::HashCombine(h, TimerDurationSeconds);
            return h;
        }

        picojson::value Serialize() const;
        static VariantOverridesType Deserialize(picojson::object const & overridesJson);
    };

public:

    MaterialColorKey ColorKey;

    std::string Name;
    rgbColor RenderColor;

    ElectricalElementType ElectricalType;

    bool IsSelfPowered;
    bool ConductsElectricity;
    bool GateState;

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
    float ParticleLifetimeAdjustment; // Relative to use-case

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

    // Smoke emitter
    SmokeEmitterSmokeElementType SmokeEmitterSmokeType;

    // Thermal switch
    float ThermalSwitchTransitionTemperature; // K

    // Water pump
    float WaterPumpNominalForce;

    // Timer
    float TimerDurationSeconds;

    // Overrides
    //
    // If set, this is a custom material; and viceversa.
    // The material itself already has these properties; this
    // member is for convenience
    std::optional<VariantOverridesType> VariantOverrides;

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

    static SmokeEmitterSmokeElementType StrToSmokeEmitterSmokeElementType(std::string const & str);

    ElectricalMaterial(
        MaterialColorKey const & colorKey,
        std::string name,
        rgbColor const & renderColor,
        ElectricalElementType electricalType,
        bool isSelfPowered,
        bool conductsElectricity,
        bool gateState,
        float luminiscence,
        vec4f lightColor,
        float lightSpread,
        float wetFailureRate,
        float externalPressureBreakageThreshold,
        float heatGenerated,
        float minimumOperatingTemperature,
        float maximumOperatingTemperature,
        float particleEmissionRate,
        float particleLifetimeAdjustment,
        bool isInstanced,
        EngineElementType engineType,
        float engineCCWDirection,
        float enginePower,
        float engineResponsiveness,
        EngineControllerElementType engineControllerType,
        InteractiveSwitchElementType interactiveSwitchType,
        ShipSoundElementType shipSoundType,
        SmokeEmitterSmokeElementType smokeEmitterSmokeType,
        float thermalSwitchTransitionTemperature,
        float waterPumpNominalForce,
        float timerDurationSeconds,
        std::optional<VariantOverridesType> variantOverrides,
        std::optional<MaterialPaletteCoordinatesType> paletteCoordinates)
        : ColorKey(colorKey)
        , Name(name)
        , RenderColor(renderColor)
        , ElectricalType(electricalType)
        , IsSelfPowered(isSelfPowered)
        , ConductsElectricity(conductsElectricity)
        , GateState(gateState)
        , Luminiscence(luminiscence)
        , LightColor(lightColor)
        , LightSpread(lightSpread)
        , WetFailureRate(wetFailureRate)
        , ExternalPressureBreakageThreshold(externalPressureBreakageThreshold)
        , HeatGenerated(heatGenerated)
        , MinimumOperatingTemperature(minimumOperatingTemperature)
        , MaximumOperatingTemperature(maximumOperatingTemperature)
        , ParticleEmissionRate(particleEmissionRate)
        , ParticleLifetimeAdjustment(particleLifetimeAdjustment)
        //
        , IsInstanced(isInstanced)
        , EngineType(engineType)
        , EngineCCWDirection(engineCCWDirection)
        , EnginePower(enginePower)
        , EngineResponsiveness(engineResponsiveness)
        , EngineControllerType(engineControllerType)
        , InteractiveSwitchType(interactiveSwitchType)
        , ShipSoundType(shipSoundType)
        , SmokeEmitterSmokeType(smokeEmitterSmokeType)
        , ThermalSwitchTransitionTemperature(thermalSwitchTransitionTemperature)
        , WaterPumpNominalForce(waterPumpNominalForce)
        , TimerDurationSeconds(timerDurationSeconds)
        , VariantOverrides(variantOverrides)
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
        , GateState(true)
        , Luminiscence(1.0f)
        , LightColor(vec4f::zero())
        , LightSpread(1.0f)
        , WetFailureRate(0.0f)
        , ExternalPressureBreakageThreshold(100000.0f)
        , HeatGenerated(0.0f)
        , MinimumOperatingTemperature(0.0f)
        , MaximumOperatingTemperature(1000.0f)
        , ParticleEmissionRate(1.0f)
        , ParticleLifetimeAdjustment(1.0f)
        //
        , IsInstanced(isInstanced)
        , EngineType(EngineElementType::Diesel)
        , EngineCCWDirection(1.0f)
        , EnginePower(1.0f)
        , EngineResponsiveness(1.0f)
        , EngineControllerType(EngineControllerElementType::Telegraph)
        , InteractiveSwitchType(InteractiveSwitchElementType::Push)
        , ShipSoundType(ShipSoundElementType::Bell1)
        , SmokeEmitterSmokeType(SmokeEmitterSmokeElementType::White)
        , ThermalSwitchTransitionTemperature(1000.0f)
        , WaterPumpNominalForce(0.0f)
        , TimerDurationSeconds(0.0f)
        , VariantOverrides(std::nullopt)
        , PaletteCoordinates(std::nullopt)
    {
    }

    std::string MakeInstancedElementLabel(ElectricalElementInstanceIndex instanceIndex) const;

    std::unique_ptr<ElectricalMaterial> MakeCustomMaterial(VariantOverridesType const & overrides) const;
};

namespace std
{
    template <>
    struct hash<ElectricalMaterial::VariantOverridesType>
    {
        std::size_t operator()(ElectricalMaterial::VariantOverridesType const & c) const
        {
            return c.CalculateHash();
        }
    };
}
