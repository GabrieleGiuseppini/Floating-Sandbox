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

- No audio (the SFML sound and music controllers are not ported).
- No UI beyond pan, zoom and ship loading: no settings, menus, interactive tools (grab, bombs, saw, and so on), ship builder or electrical panel.
- Only the fallback ship is bundled; other ships must be loaded from disk.
- Point size is passed through a `glwebPointSize` uniform that the converter injects into every vertex shader.
- `GL_TEXTURE_1D` is mapped to a height-1 2D texture, so the cloud shadows texture shares the 2D binding on its texture unit.
- Line smoothing, point sprite toggles, multisampling and polygon mode are ignored.
- Single-threaded: no render thread and no simulation thread pool.
- Scalar math only; the SSE paths are not enabled for WebAssembly SIMD.
- All 80 MB of game data is downloaded before start-up; it needs lazy loading.
- The final link uses `-O0` for the JavaScript glue because Emscripten's npm-based optimizer was not available when this was built.
