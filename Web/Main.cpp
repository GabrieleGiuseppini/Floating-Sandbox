#include <Core/Log.h>
#include <Core/ThreadManager.h>
#include <Game/GameAssetManager.h>
#include <Game/GameController.h>
#include <Game/IGameControllerSettings.h>
#include <Game/IGameControllerSettingsOptions.h>
#include <Game/ShipLoadSpecifications.h>
#include <Render/RenderDeviceProperties.h>
#include <Simulation/ISimulationEventHandlers.h>
#include <Simulation/Materials.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr char const * CanvasSelector = "#canvas";
    constexpr char const * GameRoot = "/game";
    constexpr char const * GameExecutablePath = "/game/FloatingSandbox";
    constexpr double LowFrequencyUpdateIntervalMs = 1000.0;
    constexpr float MinToolFraction = 0.1f;
    constexpr double ToolRampMs = 5000.0;

    enum class Tool
    {
        Move,
        MoveAll,
        Grab,
        Swirl,
        Smash,
        Saw,
        Pliers,
        Repair,
        Scrub,
        HeatBlaster,
        FireExtinguisher,
        Flood,
        InjectAir,
        Pin,
        AntiMatterBomb,
        FireExtinguishingBomb,
        ImpactBomb,
        RCBomb,
        TimerBomb,
        WaveMaker,
        ScareFish,
        PhysicsProbe,
        Lightning,
        Blast,
        ElectricSpark,
        LaserCannon,
        Lamp,
        ThanosSnap,
        WindMaker,
        TerrainAdjust,
        Tornado,
        AntiGravity,
        AddHuman,
        AddFurniture,
        MoveNpc,
        RemoveNpc,
        TurnaroundNpc,
        FollowNpc
    };

    std::unordered_map<std::string, Tool> const ToolsByName = {
        {"move", Tool::Move},
        {"moveall", Tool::MoveAll},
        {"grab", Tool::Grab},
        {"swirl", Tool::Swirl},
        {"smash", Tool::Smash},
        {"saw", Tool::Saw},
        {"pliers", Tool::Pliers},
        {"repair", Tool::Repair},
        {"scrub", Tool::Scrub},
        {"heatblaster", Tool::HeatBlaster},
        {"fireextinguisher", Tool::FireExtinguisher},
        {"flood", Tool::Flood},
        {"injectair", Tool::InjectAir},
        {"pin", Tool::Pin},
        {"antimatterbomb", Tool::AntiMatterBomb},
        {"fireextinguishingbomb", Tool::FireExtinguishingBomb},
        {"impactbomb", Tool::ImpactBomb},
        {"rcbomb", Tool::RCBomb},
        {"timerbomb", Tool::TimerBomb},
        {"wavemaker", Tool::WaveMaker},
        {"scarefish", Tool::ScareFish},
        {"physicsprobe", Tool::PhysicsProbe},
        {"lightning", Tool::Lightning},
        {"blast", Tool::Blast},
        {"electricspark", Tool::ElectricSpark},
        {"lasercannon", Tool::LaserCannon},
        {"lamp", Tool::Lamp},
        {"thanossnap", Tool::ThanosSnap},
        {"windmaker", Tool::WindMaker},
        {"terrainadjust", Tool::TerrainAdjust},
        {"tornado", Tool::Tornado},
        {"antigravity", Tool::AntiGravity},
        {"addhuman", Tool::AddHuman},
        {"addfurniture", Tool::AddFurniture},
        {"movenpc", Tool::MoveNpc},
        {"removenpc", Tool::RemoveNpc},
        {"turnaroundnpc", Tool::TurnaroundNpc},
        {"follownpc", Tool::FollowNpc}
    };

    enum class NpcPhase
    {
        Idle,
        Placing,
        PlaceError,
        MovingOne,
        Selecting,
        HaveMany,
        MovingMany
    };

    bool IsNpcTool(Tool tool)
    {
        return tool >= Tool::AddHuman && tool <= Tool::FollowNpc;
    }

    struct PullState
    {
        GlobalElementId Picked;
        vec2f Current;
        vec2f Target;
        vec2f Last;
        float ConvergenceSpeed;

        PullState(GlobalElementId picked, vec2f const & position)
            : Picked(picked)
            , Current(position)
            , Target(position)
            , Last(position)
            , ConvergenceSpeed(0.03f)
        {}
    };

    struct WebApp
    {
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext = 0;
        std::unique_ptr<ThreadManager> threadManager;
        std::unique_ptr<GameAssetManager> assetManager;
        std::unique_ptr<GameController> gameController;
        int canvasCssWidth = 0;
        int canvasCssHeight = 0;
        int pixelRatio = 1;
        int engineWidth = 0;
        int engineHeight = 0;
        float mouseScaleX = 1.0f;
        float mouseScaleY = 1.0f;
        double lastLowFrequencyUpdateMs = 0.0;

        int perfMode = 0;
        float renderScale = 1.0f;
        float scaleCeiling = 1.0f;
        int qualityTier = 0;
        double lastFrameStartMs = 0.0;
        double emaIntervalMs = 16.7;
        double emaCpuMs = 4.0;
        int framesSinceChange = 0;
        int stableFrames = 0;
        bool tierSaved = false;
        unsigned int savedFishes = 0;
        unsigned int savedClouds = 0;
        unsigned int savedStars = 0;
        int savedOceanDetail = 1;
        int savedLandDetail = 1;
        int savedCloudDetail = 1;
        bool savedWake = true;
        float savedIterations = 1.0f;

        Tool tool = Tool::Move;
        bool isLeftDown = false;
        bool isPanning = false;
        bool isShiftDown = false;
        bool isPaused = false;
        int mouseX = 0;
        int mouseY = 0;
        int previousMouseX = 0;
        int previousMouseY = 0;
        double engageStartMs = 0.0;

        std::optional<GlobalConnectedComponentId> movedComponent;
        std::optional<ShipId> movedShip;
        std::optional<DisplayLogicalCoordinates> rotationCenter;
        int lastMoveDeltaX = 0;
        int lastMoveDeltaY = 0;

        std::optional<PullState> pull;
        SessionId smashSession;
        SequenceNumber repairStep;
        bool isFirstSawSegment = true;

        int blastPhase = 0;
        double blastStartSec = 0.0;
        float blastSeed = 0.0f;
        std::uint64_t sparkCounter = 0;
        DisplayLogicalCoordinates laserPrevious = DisplayLogicalCoordinates(0, 0);

        int windPhase = 0;
        double windStartSec = 0.0;
        double windMainStartSec = 0.0;
        double windTearDownStartSec = 0.0;
        DisplayLogicalCoordinates windCenter = DisplayLogicalCoordinates(0, 0);
        bool windShift = false;

        std::optional<ElementIndex> tornadoId;
        int tornadoStartY = 0;
        std::optional<ElementIndex> antiGravityId;
        std::optional<DisplayLogicalCoordinates> terrainStart;

        NpcPhase npcPhase = NpcPhase::Idle;
        std::optional<PickedNpc> npcPicked;
        std::vector<NpcId> npcIds;
        DisplayLogicalCoordinates npcStart = DisplayLogicalCoordinates(0, 0);
        DisplayLogicalCoordinates npcLast = DisplayLogicalCoordinates(0, 0);
    };

    WebApp * app = nullptr;

    void PlaySoundBase(char const * base, float volume)
    {
        EM_ASM({ if (window.FSAudio) { FSAudio.playBase(UTF8ToString($0), $1); } }, base, volume);
    }

    char const * MaterialSoundName(StructuralMaterial const & material)
    {
        if (!material.MaterialSound.has_value())
        {
            return nullptr;
        }

        switch (*material.MaterialSound)
        {
            case StructuralMaterial::MaterialSoundType::AirBubble: return "airbubble";
            case StructuralMaterial::MaterialSoundType::Cable: return "cable";
            case StructuralMaterial::MaterialSoundType::Chain: return "chain";
            case StructuralMaterial::MaterialSoundType::Cloth: return "cloth";
            case StructuralMaterial::MaterialSoundType::Gas: return "gas";
            case StructuralMaterial::MaterialSoundType::Glass: return "glass";
            case StructuralMaterial::MaterialSoundType::Human: return "human";
            case StructuralMaterial::MaterialSoundType::Lego: return "lego";
            case StructuralMaterial::MaterialSoundType::Metal: return "metal";
            case StructuralMaterial::MaterialSoundType::Piano: return "piano";
            case StructuralMaterial::MaterialSoundType::Plastic: return "plastic";
            case StructuralMaterial::MaterialSoundType::Rubber: return "rubber";
            case StructuralMaterial::MaterialSoundType::RubberBand: return "rubberband";
            case StructuralMaterial::MaterialSoundType::Wood: return "wood";
        }

        return nullptr;
    }

    void PlayMaterialSound(
        char const * kind,
        StructuralMaterial const & material,
        bool isUnderwater,
        unsigned int size,
        float volume)
    {
        char const * const name = MaterialSoundName(material);
        if (name == nullptr)
        {
            return;
        }

        EM_ASM({ if (window.FSAudio) { FSAudio.playMaterial(UTF8ToString($0), UTF8ToString($1), $2, $3, $4); } },
            kind, name, static_cast<int>(size), isUnderwater ? 1 : 0, volume);
    }

    double lastProbeMs = 0.0;

    struct WebSoundHandler final
        : public IStructuralShipEventHandler
        , public IGenericShipEventHandler
        , public ICombustionEventHandler
        , public IAtmosphereEventHandler
    {
        void OnStress(StructuralMaterial const & material, bool isUnderwater, unsigned int size) override
        {
            PlayMaterialSound("stress", material, isUnderwater, size, 0.5f);
        }

        void OnImpact(StructuralMaterial const & material, bool isUnderwater, float kineticEnergy) override
        {
            PlayMaterialSound(
                "impact",
                material,
                isUnderwater,
                static_cast<unsigned int>(std::floor(kineticEnergy / 5000.0f)),
                std::min(kineticEnergy / 4000.0f, 1.0f) * 0.8f);
        }

        void OnBreak(StructuralMaterial const & material, bool isUnderwater, unsigned int size) override
        {
            PlayMaterialSound("break", material, isUnderwater, size, 0.8f);
        }

        void OnDestroy(StructuralMaterial const & material, bool isUnderwater, unsigned int size) override
        {
            PlayMaterialSound("destroy", material, isUnderwater, size, 0.8f);
        }

        void OnSpringRepaired(StructuralMaterial const & material, bool isUnderwater, unsigned int size) override
        {
            PlayMaterialSound("repairspring", material, isUnderwater, size, 0.6f);
        }

        void OnTriangleRepaired(StructuralMaterial const & material, bool isUnderwater, unsigned int size) override
        {
            PlayMaterialSound("repairtriangle", material, isUnderwater, size, 0.6f);
        }

        void OnSawed(bool isMetal, unsigned int) override
        {
            PlaySoundBase(isMetal ? "sawed_metal" : "sawed_wood", 0.6f);
        }

        void OnPinToggled(bool isPinned, bool isUnderwater) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater(UTF8ToString($0), $1, $2); } },
                isPinned ? "pinpoint" : "unpinpoint", isUnderwater ? 1 : 0, 1.0f);
        }

        void OnGadgetPlaced(GlobalGadgetId, GadgetType gadgetType, bool isUnderwater) override
        {
            if (gadgetType == GadgetType::PhysicsProbe)
            {
                EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('physicsprobeattached', $0, 1.0); } }, isUnderwater ? 1 : 0);
            }
            else
            {
                EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('bombattached', $0, 1.0); } }, isUnderwater ? 1 : 0);
            }
        }

        void OnGadgetRemoved(GlobalGadgetId, GadgetType gadgetType, std::optional<bool> isUnderwater) override
        {
            if (!isUnderwater.has_value())
            {
                return;
            }

            if (gadgetType == GadgetType::PhysicsProbe)
            {
                EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('physicsprobedetached', $0, 1.0); } }, *isUnderwater ? 1 : 0);
            }
            else
            {
                EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('bombdetached', $0, 1.0); } }, *isUnderwater ? 1 : 0);
            }
        }

        void OnBombExplosion(GadgetType gadgetType, bool isUnderwater, unsigned int) override
        {
            char const * base = "bombexplosion";
            switch (gadgetType)
            {
                case GadgetType::AntiMatterBomb: base = "antimatterbombexplosion"; break;
                case GadgetType::FireExtinguishingBomb: base = "fireextinguishingbombexplosion"; break;
                default: break;
            }

            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater(UTF8ToString($0), $1, 1.0); } }, base, isUnderwater ? 1 : 0);
        }

        void OnRCBombPing(bool isUnderwater, unsigned int) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('rcbombping', $0, 0.7); } }, isUnderwater ? 1 : 0);
        }

        void OnAntiMatterBombContained(GlobalGadgetId, bool isContained) override
        {
            if (isContained)
            {
                PlaySoundBase("antimatterbombcontained", 0.8f);
            }
        }

        void OnAntiMatterBombPreImploding() override
        {
            PlaySoundBase("antimatterbombpreimplosion", 1.0f);
        }

        void OnAntiMatterBombImploding() override
        {
            PlaySoundBase("antimatterbombimplosion", 1.0f);
        }

        void OnPhysicsProbeReading(
            vec2f const & velocity,
            float temperature,
            float depth,
            float externalPressure,
            float internalPressure) override
        {
            double const now = emscripten_get_now();
            if (now - lastProbeMs < 100.0)
            {
                return;
            }

            lastProbeMs = now;
            EM_ASM({ if (window.FSPanel) { FSPanel.probe($0, $1, $2, $3, $4, $5); } },
                static_cast<double>(velocity.x), static_cast<double>(velocity.y), static_cast<double>(temperature),
                static_cast<double>(depth), static_cast<double>(externalPressure), static_cast<double>(internalPressure));
        }

        void OnPhysicsProbePanelOpened() override
        {
            EM_ASM({ if (window.FSPanel) { FSPanel.probeVisible(1); } });
        }

        void OnPhysicsProbePanelClosed() override
        {
            EM_ASM({ if (window.FSPanel) { FSPanel.probeVisible(0); } });
        }

        void OnWaterReactionExplosion(bool, unsigned int) override
        {
            PlaySoundBase("waterreactionexplosion", 0.8f);
        }

        void OnAirBubbleSurfaced(unsigned int) override
        {
            PlaySoundBase("airbubblessurface", 0.4f);
        }

        void OnCombustionExplosion(bool isUnderwater, unsigned int) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('combustionexplosion', $0, 1.0); } }, isUnderwater ? 1 : 0);
        }

        void OnThunder() override
        {
            PlaySoundBase("thunder", 0.9f);
        }

        void OnLightning() override
        {
            PlaySoundBase("lightning", 0.9f);
        }

        void OnLightningHit(StructuralMaterial const & material) override
        {
            char const * name = MaterialSoundName(material);
            if (name != nullptr)
            {
                std::string const base = std::string("lightninghit_") + name;
                PlaySoundBase(base.c_str(), 0.9f);
            }
        }

        void OnStormBegin() override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.loop('storm', 'rain', 1, 0.5); } });
        }

        void OnStormEnd() override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.loop('storm', 'rain', 0, 0); } });
        }
    };

    void PanelEvent(char const * op, std::string const & key, std::string const & label, int a, double b, double c)
    {
        EM_ASM({ if (window.FSPanel) { FSPanel.event(UTF8ToString($0), UTF8ToString($1), UTF8ToString($2), $3, $4, $5); } },
            op, key.c_str(), label.c_str(), a, b, c);
    }

    std::string ElementKey(GlobalElectricalElementId const & id)
    {
        return std::to_string(id.GetShipId()) + "_" + std::to_string(id.GetLocalObjectId());
    }

    struct PanelInfo
    {
        std::string Label;
        bool IsHidden;
    };

    PanelInfo MakePanelInfo(
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & material,
        std::optional<ElectricalPanel::ElementMetadata> const & metadata)
    {
        PanelInfo info{ std::string(), false };

        if (metadata.has_value())
        {
            if (metadata->Label.has_value())
            {
                info.Label = *metadata->Label;
            }

            info.IsHidden = metadata->IsHidden;
        }

        if (info.Label.empty())
        {
            info.Label = material.IsInstanced ? material.MakeInstancedElementLabel(instanceIndex) : material.Name;
        }

        return info;
    }

    char const * ShipSoundBase(ElectricalMaterial::ShipSoundElementType type)
    {
        switch (type)
        {
            case ElectricalMaterial::ShipSoundElementType::Bell1: return "shipbell1";
            case ElectricalMaterial::ShipSoundElementType::Bell2: return "shipbell2";
            case ElectricalMaterial::ShipSoundElementType::QueenMaryHorn: return "shipqueenmaryhorn";
            case ElectricalMaterial::ShipSoundElementType::FourFunnelLinerWhistle: return "shipfourfunnellinerwhistle";
            case ElectricalMaterial::ShipSoundElementType::TripodHorn: return "shiptripodhorn";
            case ElectricalMaterial::ShipSoundElementType::PipeWhistle: return "shippipewhistle";
            case ElectricalMaterial::ShipSoundElementType::LakeFreighterHorn: return "shiplakefreighterhorn";
            case ElectricalMaterial::ShipSoundElementType::ShieldhallSteamSiren: return "shipshieldhallsteamsiren";
            case ElectricalMaterial::ShipSoundElementType::QueenElizabeth2Horn: return "shipqueenelizabeth2horn";
            case ElectricalMaterial::ShipSoundElementType::SSRexWhistle: return "shipssrexwhistle";
            case ElectricalMaterial::ShipSoundElementType::SteamWhistle: return "shipsteamwhistle";
            case ElectricalMaterial::ShipSoundElementType::SuperWhistle: return "shipsuperwhistle";
            case ElectricalMaterial::ShipSoundElementType::IndustrialHorn: return "shipindustrialhorn";
            case ElectricalMaterial::ShipSoundElementType::Klaxon1: return "shipklaxon1";
            case ElectricalMaterial::ShipSoundElementType::NuclearAlarm1: return "shipnuclearalarm1";
            case ElectricalMaterial::ShipSoundElementType::EvacuationAlarm1: return "shipevacuationalarm1";
            case ElectricalMaterial::ShipSoundElementType::EvacuationAlarm2: return "shipevacuationalarm2";
        }

        return "shipbell1";
    }

    struct WebElectricalHandler final : public IElectricalElementEventHandler
    {
        std::unordered_map<std::string, std::string> engineSounds;

        void OnElectricalElementAnnouncementsBegin() override
        {
            engineSounds.clear();
            EM_ASM({ if (window.FSAudio) { FSAudio.stopGroup('eng_'); FSAudio.stopGroup('ship_'); } });
            PanelEvent("begin", "", "", 0, 0.0, 0.0);
        }

        void OnElectricalElementAnnouncementsEnd() override
        {
            PanelEvent("end", "", "", 0, 0.0, 0.0);
        }

        void OnSwitchCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            SwitchType type,
            ElectricalState state,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("switch", ElementKey(electricalElementId), info.Label, static_cast<int>(type), state == ElectricalState::On ? 1.0 : 0.0, static_cast<double>(instanceIndex));
        }

        void OnPowerProbeCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            PowerProbeType type,
            ElectricalState state,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("probe", ElementKey(electricalElementId), info.Label, static_cast<int>(type), state == ElectricalState::On ? 1.0 : 0.0, static_cast<double>(instanceIndex));
        }

        void OnEngineControllerCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("enginectl", ElementKey(electricalElementId), info.Label, static_cast<int>(electricalMaterial.EngineControllerType), 0.0, static_cast<double>(instanceIndex));
        }

        void OnEngineMonitorCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            float thrustMagnitude,
            float rpm,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            char const * sound = "enginediesel1";
            switch (electricalMaterial.EngineType)
            {
                case ElectricalMaterial::EngineElementType::Jet: sound = "enginejet1"; break;
                case ElectricalMaterial::EngineElementType::Outboard: sound = "engineoutboard1"; break;
                case ElectricalMaterial::EngineElementType::Steam: sound = "enginesteam1"; break;
                default: break;
            }

            engineSounds[ElementKey(electricalElementId)] = sound;

            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("enginemon", ElementKey(electricalElementId), info.Label, 0, static_cast<double>(thrustMagnitude), static_cast<double>(rpm));
        }

        void OnWaterPumpCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            float normalizedForce,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("pump", ElementKey(electricalElementId), info.Label, 0, static_cast<double>(normalizedForce), 0.0);
        }

        void OnWatertightDoorCreated(
            GlobalElectricalElementId electricalElementId,
            ElectricalElementInstanceIndex instanceIndex,
            bool isOpen,
            ElectricalMaterial const & electricalMaterial,
            std::optional<ElectricalPanel::ElementMetadata> const & panelElementMetadata) override
        {
            PanelInfo const info = MakePanelInfo(instanceIndex, electricalMaterial, panelElementMetadata);
            if (info.IsHidden)
            {
                return;
            }

            PanelEvent("door", ElementKey(electricalElementId), info.Label, 0, isOpen ? 1.0 : 0.0, 0.0);
        }

        void OnSwitchEnabled(GlobalElectricalElementId electricalElementId, bool isEnabled) override
        {
            PanelEvent("enabled", ElementKey(electricalElementId), "", 0, isEnabled ? 1.0 : 0.0, 0.0);
        }

        void OnSwitchToggled(GlobalElectricalElementId electricalElementId, ElectricalState newState) override
        {
            PanelEvent("state", ElementKey(electricalElementId), "", 0, newState == ElectricalState::On ? 1.0 : 0.0, 0.0);
            PlaySoundBase(newState == ElectricalState::On ? "interactiveswitchon" : "interactiveswitchoff", 0.8f);
        }

        void OnPowerProbeToggled(GlobalElectricalElementId electricalElementId, ElectricalState newState) override
        {
            PanelEvent("state", ElementKey(electricalElementId), "", 0, newState == ElectricalState::On ? 1.0 : 0.0, 0.0);
        }

        void OnEngineControllerEnabled(GlobalElectricalElementId electricalElementId, bool isEnabled) override
        {
            PanelEvent("enabled", ElementKey(electricalElementId), "", 0, isEnabled ? 1.0 : 0.0, 0.0);
        }

        void OnEngineControllerUpdated(
            GlobalElectricalElementId electricalElementId,
            ElectricalMaterial const & electricalMaterial,
            float oldControllerValue,
            float newControllerValue) override
        {
            PanelEvent("value", ElementKey(electricalElementId), "", 0, static_cast<double>(newControllerValue), 0.0);

            switch (electricalMaterial.EngineControllerType)
            {
                case ElectricalMaterial::EngineControllerElementType::JetThrust:
                {
                    PlaySoundBase(newControllerValue != 0.0f ? "interactiveswitchon" : "interactiveswitchoff", 0.8f);
                    break;
                }

                case ElectricalMaterial::EngineControllerElementType::Telegraph:
                {
                    if (oldControllerValue != newControllerValue)
                    {
                        PlaySoundBase("enginetelegraph", 0.7f);
                    }

                    break;
                }

                default:
                {
                    break;
                }
            }
        }

        void OnEngineMonitorUpdated(GlobalElectricalElementId electricalElementId, float thrustMagnitude, float rpm) override
        {
            std::string const key = ElementKey(electricalElementId);
            PanelEvent("monitor", key, "", 0, static_cast<double>(thrustMagnitude), static_cast<double>(rpm));

            auto const it = engineSounds.find(key);
            if (it != engineSounds.end())
            {
                EM_ASM({ if (window.FSAudio) { FSAudio.engine('eng_' + UTF8ToString($0), UTF8ToString($1), $2); } },
                    key.c_str(), it->second.c_str(), static_cast<double>(rpm));
            }
        }

        void OnShipSoundUpdated(
            GlobalElectricalElementId electricalElementId,
            ElectricalMaterial const & electricalMaterial,
            bool isPlaying,
            bool isUnderwater) override
        {
            std::string base = ShipSoundBase(electricalMaterial.ShipSoundType);
            if (isUnderwater)
            {
                base += "_underwater";
            }

            std::string const key = "ship_" + ElementKey(electricalElementId);
            EM_ASM({ if (window.FSAudio) { FSAudio.shipSound(UTF8ToString($0), UTF8ToString($1), $2); } },
                key.c_str(), base.c_str(), isPlaying ? 1 : 0);
        }

        void OnWaterPumpEnabled(GlobalElectricalElementId electricalElementId, bool isEnabled) override
        {
            PanelEvent("enabled", ElementKey(electricalElementId), "", 0, isEnabled ? 1.0 : 0.0, 0.0);
        }

        void OnWaterPumpUpdated(GlobalElectricalElementId electricalElementId, float normalizedForce) override
        {
            PanelEvent("value", ElementKey(electricalElementId), "", 0, static_cast<double>(normalizedForce), 0.0);
        }

        void OnWatertightDoorEnabled(GlobalElectricalElementId electricalElementId, bool isEnabled) override
        {
            PanelEvent("enabled", ElementKey(electricalElementId), "", 0, isEnabled ? 1.0 : 0.0, 0.0);
        }

        void OnWatertightDoorUpdated(GlobalElectricalElementId electricalElementId, bool isOpen) override
        {
            PanelEvent("state", ElementKey(electricalElementId), "", 0, isOpen ? 1.0 : 0.0, 0.0);
            PlaySoundBase(isOpen ? "watertightdooropened" : "watertightdoorclosed", 0.8f);
        }

        void OnLampBroken(bool isUnderwater, unsigned int size) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playMaterial('break', 'glass', $0, $1, 0.8); } }, static_cast<int>(size), isUnderwater ? 1 : 0);
        }

        void OnLampExploded(bool isUnderwater, unsigned int) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('lampexplosion', $0, 1.0); } }, isUnderwater ? 1 : 0);
        }

        void OnLampImploded(bool isUnderwater, unsigned int) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater('lampimplosion', $0, 1.0); } }, isUnderwater ? 1 : 0);
        }

        void OnLightFlicker(DurationShortLongType duration, bool isUnderwater, unsigned int) override
        {
            EM_ASM({ if (window.FSAudio) { FSAudio.playUnderwater($2 ? 'lightflicker_long' : 'lightflicker_short', $0, 0.7); } },
                isUnderwater ? 1 : 0, 0, duration == DurationShortLongType::Long ? 1 : 0);
        }
    };

    WebElectricalHandler electricalHandler;

    WebSoundHandler soundHandler;

    char const * ToolLoopSound(Tool tool, bool isShiftDown)
    {
        switch (tool)
        {
            case Tool::Grab: return "draw";
            case Tool::Swirl: return "swirl";
            case Tool::Smash: return "blasttoolslow1";
            case Tool::Saw: return "saw";
            case Tool::Scrub: return "scrub";
            case Tool::HeatBlaster: return isShiftDown ? "heatblastercool" : "heatblasterheat";
            case Tool::FireExtinguisher: return "fireextinguisher";
            case Tool::Flood: return "floodhose";
            case Tool::InjectAir: return "pressureinjection";
            case Tool::WaveMaker: return "wavemaker";
            case Tool::ScareFish: return "fishscream";
            case Tool::ElectricSpark: return "electricspark";
            case Tool::LaserCannon: return isShiftDown ? "laserrayamplified" : "laserraynormal";
            case Tool::Tornado: return "tornado";
            default: return nullptr;
        }
    }

    void SetToolLoop(bool isOn)
    {
        char const * const base = ToolLoopSound(app->tool, app->isShiftDown);
        if (base == nullptr)
        {
            return;
        }

        EM_ASM({ if (window.FSAudio) { FSAudio.loop('tool', UTF8ToString($0), $1, 0.7); } }, base, isOn ? 1 : 0);
    }

    void ReportProgress(float progress)
    {
        EM_ASM({ if (Module.onGameProgress) { Module.onGameProgress($0); } }, progress);
    }

    void ReportError(std::string const & message)
    {
        EM_ASM({ if (Module.onGameError) { Module.onGameError(UTF8ToString($0)); } }, message.c_str());
    }

    void ReportReady()
    {
        EM_ASM({ if (Module.onGameReady) { Module.onGameReady(); } });
    }

    void ReadCanvasSize(int & cssWidth, int & cssHeight, int & pixelRatio)
    {
        double width = 0.0;
        double height = 0.0;
        emscripten_get_element_css_size(CanvasSelector, &width, &height);

        cssWidth = std::max(1, static_cast<int>(std::lround(width)));
        cssHeight = std::max(1, static_cast<int>(std::lround(height)));
        pixelRatio = std::max(1, static_cast<int>(std::lround(emscripten_get_device_pixel_ratio())));
    }

    void ComputeEngineSize(int cssWidth, int cssHeight, int pixelRatio, float scale, int & width, int & height)
    {
        width = std::max(64, static_cast<int>(std::lround(static_cast<double>(cssWidth) * pixelRatio * scale)));
        height = std::max(64, static_cast<int>(std::lround(static_cast<double>(cssHeight) * pixelRatio * scale)));
    }

    void ApplyCanvasSize(int cssWidth, int cssHeight, int pixelRatio)
    {
        int width = 0;
        int height = 0;
        ComputeEngineSize(cssWidth, cssHeight, pixelRatio, app->renderScale, width, height);
        emscripten_set_canvas_element_size(CanvasSelector, width, height);
        app->engineWidth = width;
        app->engineHeight = height;
        app->mouseScaleX = static_cast<float>(width) / static_cast<float>(std::max(1, cssWidth));
        app->mouseScaleY = static_cast<float>(height) / static_cast<float>(std::max(1, cssHeight));
    }

    void ApplyQualityTier(int tier)
    {
        IGameControllerSettings & settings = *app->gameController;

        if (!app->tierSaved)
        {
            app->savedFishes = settings.GetNumberOfFishes();
            app->savedClouds = settings.GetNumberOfClouds();
            app->savedStars = settings.GetNumberOfStars();
            app->savedOceanDetail = static_cast<int>(settings.GetOceanRenderDetail());
            app->savedLandDetail = static_cast<int>(settings.GetLandRenderDetail());
            app->savedCloudDetail = static_cast<int>(settings.GetCloudRenderDetail());
            app->savedWake = settings.GetDoGenerateEngineWakeParticles();
            app->savedIterations = settings.GetNumMechanicalDynamicsIterationsAdjustment();
            app->tierSaved = true;
        }

        if (tier <= 0)
        {
            settings.SetNumberOfFishes(app->savedFishes);
            settings.SetNumberOfClouds(app->savedClouds);
            settings.SetNumberOfStars(app->savedStars);
            settings.SetOceanRenderDetail(static_cast<OceanRenderDetailType>(app->savedOceanDetail));
            settings.SetLandRenderDetail(static_cast<LandRenderDetailType>(app->savedLandDetail));
            settings.SetCloudRenderDetail(static_cast<CloudRenderDetailType>(app->savedCloudDetail));
            settings.SetDoGenerateEngineWakeParticles(app->savedWake);
            settings.SetNumMechanicalDynamicsIterationsAdjustment(app->savedIterations);
        }
        else if (tier == 1)
        {
            settings.SetNumberOfFishes(std::min(app->savedFishes, 20u));
            settings.SetNumberOfClouds(std::min(app->savedClouds, 8u));
            settings.SetNumberOfStars(std::min(app->savedStars, 500u));
            settings.SetOceanRenderDetail(OceanRenderDetailType::Basic);
            settings.SetLandRenderDetail(LandRenderDetailType::Basic);
            settings.SetCloudRenderDetail(CloudRenderDetailType::Basic);
            settings.SetDoGenerateEngineWakeParticles(app->savedWake);
            settings.SetNumMechanicalDynamicsIterationsAdjustment(app->savedIterations);
        }
        else
        {
            settings.SetNumberOfFishes(0);
            settings.SetNumberOfClouds(std::min(app->savedClouds, 4u));
            settings.SetNumberOfStars(std::min(app->savedStars, 200u));
            settings.SetOceanRenderDetail(OceanRenderDetailType::Basic);
            settings.SetLandRenderDetail(LandRenderDetailType::Basic);
            settings.SetCloudRenderDetail(CloudRenderDetailType::Basic);
            settings.SetDoGenerateEngineWakeParticles(false);
            settings.SetNumMechanicalDynamicsIterationsAdjustment(std::min(app->savedIterations, 0.75f));
        }

        app->qualityTier = tier;
    }

    void ApplyRenderScale(float scale)
    {
        scale = std::clamp(scale, 0.4f, 1.0f);
        if (std::abs(scale - app->renderScale) < 0.001f)
        {
            return;
        }

        app->renderScale = scale;
        ApplyCanvasSize(app->canvasCssWidth, app->canvasCssHeight, app->pixelRatio);
        app->gameController->SetCanvasSize(DisplayLogicalSize(app->engineWidth, app->engineHeight));
    }

    void SetPerfMode(int mode)
    {
        app->perfMode = mode;
        app->framesSinceChange = 0;
        app->stableFrames = 0;

        switch (mode)
        {
            case 1:
            {
                app->scaleCeiling = 1.0f;
                ApplyQualityTier(0);
                ApplyRenderScale(1.0f);
                break;
            }

            case 2:
            {
                app->scaleCeiling = 0.75f;
                ApplyQualityTier(1);
                ApplyRenderScale(0.75f);
                break;
            }

            case 3:
            {
                app->scaleCeiling = 0.5f;
                ApplyQualityTier(2);
                ApplyRenderScale(0.5f);
                break;
            }

            default:
            {
                app->scaleCeiling = 1.0f;
                ApplyQualityTier(0);
                ApplyRenderScale(1.0f);
                break;
            }
        }
    }

    void AdaptPerformance(double frameStartMs, double cpuMs)
    {
        if (app->lastFrameStartMs > 0.0)
        {
            double const interval = std::min(frameStartMs - app->lastFrameStartMs, 250.0);
            app->emaIntervalMs += (interval - app->emaIntervalMs) * 0.08;
            app->emaCpuMs += (cpuMs - app->emaCpuMs) * 0.08;
        }

        app->lastFrameStartMs = frameStartMs;

        if (app->perfMode != 0)
        {
            return;
        }

        ++app->framesSinceChange;
        if (app->framesSinceChange < 45)
        {
            return;
        }

        bool const slow = app->emaIntervalMs > 21.5;
        bool const cpuBound = app->emaCpuMs > app->emaIntervalMs * 0.6;

        if (slow)
        {
            app->stableFrames = 0;

            if (!cpuBound && app->renderScale > 0.5f)
            {
                ApplyRenderScale(app->renderScale - 0.125f);
                app->framesSinceChange = 0;
            }
            else if (app->qualityTier < 2)
            {
                ApplyQualityTier(app->qualityTier + 1);
                app->framesSinceChange = 0;
            }
            else if (app->renderScale > 0.5f)
            {
                ApplyRenderScale(app->renderScale - 0.125f);
                app->framesSinceChange = 0;
            }
        }
        else if (app->emaIntervalMs < 17.5)
        {
            if (++app->stableFrames > 900)
            {
                app->stableFrames = 0;

                if (app->renderScale < 0.999f && app->emaCpuMs < app->emaIntervalMs * 0.5)
                {
                    ApplyRenderScale(app->renderScale + 0.125f);
                    app->framesSinceChange = 0;
                }
            }
        }
        else
        {
            app->stableFrames = 0;
        }
    }

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE CreateGlContext()
    {
        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.majorVersion = 2;
        attributes.minorVersion = 0;
        attributes.alpha = false;
        attributes.depth = true;
        attributes.stencil = false;
        attributes.antialias = false;
        attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
        attributes.enableExtensionsByDefault = true;

        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context(CanvasSelector, &attributes);
        if (context <= 0)
        {
            return 0;
        }

        emscripten_webgl_make_context_current(context);

        emscripten_webgl_enable_extension(context, "EXT_color_buffer_float");
        emscripten_webgl_enable_extension(context, "OES_texture_float_linear");
        emscripten_webgl_enable_extension(context, "EXT_float_blend");

        return context;
    }

    float SmoothStep(float edge0, float edge1, float x)
    {
        float const t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    DisplayLogicalCoordinates MousePosition()
    {
        return DisplayLogicalCoordinates(app->mouseX, app->mouseY);
    }

    float RampedFraction(double nowMs)
    {
        double const elapsed = std::max(0.0, nowMs - app->engageStartMs);
        return MinToolFraction + (1.0f - MinToolFraction) * static_cast<float>(std::min(1.0, elapsed / ToolRampMs));
    }


    void ResetNpcState()
    {
        IGameController & controller = *app->gameController;

        if (app->npcPicked.has_value())
        {
            if (app->npcPhase == NpcPhase::Placing)
            {
                controller.AbortNewNpc(app->npcPicked->Id);
            }
            else if (app->npcPhase == NpcPhase::MovingOne)
            {
                controller.EndMoveNpc(app->npcPicked->Id);
            }
        }

        if (app->npcPhase == NpcPhase::MovingMany)
        {
            for (NpcId const id : app->npcIds)
            {
                controller.EndMoveNpc(id);
            }
        }

        app->npcPicked.reset();
        app->npcIds.clear();
        app->npcPhase = NpcPhase::Idle;
    }

    void BeginNpcLeft()
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        bool const shift = app->isShiftDown;

        switch (app->tool)
        {
            case Tool::AddHuman:
            case Tool::AddFurniture:
            {
                std::optional<PickedNpc> const picked = app->tool == Tool::AddHuman
                    ? controller.BeginPlaceNewHumanNpc(std::nullopt, position, shift)
                    : controller.BeginPlaceNewFurnitureNpc(std::nullopt, position, shift);

                if (picked.has_value())
                {
                    app->npcPicked = picked;
                    app->npcPhase = NpcPhase::Placing;
                }
                else
                {
                    app->npcPhase = NpcPhase::PlaceError;
                    PlaySoundBase("error", 0.8f);
                }

                break;
            }

            case Tool::MoveNpc:
            {
                if (app->npcPhase == NpcPhase::HaveMany && !app->npcIds.empty())
                {
                    controller.BeginMoveNpcs(app->npcIds);
                    app->npcLast = position;
                    app->npcPhase = NpcPhase::MovingMany;
                    break;
                }

                std::optional<PickedNpc> probe;
                if (!shift)
                {
                    probe = controller.ProbeNpcAt(position);
                }

                if (probe.has_value())
                {
                    controller.BeginMoveNpc(probe->Id, probe->ParticleOrdinal, shift);
                    app->npcPicked = probe;
                    app->npcPhase = NpcPhase::MovingOne;
                }
                else
                {
                    app->npcStart = position;
                    app->npcPhase = NpcPhase::Selecting;
                }

                break;
            }

            case Tool::RemoveNpc:
            case Tool::TurnaroundNpc:
            {
                std::optional<PickedNpc> probe;
                if (!shift)
                {
                    probe = controller.ProbeNpcAt(position);
                }

                if (probe.has_value())
                {
                    if (app->tool == Tool::RemoveNpc)
                    {
                        controller.RemoveNpc(probe->Id);
                    }
                    else
                    {
                        controller.TurnaroundNpc(probe->Id);
                    }
                }
                else
                {
                    app->npcStart = position;
                    app->npcPhase = NpcPhase::Selecting;
                }

                break;
            }

            case Tool::FollowNpc:
            {
                std::optional<PickedNpc> const probe = controller.ProbeNpcAt(position);
                if (probe.has_value())
                {
                    controller.SelectNpc(probe->Id);
                }
                else
                {
                    controller.SelectNpc(std::nullopt);
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    void EndNpcLeft()
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();

        switch (app->npcPhase)
        {
            case NpcPhase::Placing:
            {
                controller.CompleteNewNpc(app->npcPicked->Id);
                app->npcPicked.reset();
                app->npcPhase = NpcPhase::Idle;
                break;
            }

            case NpcPhase::PlaceError:
            {
                app->npcPhase = NpcPhase::Idle;
                break;
            }

            case NpcPhase::MovingOne:
            {
                controller.EndMoveNpc(app->npcPicked->Id);
                app->npcPicked.reset();
                app->npcPhase = NpcPhase::Idle;
                break;
            }

            case NpcPhase::Selecting:
            {
                if (app->tool == Tool::MoveNpc)
                {
                    app->npcIds = controller.ProbeNpcsInRect(app->npcStart, position);
                    app->npcPhase = app->npcIds.empty() ? NpcPhase::Idle : NpcPhase::HaveMany;
                }
                else
                {
                    if (app->tool == Tool::RemoveNpc)
                    {
                        controller.RemoveNpcsInRect(app->npcStart, position);
                    }
                    else
                    {
                        controller.TurnaroundNpcsInRect(app->npcStart, position);
                    }

                    app->npcPhase = NpcPhase::Idle;
                }

                break;
            }

            case NpcPhase::MovingMany:
            {
                for (NpcId const id : app->npcIds)
                {
                    controller.EndMoveNpc(id);
                }

                app->npcPhase = NpcPhase::HaveMany;
                break;
            }

            case NpcPhase::HaveMany:
            case NpcPhase::Idle:
            {
                break;
            }
        }
    }

    void UpdateNpcTool()
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        bool const shift = app->isShiftDown;

        switch (app->npcPhase)
        {
            case NpcPhase::Placing:
            case NpcPhase::MovingOne:
            {
                controller.MoveNpcTo(app->npcPicked->Id, position, app->npcPicked->WorldOffset, shift);
                break;
            }

            case NpcPhase::Selecting:
            {
                controller.ShowInteractiveToolDashedRect(app->npcStart, position);
                controller.HighlightNpcsInRect(app->npcStart, position);
                break;
            }

            case NpcPhase::HaveMany:
            {
                controller.HighlightNpcs(app->npcIds);
                break;
            }

            case NpcPhase::MovingMany:
            {
                int const dx = position.x - app->npcLast.x;
                int const dy = position.y - app->npcLast.y;
                if (dx != 0 || dy != 0)
                {
                    controller.MoveNpcsBy(app->npcIds, DisplayLogicalSize(dx, dy));
                }

                app->npcLast = position;
                break;
            }

            case NpcPhase::PlaceError:
            {
                break;
            }

            case NpcPhase::Idle:
            {
                bool const isHoverTool = app->tool == Tool::MoveNpc || app->tool == Tool::RemoveNpc || app->tool == Tool::TurnaroundNpc;
                if (isHoverTool && !shift && controller.HasNpcs())
                {
                    std::optional<PickedNpc> const probe = controller.ProbeNpcAt(position);
                    if (probe.has_value())
                    {
                        controller.HighlightNpcs({ probe->Id });
                    }
                }

                break;
            }
        }
    }

    void UpdateBlast(double nowSec)
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        float const elapsed = static_cast<float>(nowSec - app->blastStartSec);

        switch (app->blastPhase)
        {
            case 1:
            {
                float const progress = std::min(elapsed / 0.75f, 1.0f);
                controller.ApplyBlastAt(position, progress, 1.0f, progress, app->blastSeed);
                if (progress >= 1.0f)
                {
                    app->blastPhase = 2;
                    app->blastStartSec = nowSec;
                }

                break;
            }

            case 2:
            {
                if (elapsed >= 0.5f)
                {
                    app->blastPhase = 3;
                    app->blastStartSec = nowSec;
                    PlaySoundBase("blasttoolslow2", 0.9f);
                }

                break;
            }

            case 3:
            {
                float const progress = std::min(elapsed / 0.25f, 1.0f);
                controller.ApplyBlastAt(position, 1.0f + progress * 2.5f, 2.5f, progress, app->blastSeed);
                if (progress >= 1.0f)
                {
                    app->blastPhase = 0;
                }

                break;
            }

            case 4:
            {
                float const progress = std::min(elapsed / 0.25f, 1.0f);
                controller.ApplyBlastAt(position, progress * 2.0f, 2.0f, progress, app->blastSeed);
                if (progress >= 1.0f)
                {
                    app->blastPhase = 0;
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    void UpdateWind(double nowSec)
    {
        if (app->windPhase == 0)
        {
            return;
        }

        IGameController & controller = *app->gameController;
        bool const shift = app->windShift;

        if (app->windPhase != 3 && !app->isLeftDown)
        {
            app->windPhase = 3;
            app->windTearDownStartSec = nowSec;
            app->windCenter = MousePosition();
        }

        float const preElapsed = static_cast<float>(std::max(nowSec - app->windStartSec, 0.0));
        float intensity = 0.0f;

        if (app->windPhase == 1)
        {
            app->windCenter = MousePosition();
            float const duration = shift ? 0.05f : 1.5f;
            if (preElapsed >= duration)
            {
                intensity = 1.0f;
                app->windPhase = 2;
                app->windMainStartSec = nowSec;
            }
            else
            {
                intensity = preElapsed / duration;
            }
        }
        else if (app->windPhase == 2)
        {
            app->windCenter = MousePosition();
            intensity = 1.0f;
        }
        else
        {
            float const elapsed = static_cast<float>(nowSec - app->windTearDownStartSec);
            if (elapsed >= 2.0f)
            {
                app->windPhase = 0;
                EM_ASM({ if (window.FSAudio) { FSAudio.loop('wind', '', 0, 0); } });
                return;
            }

            intensity = 1.0f - elapsed / 2.0f;
        }

        float const mainElapsed = app->windMainStartSec > 0.0 ? static_cast<float>(nowSec - app->windMainStartSec) : 0.0f;
        controller.ApplyRadialWindFrom(
            app->windCenter,
            preElapsed,
            intensity * (shift ? 3.0f : 1.0f),
            mainElapsed,
            intensity * (shift ? 1.5f : 1.0f));
    }

    void ResetToolSpecificState()
    {
        IGameController & controller = *app->gameController;

        if (app->tornadoId.has_value())
        {
            controller.EndPlaceTornado(*app->tornadoId);
            app->tornadoId.reset();
        }

        if (app->antiGravityId.has_value())
        {
            controller.AbortPlaceAntiGravityField(*app->antiGravityId);
            app->antiGravityId.reset();
        }

        app->terrainStart.reset();
        app->blastPhase = 0;
        controller.ResetLamp();
        ResetNpcState();
    }

    void ReleasePull()
    {
        app->pull.reset();
    }

    void EndMove()
    {
        IGameController & controller = *app->gameController;

        if (app->movedComponent.has_value())
        {
            controller.MoveBy(
                *app->movedComponent,
                DisplayLogicalSize(0, 0),
                DisplayLogicalSize(app->lastMoveDeltaX, app->lastMoveDeltaY));
            controller.SetMoveToolEngaged(false);
        }
        else if (app->movedShip.has_value())
        {
            controller.MoveBy(
                *app->movedShip,
                DisplayLogicalSize(0, 0),
                DisplayLogicalSize(app->lastMoveDeltaX, app->lastMoveDeltaY));
            controller.SetMoveToolEngaged(false);
        }

        app->movedComponent.reset();
        app->movedShip.reset();
        app->rotationCenter.reset();
        app->lastMoveDeltaX = 0;
        app->lastMoveDeltaY = 0;
    }

    void BeginLeft(double nowMs)
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();

        app->isLeftDown = true;
        app->engageStartMs = nowMs;
        app->smashSession = SessionId::New();
        ++app->repairStep;
        ++app->repairStep;
        app->isFirstSawSegment = true;

        SetToolLoop(true);

        if (app->tool == Tool::Pliers)
        {
            PlaySoundBase("pliers", 0.8f);
        }

        if (IsNpcTool(app->tool))
        {
            BeginNpcLeft();
            return;
        }

        switch (app->tool)
        {
            case Tool::Move:
            {
                controller.PickObjectToMove(position, app->movedComponent);
                if (app->movedComponent.has_value())
                {
                    controller.SetMoveToolEngaged(true);
                }

                break;
            }

            case Tool::MoveAll:
            {
                controller.PickObjectToMove(position, app->movedShip);
                if (app->movedShip.has_value())
                {
                    controller.SetMoveToolEngaged(true);
                }

                break;
            }

            case Tool::Pin:
            {
                if (app->isShiftDown)
                {
                    controller.RemoveAllPins();
                }
                else
                {
                    controller.TogglePinAt(position);
                }

                break;
            }

            case Tool::AntiMatterBomb:
            {
                controller.ToggleAntiMatterBombAt(position);
                break;
            }

            case Tool::FireExtinguishingBomb:
            {
                controller.ToggleFireExtinguishingBombAt(position);
                break;
            }

            case Tool::ImpactBomb:
            {
                controller.ToggleImpactBombAt(position);
                break;
            }

            case Tool::RCBomb:
            {
                controller.ToggleRCBombAt(position);
                break;
            }

            case Tool::TimerBomb:
            {
                controller.ToggleTimerBombAt(position);
                break;
            }

            case Tool::PhysicsProbe:
            {
                controller.TogglePhysicsProbeAt(position);
                break;
            }

            case Tool::Lightning:
            {
                controller.TriggerInteractiveLightningAt(position);
                break;
            }

            case Tool::Blast:
            {
                app->blastPhase = app->isShiftDown ? 4 : 1;
                app->blastStartSec = nowMs / 1000.0;
                app->blastSeed = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
                PlaySoundBase(app->isShiftDown ? "blasttoolfast" : "blasttoolslow1", 0.9f);
                break;
            }

            case Tool::ElectricSpark:
            {
                app->sparkCounter = 0;
                break;
            }

            case Tool::LaserCannon:
            {
                app->laserPrevious = position;
                break;
            }

            case Tool::Lamp:
            {
                controller.SetLampAt(position, app->isShiftDown ? 1.0f / 2.5f : 1.0f / 6.0f);
                break;
            }

            case Tool::ThanosSnap:
            {
                controller.ApplyThanosSnapAt(position, app->isShiftDown);
                PlaySoundBase("thanossnap", 0.9f);
                break;
            }

            case Tool::WindMaker:
            {
                app->windPhase = 1;
                app->windStartSec = nowMs / 1000.0;
                app->windMainStartSec = 0.0;
                app->windShift = app->isShiftDown;
                app->windCenter = position;
                EM_ASM({ if (window.FSAudio) { FSAudio.loop('wind', 'wind', 1, 0.6); } });
                if (app->isShiftDown)
                {
                    PlaySoundBase("windgustshort", 0.8f);
                }

                break;
            }

            case Tool::TerrainAdjust:
            {
                app->terrainStart = position;
                break;
            }

            case Tool::Tornado:
            {
                app->tornadoId = controller.BeginPlaceTornado(position.x);
                app->tornadoStartY = position.y;
                break;
            }

            case Tool::AntiGravity:
            {
                app->antiGravityId = controller.BeginPlaceAntiGravityField(position);
                break;
            }

            default:
            {
                break;
            }
        }
    }


    void EndToolSpecific()
    {
        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();

        switch (app->tool)
        {
            case Tool::Lamp:
            {
                controller.ResetLamp();
                break;
            }

            case Tool::Tornado:
            {
                if (app->tornadoId.has_value())
                {
                    controller.EndPlaceTornado(*app->tornadoId);
                    app->tornadoId.reset();
                }

                break;
            }

            case Tool::AntiGravity:
            {
                if (app->antiGravityId.has_value())
                {
                    controller.EndPlaceAntiGravityField(*app->antiGravityId, position, app->isShiftDown ? 10.0f : 1.0f);
                    app->antiGravityId.reset();
                }

                break;
            }

            case Tool::TerrainAdjust:
            {
                if (app->terrainStart.has_value() && app->isShiftDown)
                {
                    controller.AdjustOceanFloorTo(
                        controller.ScreenToWorld(*app->terrainStart),
                        controller.ScreenToWorld(position));
                }

                app->terrainStart.reset();
                break;
            }

            default:
            {
                break;
            }
        }
    }

    void EndLeft()
    {
        if (!app->isLeftDown)
        {
            return;
        }

        app->isLeftDown = false;

        SetToolLoop(false);
        EM_ASM({ if (window.FSAudio) { FSAudio.loop('tool', '', 0, 0); } });

        if (IsNpcTool(app->tool))
        {
            EndNpcLeft();
            return;
        }

        EndToolSpecific();

        EndMove();
        ReleasePull();
    }

    void UpdateTool(double nowMs)
    {
        if (IsNpcTool(app->tool))
        {
            if (!app->isPanning)
            {
                UpdateNpcTool();
            }

            return;
        }

        UpdateWind(nowMs / 1000.0);

        if (!app->isLeftDown || app->isPanning)
        {
            return;
        }

        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        bool const shift = app->isShiftDown;

        switch (app->tool)
        {
            case Tool::Blast:
            {
                UpdateBlast(nowMs / 1000.0);
                break;
            }

            case Tool::ElectricSpark:
            {
                controller.ApplyElectricSparkAt(position, ++app->sparkCounter, shift ? 3.2f : 1.0f);
                break;
            }

            case Tool::LaserCannon:
            {
                controller.ApplyLaserCannonThrough(app->laserPrevious, position, shift ? 2.0f : 1.0f);
                app->laserPrevious = position;
                break;
            }

            case Tool::Lamp:
            {
                controller.SetLampAt(position, shift ? 1.0f / 2.5f : 1.0f / 6.0f);
                break;
            }

            case Tool::Tornado:
            {
                if (app->tornadoId.has_value())
                {
                    float strength = 1.0f;
                    float heat = 0.0f;
                    int const deltaY = position.y - app->tornadoStartY;
                    if (deltaY < -20)
                    {
                        strength = 1.0f + std::min(static_cast<float>(-deltaY - 20) / 150.0f, 1.0f);
                    }
                    else if (deltaY > 20)
                    {
                        heat = std::min(static_cast<float>(deltaY - 20) / 150.0f, 1.0f);
                    }

                    controller.UpdateTornado(*app->tornadoId, position.x, strength, heat);
                }

                break;
            }

            case Tool::AntiGravity:
            {
                if (app->antiGravityId.has_value())
                {
                    controller.UpdatePlaceAntiGravityField(*app->antiGravityId, position);
                }

                if (shift)
                {
                    controller.BoostAntiGravityFields(10.0f);
                }

                break;
            }

            case Tool::Grab:
            {
                float const fraction = RampedFraction(nowMs);
                controller.DrawTo(position, shift ? -fraction : fraction);
                break;
            }

            case Tool::Swirl:
            {
                float const fraction = RampedFraction(nowMs);
                controller.SwirlAt(position, shift ? -fraction : fraction);
                break;
            }

            case Tool::Smash:
            {
                controller.DestroyAt(position, RampedFraction(nowMs) * 10.0f, app->smashSession);
                break;
            }

            case Tool::Repair:
            {
                ++app->repairStep;
                controller.RepairAt(position, 1.0f, app->repairStep);
                break;
            }

            case Tool::HeatBlaster:
            {
                controller.ApplyHeatBlasterAt(
                    position,
                    shift ? HeatBlasterActionType::Cool : HeatBlasterActionType::Heat);
                break;
            }

            case Tool::FireExtinguisher:
            {
                controller.ExtinguishFireAt(position, shift ? 4.0f : 1.0f);
                break;
            }

            case Tool::Flood:
            {
                controller.FloodAt(position, shift ? -1.0f : 1.0f);
                break;
            }

            case Tool::InjectAir:
            {
                controller.InjectAirAt(position, shift ? -1.0f : 1.0f);
                break;
            }

            case Tool::WaveMaker:
            {
                controller.ApplyInteractiveWaveAt(position, 0);
                break;
            }

            case Tool::ScareFish:
            {
                if (shift)
                {
                    controller.AttractFish(position, 50.0f, std::chrono::milliseconds(0));
                }
                else
                {
                    controller.ScareFish(position, 40.0f, std::chrono::milliseconds(0));
                }

                break;
            }

            case Tool::Pliers:
            {
                vec2f const mouse = position.ToFloat();

                if (!app->pull.has_value())
                {
                    std::optional<GlobalElementId> const picked = controller.PickObjectForPickAndPull(position);
                    if (picked.has_value())
                    {
                        app->pull.emplace(*picked, mouse);
                    }
                }
                else
                {
                    PullState & pull = *app->pull;

                    pull.Target = mouse;

                    vec2f const stride = mouse - pull.Last;
                    float const worldStride = controller.ScreenOffsetToWorldOffset(DisplayLogicalSize::FromFloatRound(stride)).length();

                    float const minRate = 0.03f;
                    float const maxRate = 0.09f;
                    float const newSpeed = minRate + (maxRate - minRate) * SmoothStep(2.0f, 50.0f, worldStride);
                    float const alpha = SmoothStep(0.0f, 3.0f, stride.length());
                    pull.ConvergenceSpeed = pull.ConvergenceSpeed + (newSpeed - pull.ConvergenceSpeed) * alpha;

                    pull.Last = mouse;
                    pull.Current = pull.Current + (pull.Target - pull.Current) * pull.ConvergenceSpeed;

                    controller.Pull(pull.Picked, DisplayLogicalCoordinates::FromFloatRound(pull.Current));
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    void OnToolMouseMove(int deltaX, int deltaY)
    {
        if (!app->isLeftDown)
        {
            return;
        }

        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        DisplayLogicalCoordinates const previous(app->previousMouseX, app->previousMouseY);

        switch (app->tool)
        {
            case Tool::Move:
            case Tool::MoveAll:
            {
                if (!app->movedComponent.has_value() && !app->movedShip.has_value())
                {
                    break;
                }

                if (app->isShiftDown)
                {
                    if (!app->rotationCenter.has_value())
                    {
                        app->rotationCenter = previous;
                    }

                    float const deltaYFloat = static_cast<float>(deltaY);

                    if (app->movedComponent.has_value())
                    {
                        controller.RotateBy(*app->movedComponent, deltaYFloat, *app->rotationCenter, deltaYFloat);
                    }
                    else
                    {
                        controller.RotateBy(*app->movedShip, deltaYFloat, *app->rotationCenter, deltaYFloat);
                    }
                }
                else
                {
                    app->rotationCenter.reset();

                    DisplayLogicalSize const offset(deltaX, deltaY);

                    if (app->movedComponent.has_value())
                    {
                        controller.MoveBy(*app->movedComponent, offset, offset);
                    }
                    else
                    {
                        controller.MoveBy(*app->movedShip, offset, offset);
                    }

                    app->lastMoveDeltaX = deltaX;
                    app->lastMoveDeltaY = deltaY;
                }

                break;
            }

            case Tool::Saw:
            {
                controller.SawThrough(previous, position, app->isFirstSawSegment);
                app->isFirstSawSegment = false;
                break;
            }

            case Tool::Scrub:
            {
                if (app->isShiftDown)
                {
                    controller.RustThrough(previous, position);
                }
                else
                {
                    controller.ScrubThrough(previous, position);
                }

                break;
            }

            case Tool::Lamp:
            {
                controller.SetLampAt(position, app->isShiftDown ? 1.0f / 2.5f : 1.0f / 6.0f);
                break;
            }

            case Tool::TerrainAdjust:
            {
                if (!app->isShiftDown)
                {
                    controller.AdjustOceanFloorTo(controller.ScreenToWorld(previous), controller.ScreenToWorld(position));
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    void RunFrame()
    {
        if (app == nullptr || !app->gameController)
        {
            return;
        }

        try
        {
            int cssWidth = 0;
            int cssHeight = 0;
            int pixelRatio = 1;
            ReadCanvasSize(cssWidth, cssHeight, pixelRatio);

            if (cssWidth != app->canvasCssWidth || cssHeight != app->canvasCssHeight || pixelRatio != app->pixelRatio)
            {
                app->canvasCssWidth = cssWidth;
                app->canvasCssHeight = cssHeight;
                app->pixelRatio = pixelRatio;
                ApplyCanvasSize(cssWidth, cssHeight, pixelRatio);
                app->gameController->SetCanvasSize(DisplayLogicalSize(app->engineWidth, app->engineHeight));
            }

            double const nowMs = emscripten_get_now();
            if (nowMs - app->lastLowFrequencyUpdateMs >= LowFrequencyUpdateIntervalMs)
            {
                app->lastLowFrequencyUpdateMs = nowMs;
                app->gameController->LowFrequencyUpdate();
            }

            UpdateTool(nowMs);

            app->gameController->RunGameIteration();

            AdaptPerformance(nowMs, emscripten_get_now() - nowMs);
        }
        catch (std::exception const & exception)
        {
            emscripten_cancel_main_loop();
            ReportError(std::string("Error during game iteration: ") + exception.what());
        }
    }

    void SetShift(bool value)
    {
        if (app != nullptr && app->gameController && app->isShiftDown != value)
        {
            app->isShiftDown = value;
            app->gameController->SetShiftOn(value);
        }
    }

    void TrackMouse(EmscriptenMouseEvent const * event)
    {
        app->previousMouseX = app->mouseX;
        app->previousMouseY = app->mouseY;
        app->mouseX = static_cast<int>(event->targetX * app->mouseScaleX);
        app->mouseY = static_cast<int>(event->targetY * app->mouseScaleY);
        SetShift(event->shiftKey != 0);
    }

    EM_BOOL OnMouseDown(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app == nullptr || !app->gameController)
        {
            return EM_FALSE;
        }

        try
        {
            TrackMouse(event);
            app->previousMouseX = app->mouseX;
            app->previousMouseY = app->mouseY;

            if (event->button == 0)
            {
                BeginLeft(emscripten_get_now());
            }
            else if (event->button == 1 || event->button == 2)
            {
                app->isPanning = true;
                EndLeft();
            }
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string("Tool error: ") + exception.what());
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseUp(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app == nullptr || !app->gameController)
        {
            return EM_FALSE;
        }

        try
        {
            if (event->button == 0)
            {
                EndLeft();
            }
            else if (event->button == 1 || event->button == 2)
            {
                app->isPanning = false;
            }
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string("Tool error: ") + exception.what());
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseMove(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app == nullptr || !app->gameController)
        {
            return EM_FALSE;
        }

        try
        {
            TrackMouse(event);

            int const deltaX = app->mouseX - app->previousMouseX;
            int const deltaY = app->mouseY - app->previousMouseY;

            if (app->isPanning)
            {
                app->gameController->Pan(DisplayLogicalSize(-deltaX, -deltaY));
            }
            else
            {
                OnToolMouseMove(deltaX, deltaY);
            }
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string("Tool error: ") + exception.what());
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseLeave(int, EmscriptenMouseEvent const *, void *)
    {
        if (app != nullptr && app->gameController)
        {
            try
            {
                app->isPanning = false;
                EndLeft();
            }
            catch (std::exception const & exception)
            {
                ReportError(std::string("Tool error: ") + exception.what());
            }
        }

        return EM_TRUE;
    }

    EM_BOOL OnWheel(int, EmscriptenWheelEvent const * event, void *)
    {
        if (app != nullptr && app->gameController)
        {
            double const scale = (event->deltaMode == DOM_DELTA_LINE) ? 33.0 : 1.0;
            app->gameController->AdjustZoom(std::pow(1.002f, static_cast<float>(-event->deltaY * scale)));
        }

        return EM_TRUE;
    }

    void RegisterInputCallbacks()
    {
        emscripten_set_mousedown_callback(CanvasSelector, nullptr, EM_TRUE, OnMouseDown);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnMouseUp);
        emscripten_set_mousemove_callback(CanvasSelector, nullptr, EM_TRUE, OnMouseMove);
        emscripten_set_mouseleave_callback(CanvasSelector, nullptr, EM_TRUE, OnMouseLeave);
        emscripten_set_wheel_callback(CanvasSelector, nullptr, EM_TRUE, OnWheel);
    }

    void StartGame(void *)
    {
        app = new WebApp();

        try
        {
            app->glContext = CreateGlContext();
            if (app->glContext == 0)
            {
                ReportError("Could not create a WebGL2 context; this browser or device may not support WebGL2");
                return;
            }

            ReadCanvasSize(app->canvasCssWidth, app->canvasCssHeight, app->pixelRatio);
            ApplyCanvasSize(app->canvasCssWidth, app->canvasCssHeight, app->pixelRatio);

            std::filesystem::create_directories(GameRoot);
            std::ofstream(GameExecutablePath).close();

            std::vector<ThreadManager::CpuInfo> cpuResources;
            cpuResources.emplace_back(0, 1.0f);

            app->threadManager = std::make_unique<ThreadManager>(
                false,
                1,
                cpuResources,
                [](ThreadManager::ThreadTaskKind, std::optional<size_t>, size_t, std::string const &)
                {
                });

            app->assetManager = std::make_unique<GameAssetManager>(std::string(GameExecutablePath));

            EMSCRIPTEN_WEBGL_CONTEXT_HANDLE const glContext = app->glContext;

            RenderDeviceProperties const renderDeviceProperties(
                DisplayLogicalSize(app->engineWidth, app->engineHeight),
                1,
                false,
                std::optional<bool>(true),
                std::optional<bool>(true),
                [glContext]()
                {
                    emscripten_webgl_make_context_current(glContext);
                },
                []()
                {
                });

            app->gameController = GameController::Create(
                renderDeviceProperties,
                *app->threadManager,
                *app->assetManager,
                ProgressCallback(
                    [](float progress, ProgressMessageType)
                    {
                        ReportProgress(progress);
                    },
                    0.0f,
                    1.0f));

            app->gameController->RegisterStructuralShipEventHandler(&soundHandler);
            app->gameController->RegisterGenericShipEventHandler(&soundHandler);
            app->gameController->RegisterCombustionEventHandler(&soundHandler);
            app->gameController->RegisterAtmosphereEventHandler(&soundHandler);
            app->gameController->RegisterElectricalElementEventHandler(&electricalHandler);

            app->gameController->ResetAndLoadShip(
                ShipLoadSpecifications(app->assetManager->GetFallbackShipDefinitionFilePath()),
                *app->assetManager);

            RegisterInputCallbacks();

            app->lastLowFrequencyUpdateMs = emscripten_get_now();

            emscripten_set_main_loop(RunFrame, 0, 0);

            ReportReady();
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string("Error during initialization: ") + exception.what());
        }
    }

    bool IsReady()
    {
        return app != nullptr && app->gameController != nullptr;
    }

    template<typename TAction>
    void Guarded(char const * what, TAction && action)
    {
        if (!IsReady())
        {
            return;
        }

        try
        {
            action();
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string(what) + ": " + exception.what());
        }
    }

    struct ScalarSetting
    {
        char const * Name;
        char const * Label;
        char const * Group;
        char const * Kind;
        int Step;
        double (*Get)(IGameControllerSettings const &, IGameControllerSettingsOptions const &);
        double (*Min)(IGameControllerSettings const &, IGameControllerSettingsOptions const &);
        double (*Max)(IGameControllerSettings const &, IGameControllerSettingsOptions const &);
        void (*Set)(IGameControllerSettings &, double);
    };

    ScalarSetting const ScalarSettings[] = {
#include "WebSettings.inc"
    };

    struct EnumSetting
    {
        char const * Name;
        char const * Label;
        char const * Group;
        std::vector<std::string> Options;
        int (*Get)(IGameControllerSettings const &);
        void (*Set)(IGameControllerSettings &, int);
    };

#define FS_ENUM_SETTING(NAME, TYPE, LABEL, GROUP, ...) \
    { #NAME, LABEL, GROUP, { __VA_ARGS__ }, \
        [](IGameControllerSettings const & c) { return static_cast<int>(c.Get##NAME()); }, \
        [](IGameControllerSettings & c, int v) { c.Set##NAME(static_cast<TYPE>(v)); } }

    std::vector<EnumSetting> const & EnumSettings()
    {
        static std::vector<EnumSetting> const settings = {
            FS_ENUM_SETTING(ShipViewMode, ShipViewModeType, "Ship View", "Render parameters", "Exterior", "Interior"),
            FS_ENUM_SETTING(DebugShipRenderMode, DebugShipRenderModeType, "Debug Ship Render", "Render parameters", "None", "Wireframe", "Points", "Springs", "Edge springs", "Structure", "Air pressure", "Strength"),
            FS_ENUM_SETTING(HeatRenderMode, HeatRenderModeType, "Heat Render", "Render parameters", "None", "Incandescence", "Heat overlay"),
            FS_ENUM_SETTING(StressRenderMode, StressRenderModeType, "Stress Render", "Render parameters", "None", "Stress overlay", "Tension overlay"),
            FS_ENUM_SETTING(ShipParticleRenderMode, ShipParticleRenderModeType, "Ship Particle Render", "Render parameters", "Fragment", "Particle"),
            FS_ENUM_SETTING(NpcRenderMode, NpcRenderModeType, "NPC Render", "Render parameters", "Texture", "Quad with roles", "Quad flat"),
            FS_ENUM_SETTING(VectorFieldRenderMode, VectorFieldRenderModeType, "Vector Field", "Render parameters", "None", "Point velocity", "Point static force", "Point dynamic force", "Point air momentum", "Point water momentum", "Point air and water momentum"),
            FS_ENUM_SETTING(OceanRenderMode, OceanRenderModeType, "Ocean Render", "Render parameters", "Texture", "Depth", "Flat"),
            FS_ENUM_SETTING(OceanRenderDetail, OceanRenderDetailType, "Ocean Detail", "Render parameters", "Basic", "Detailed"),
            FS_ENUM_SETTING(LandRenderMode, LandRenderModeType, "Land Render", "Render parameters", "Texture", "Flat"),
            FS_ENUM_SETTING(LandRenderDetail, LandRenderDetailType, "Land Detail", "Render parameters", "Basic", "Detailed"),
            FS_ENUM_SETTING(CloudRenderDetail, CloudRenderDetailType, "Cloud Detail", "Render parameters", "Basic", "Detailed")
        };

        return settings;
    }

    std::string JsonEscape(std::string const & text)
    {
        std::string result;
        for (char const c : text)
        {
            if (c == '"' || c == '\\')
            {
                result += '\\';
            }

            result += c;
        }

        return result;
    }

    std::string FormatNumber(double value)
    {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.9g", value);
        return buffer;
    }

    std::string BuildSettingsJson()
    {
        IGameControllerSettings const & settings = *app->gameController;
        IGameControllerSettingsOptions const & options = *app->gameController;

        std::string json = "[";
        bool isFirst = true;

        for (ScalarSetting const & setting : ScalarSettings)
        {
            if (!isFirst)
            {
                json += ",";
            }

            isFirst = false;
            json += "{\"name\":\"" + std::string(setting.Name) + "\",\"label\":\"" + JsonEscape(setting.Label)
                + "\",\"group\":\"" + setting.Group + "\",\"kind\":\"" + setting.Kind
                + "\",\"value\":" + FormatNumber(setting.Get(settings, options))
                + ",\"min\":" + FormatNumber(setting.Min(settings, options))
                + ",\"max\":" + FormatNumber(setting.Max(settings, options)) + "}";
        }

        for (EnumSetting const & setting : EnumSettings())
        {
            if (!isFirst)
            {
                json += ",";
            }

            isFirst = false;
            json += "{\"name\":\"" + std::string(setting.Name) + "\",\"label\":\"" + JsonEscape(setting.Label)
                + "\",\"group\":\"" + setting.Group + "\",\"kind\":\"enum\",\"value\":" + std::to_string(setting.Get(settings))
                + ",\"options\":[";

            for (size_t i = 0; i < setting.Options.size(); ++i)
            {
                json += (i == 0 ? "\"" : ",\"") + JsonEscape(setting.Options[i]) + "\"";
            }

            json += "]}";
        }

        json += "]";
        return json;
    }

    std::string SettingsJsonBuffer;
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE int fs_load_ship(char const * path)
    {
        if (!IsReady() || !app->assetManager)
        {
            return 0;
        }

        try
        {
            EndLeft();

            app->gameController->ResetAndLoadShip(
                ShipLoadSpecifications(std::filesystem::path(path)),
                *app->assetManager);

            return 1;
        }
        catch (std::exception const & exception)
        {
            ReportError(std::string("Could not load ship: ") + exception.what());
            return 0;
        }
    }


    EMSCRIPTEN_KEEPALIVE char const * fs_settings_json()
    {
        SettingsJsonBuffer.clear();

        if (IsReady())
        {
            try
            {
                SettingsJsonBuffer = BuildSettingsJson();
            }
            catch (std::exception const & exception)
            {
                ReportError(std::string("Settings error: ") + exception.what());
            }
        }

        return SettingsJsonBuffer.c_str();
    }

    EMSCRIPTEN_KEEPALIVE int fs_set_setting(char const * name, double value)
    {
        if (!IsReady())
        {
            return 0;
        }

        int result = 0;

        Guarded("Settings error", [&]()
        {
            IGameControllerSettings & settings = *app->gameController;

            for (ScalarSetting const & setting : ScalarSettings)
            {
                if (std::string(setting.Name) == name)
                {
                    setting.Set(settings, value);
                    result = 1;
                    return;
                }
            }

            for (EnumSetting const & setting : EnumSettings())
            {
                if (std::string(setting.Name) == name)
                {
                    setting.Set(settings, static_cast<int>(value + 0.5));
                    result = 1;
                    return;
                }
            }
        });

        return result;
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_switch(unsigned int shipId, unsigned int localId, int isOn)
    {
        Guarded("Switch error", [&]()
        {
            app->gameController->SetSwitchState(
                GlobalElectricalElementId(static_cast<ShipId>(shipId), static_cast<ElementIndex>(localId)),
                isOn ? ElectricalState::On : ElectricalState::Off);
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_engine(unsigned int shipId, unsigned int localId, double value)
    {
        Guarded("Engine control error", [&]()
        {
            app->gameController->SetEngineControllerState(
                GlobalElectricalElementId(static_cast<ShipId>(shipId), static_cast<ElementIndex>(localId)),
                static_cast<float>(value));
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_add_npc_group(int isHuman)
    {
        Guarded("NPC error", [&]()
        {
            app->gameController->AddNpcGroup(isHuman ? NpcKindType::Human : NpcKindType::Furniture);
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_select_next_npc()
    {
        Guarded("NPC error", [&]()
        {
            app->gameController->SelectNextNpc();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_trigger(char const * name)
    {
        Guarded("Trigger error", [&]()
        {
            std::string const what(name);
            IGameController & controller = *app->gameController;

            if (what == "tsunami")
            {
                controller.TriggerTsunami();
            }
            else if (what == "roguewave")
            {
                controller.TriggerRogueWave();
            }
            else if (what == "storm")
            {
                controller.TriggerStorm();
            }
            else if (what == "lightning")
            {
                controller.TriggerRandomLightning();
            }
            else if (what == "removeantigravity")
            {
                controller.RemoveAllAntiGravityFields();
            }
            else if (what == "panleft")
            {
                controller.PanToWorldEnd(0);
            }
            else if (what == "panright")
            {
                controller.PanToWorldEnd(1);
            }
        });
    }

    EMSCRIPTEN_KEEPALIVE int fs_set_tool(char const * name)
    {
        if (!IsReady())
        {
            return 0;
        }

        auto const it = ToolsByName.find(name);
        if (it == ToolsByName.end())
        {
            return 0;
        }

        Guarded("Tool error", [&]()
        {
            EndLeft();
            ResetToolSpecificState();
            app->tool = it->second;
        });

        return 1;
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_paused(int isPaused)
    {
        Guarded("Pause error", [&]()
        {
            app->isPaused = isPaused != 0;
            app->gameController->SetPaused(app->isPaused);
        });
    }

    EMSCRIPTEN_KEEPALIVE int fs_is_paused()
    {
        return (IsReady() && app->isPaused) ? 1 : 0;
    }

    EMSCRIPTEN_KEEPALIVE void fs_step()
    {
        Guarded("Step error", [&]()
        {
            app->gameController->PulseUpdateAtNextGameIteration();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_zoom(float amount)
    {
        Guarded("Zoom error", [&]()
        {
            app->gameController->AdjustZoom(amount);
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_perf_mode(int mode)
    {
        Guarded("Performance error", [&]()
        {
            SetPerfMode(mode);
        });
    }

    EMSCRIPTEN_KEEPALIVE int fs_perf_mode()
    {
        return app != nullptr ? app->perfMode : 0;
    }

    EMSCRIPTEN_KEEPALIVE void fs_reset_view()
    {
        Guarded("View error", [&]()
        {
            app->gameController->ResetView();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_focus_on_ships()
    {
        Guarded("View error", [&]()
        {
            app->gameController->FocusOnShips();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_toggle_day_night()
    {
        Guarded("Day/night error", [&]()
        {
            app->gameController->ToggleToFullDayOrNight();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_detonate_rc_bombs()
    {
        Guarded("Bomb error", [&]()
        {
            app->gameController->DetonateRCBombs();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_detonate_anti_matter_bombs()
    {
        Guarded("Bomb error", [&]()
        {
            app->gameController->DetonateAntiMatterBombs();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_remove_all_pins()
    {
        Guarded("Pin error", [&]()
        {
            app->gameController->RemoveAllPins();
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_status_text(int isShown)
    {
        Guarded("Status text error", [&]()
        {
            app->gameController->SetShowStatusText(isShown != 0);
            app->gameController->SetShowExtendedStatusText(isShown != 0);
        });
    }

    EMSCRIPTEN_KEEPALIVE void fs_set_shift(int isDown)
    {
        if (IsReady())
        {
            SetShift(isDown != 0);
        }
    }
}

int main()
{
    emscripten_async_call(StartGame, nullptr, 50);

    return 0;
}
