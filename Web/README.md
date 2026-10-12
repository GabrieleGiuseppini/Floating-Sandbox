# Floating Sandbox - WebAssembly port

Work in progress. Goal: run Floating Sandbox in a browser.

## Status

The game boots in a browser. The simulation, the renderer and the asset pipeline run as WebAssembly on WebGL2:

- The built-in logo ship loads, simulates and renders (ocean, clouds, textured ship, HUD).
- Left-drag pans and the mouse wheel zooms.
- "Load ship..." loads a `.shp2`, `.shp` or `.png` ship definition from disk.
- Verified in headless Chromium with software WebGL (about 10 FPS, which says nothing about real GPU performance). Not yet tried on real hardware or other browsers.

## Building

Requirements: Emscripten 3.1.50 on `PATH` (`emcmake` available), plus wasm builds of zlib and libpng installed under one prefix, and picojson 1.3.0.

```
export FS_WEB_DEPS_PREFIX=/path/to/prefix/with/libz.a/and/libpng16.a
export FS_PICOJSON_DIR=/path/to/picojson
Web/build.sh
```

Optional: `FS_WEB_BUILD_DIR`, `FS_WEB_BUILD_TYPE` (default `Release`), `FS_WEB_JOBS` (default `2`).

The output is `Web/build/index.html` plus `index.js`, `index.wasm` and `index.data` (about 80 MB of game data; music, sounds, languages, help and themes are left out). Serve the directory over HTTP and open it:

```
python3 -m http.server -d Web/build 8000
```

No special headers are needed because the build is single-threaded.

## Layout

- `Web/Main.cpp`: entry point; WebGL2 context, single-threaded `ThreadManager`, `GameController`, frame loop, input.
- `Web/shell.html`: page shell with the loading bar, error display and ship loader.
- `Web/CMakeLists.txt`, `Web/build.sh`: Emscripten build.
- `Sources/OpenGLCore/web/GLWeb.h`: WebGL2 (GLES3) shim replacing `glad`; emulates `glMapBuffer`, 1D textures and `glPointSize`, and ignores desktop-only capabilities.
- `Sources/OpenGLCore/web/ShaderConverter.h`: converts the GLSL 120 shaders to GLSL ES 3.00 at load time. All 100 Game shader programs were checked with `glslangValidator`.

## Changes to existing sources

- `Core/SysSpecifics.h`: Emscripten is treated as a 32-bit target so `register_int` is defined.
- `Render/ShipRenderContext.cpp`: `PopulateAttributeGroup` takes its destination type as a template parameter, because clang rejects naming a private nested type from a free function.
- `OpenGLCore/GameOpenGL_Ext.*` and `GameOpenGL.cpp`: extension loading and the GL debug callback are skipped on Emscripten; shaders go through the converter before compilation.
- `OpenGLCore/ShaderManager.cpp.inl`: uniforms starting with `glweb` are exempt from the `param` naming rule.

## Known gaps

- No ship builder or ship saving.
- The Move Gripped tool, the NPC subkind picker and colour settings are not ported.
- Point size is passed through a `glwebPointSize` uniform that the converter injects into every vertex shader.
- `GL_TEXTURE_1D` is mapped to a height-1 2D texture, so the cloud shadows texture shares the 2D binding on its texture unit.
- Line smoothing, point sprite toggles, multisampling and polygon mode are ignored.
- Single-threaded: no render thread and no simulation thread pool.
- Scalar math only; the SSE paths are not enabled for WebAssembly SIMD.
- All 80 MB of game data is downloaded before start-up; it needs lazy loading.
- The final link uses `-O0` for the JavaScript glue because Emscripten's npm-based optimizer was not available when this was built.

## Menus, tools, ships and sound

The page has a menu bar (File, Tools, Controls, View, Sound, Help). Tools work with the mouse like the desktop game: left button uses the current tool, right or middle drag pans, the wheel zooms, Shift modifies tools.

The build step generates `ships.json` plus a `Ships/` folder from the game's `Ships` directory (File > Load Ship...), and converts `Data/Sounds` and `Data/Music` to small Opus files with `sounds.json` (needs `ffmpeg` on the build machine). Sounds and music are fetched on demand, not bundled in `index.data`. Audio starts after the first click or key press, as browsers require.

## NPCs, electrical panel and settings

Tools menu has Effects (blast, electric spark, laser cannon, lamp, Thanos snap, wind maker, terrain adjust, tornado, anti-gravity) and NPCs (add human, add furniture, move, remove, turn around, follow, plus add-group and select-next commands). Ships with electrical elements get a floating Switchboard panel (switches, engine telegraphs, jet throttles, probes). View and Controls menus expose the render-mode enums, and a Settings dialog is generated from `IGameControllerSettings` with `Web/generate_settings.py` into `Web/WebSettings.inc`.

## Performance

The link step runs at `-O0` because Emscripten's JS optimizer needs npm modules that may not be installed; instead a post-build step runs Binaryen's `wasm-opt -O3` on `index.wasm` when `wasm-opt` is found (set `FS_BINARYEN_ROOT` or `BINARYEN_ROOT`). The build uses WebAssembly SIMD (`-msimd128`), the desktop SSE spring-force kernel is used through Emscripten's SSE-to-SIMD headers, and per-call `glGetError` checks and buffer-size queries are disabled for the web build because they stall the GPU pipeline.
