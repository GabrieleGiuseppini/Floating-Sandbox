#include <Core/Log.h>
#include <Core/ThreadManager.h>
#include <Game/GameAssetManager.h>
#include <Game/GameController.h>
#include <Game/ShipLoadSpecifications.h>
#include <Render/RenderDeviceProperties.h>
#include <Simulation/ISimulationEventHandlers.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <chrono>
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
        Lightning
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
        {"lightning", Tool::Lightning}
    };

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
        double lastLowFrequencyUpdateMs = 0.0;

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

    void ApplyCanvasSize(int cssWidth, int cssHeight, int pixelRatio)
    {
        emscripten_set_canvas_element_size(CanvasSelector, cssWidth * pixelRatio, cssHeight * pixelRatio);
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

        EndMove();
        ReleasePull();
    }

    void UpdateTool(double nowMs)
    {
        if (!app->isLeftDown || app->isPanning)
        {
            return;
        }

        IGameController & controller = *app->gameController;
        DisplayLogicalCoordinates const position = MousePosition();
        bool const shift = app->isShiftDown;

        switch (app->tool)
        {
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
                app->gameController->SetCanvasSize(DisplayLogicalSize(cssWidth, cssHeight));
            }

            double const nowMs = emscripten_get_now();
            if (nowMs - app->lastLowFrequencyUpdateMs >= LowFrequencyUpdateIntervalMs)
            {
                app->lastLowFrequencyUpdateMs = nowMs;
                app->gameController->LowFrequencyUpdate();
            }

            UpdateTool(nowMs);

            app->gameController->RunGameIteration();
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
        app->mouseX = static_cast<int>(event->targetX);
        app->mouseY = static_cast<int>(event->targetY);
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
                DisplayLogicalSize(app->canvasCssWidth, app->canvasCssHeight),
                app->pixelRatio,
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
