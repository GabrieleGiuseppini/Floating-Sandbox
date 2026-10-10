# Floating Sandbox - WebAssembly port

Work in progress. Goal: run Floating Sandbox in a browser.

## Status

- Core, Simulation, Game, OpenGLCore, Render and SoundCore compile and link to WebAssembly with Emscripten.
- The desktop `glad` loader is replaced on Emscripten by `Sources/OpenGLCore/web/GLWeb.h`, which targets WebGL2 (GLES3).
- Nothing has been run in a browser yet. There is no entry point, no web UI, no audio and no asset loading.

## Building

Requirements: Emscripten 3.1.50 on `PATH` (`emcmake` available), plus wasm builds of zlib and libpng installed under one prefix, and picojson 1.3.0.

```
export FS_WEB_DEPS_PREFIX=/path/to/prefix/with/libz.a/and/libpng16.a
export FS_PICOJSON_DIR=/path/to/picojson
Web/build.sh
```

Optional: `FS_WEB_BUILD_DIR`, `FS_WEB_BUILD_TYPE` (default `Debug`), `FS_WEB_JOBS` (default `2`).

## Changes to existing sources

- `Core/SysSpecifics.h`: Emscripten is treated as a 32-bit target so `register_int` is defined.
- `Render/ShipRenderContext.cpp`: `PopulateAttributeGroup` takes its destination type as a template parameter, because clang rejects naming a private nested type from a free function.
- `OpenGLCore/GameOpenGL_Ext.*` and `GameOpenGL.cpp`: extension loading and the GL debug callback are skipped on Emscripten.

## Known gaps

- Shaders in `Data/Shaders` are GLSL 120 and must be converted to `#version 300 es`.
- No shader writes `gl_PointSize`; point size currently comes from `glPointSize`, which the shim only records in `glweb::PointSize`. Shaders need a point size uniform.
- `sampler1D` and `texture1D` in shaders must become `sampler2D` and `texture`; the shim maps `GL_TEXTURE_1D` to a height-1 2D texture.
- The cloud shadows texture is `GL_R32F` with linear filtering, which needs the `OES_texture_float_linear` extension in WebGL2.
- `glMapBuffer` is emulated with a CPU shadow buffer uploaded on unmap.
- Line smoothing, point sprites, multisample toggles and polygon mode are ignored.
- The wxWidgets UI and the SFML audio code are not ported.
- Game data (about 190 MB) needs lazy loading.
- Threads are not enabled; the render thread and simulation thread pool still need a single-threaded path.
