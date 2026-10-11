#include <Core/Log.h>
#include <Core/ThreadManager.h>
#include <Game/GameAssetManager.h>
#include <Game/GameController.h>
#include <Game/ShipLoadSpecifications.h>
#include <Render/RenderDeviceProperties.h>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace
{
    constexpr char const * CanvasSelector = "#canvas";
    constexpr char const * GameRoot = "/game";
    constexpr char const * GameExecutablePath = "/game/FloatingSandbox";
    constexpr double LowFrequencyUpdateIntervalMs = 1000.0;

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
        bool isDragging = false;
    };

    WebApp * app = nullptr;

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

            app->gameController->RunGameIteration();
        }
        catch (std::exception const & exception)
        {
            emscripten_cancel_main_loop();
            ReportError(std::string("Error during game iteration: ") + exception.what());
        }
    }

    EM_BOOL OnMouseDown(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app != nullptr && event->button == 0)
        {
            app->isDragging = true;
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseUp(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app != nullptr && event->button == 0)
        {
            app->isDragging = false;
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseMove(int, EmscriptenMouseEvent const * event, void *)
    {
        if (app != nullptr && app->isDragging && app->gameController)
        {
            app->gameController->Pan(DisplayLogicalSize(-event->movementX, -event->movementY));
        }

        return EM_TRUE;
    }

    EM_BOOL OnMouseLeave(int, EmscriptenMouseEvent const *, void *)
    {
        if (app != nullptr)
        {
            app->isDragging = false;
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
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE int fs_load_ship(char const * path)
    {
        if (app == nullptr || !app->gameController || !app->assetManager)
        {
            return 0;
        }

        try
        {
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
}

int main()
{
    emscripten_async_call(StartGame, nullptr, 50);

    return 0;
}
