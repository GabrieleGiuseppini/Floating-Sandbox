# Native Apple Silicon macOS port

For consolidated source changes and reproducible commands, see [BUILD-macOS.md](BUILD-macOS.md).

## Current deliverable

`build-macos-arm64/package/Floating Sandbox.app` is a self-contained, double-click
application for Apple Silicon macOS 26+. The adjacent
`Floating-Sandbox-macOS-arm64.zip` is the portable package. No Homebrew, Terminal,
or build tree is needed to run the app. Copy the complete `.app` to Applications
or another folder. This is an ad-hoc signed personal-use build, not notarized.

Rebuild the app and ZIP from the configured build with:

```sh
cmake --build build-macos-arm64 --target macos-package -j 8
```

The sections below preserve the investigation and development milestones.

## Target and baseline

- macOS 26+, arm64, Apple Clang, native development executable first.
- Baseline: branch `master`, commit `90b3472c`; working tree initially clean.
- Preserve physics, wxWidgets UI, SFML audio/network, OpenGL renderer and assets.
- No application bundle until the development executable is validated.

## Inspection

Windows uses MSVC, configured dependency roots and runtime DLL installation.
Linux uses GCC 14+, wxWidgets GTK, SFML 2.x and a flat installed resource tree.
Dependencies are external, except the bundled GLAD loader; PicoJSON is header-only.
The current build requires a GoogleTest source checkout and a user-defined install macro.

Existing abstractions already detect macOS and AArch64, implement ARM NEON physics,
use standard threads, and disable threaded rendering on macOS. GLAD already loads
Apple's OpenGL framework. wxGLCanvas owns context creation; preserve its existing
profile first and observe real context/shader failures before changing rendering.
Resource and save paths use the existing asset manager and wx standard paths;
startup, user directories, keyboard/mouse and Retina behavior need runtime checks.

## Dependencies

- Apple Clang 21 and macOS SDK: available; host reports arm64.
- CMake: missing from PATH at initial inspection.
- wxWidgets Cocoa, SFML **2.x** (system/audio/network), PicoJSON, GoogleTest: needed.
- Homebrew arm64 libpng and jpeg-turbo available; system zlib/OpenGL available.
- Use Homebrew dynamic libraries for the personal development build. Retain SFML;
  its audio backend dependencies must be checked through actual binary linkage.

## Plan / shortest build path

1. Install missing native build dependencies; configure `build-macos-arm64/` with
   Apple Clang, arm64 and deployment target 26.0, benchmarks disabled.
2. Fix observed CMake blockers, then compile FloatingSandbox and fix precise
   Clang/macOS/ARM failures in small units. Preserve Windows/Linux paths.
3. Install the flat development tree, inspect architecture/linkage, launch and
   collect actual logs/crashes. Validate window, context, shaders, assets, menus,
   ship simulation, input, audio, saving/loading in that order.
4. Validate Retina, resizing/fullscreen, focus, dialogs and user paths.
5. Only then add a conventional Finder-launchable `.app` with independent resources.

## Blockers and unresolved issues

- Initial tooling/dependency setup pending.
- CMake uses `find_package(iconv)` on Apple; inspect actual configure failure.
- SFML network is linked but not requested in `find_package` components.
- Unconditional GoogleTest source dependency and install callback require settings.
- Runtime OpenGL compatibility and all GUI/audio behavior remain untested.

## Changes / files modified

- `MACOS_PORT.md`: initial inspection, plan and milestone tracking.

## Milestones and testing status

- Repository, recent relevant history, Windows/Linux build docs and platform
  abstractions inspected. No source changes yet.
- Configuration, compilation, executable launch and functional validation pending.

### Configuration milestone

Native configuration passed with CMake 4.4.3, wxWidgets Cocoa 3.3.3,
SFML 2.6.2 and GoogleTest 1.18.0. PicoJSON 1.3.0 header fetched from its upstream
release into `build-macos-arm64/deps/` (not vendored).

Observed CMake failures fixed:
- `CMakeLists.txt`: accept installed GTest config when `GTEST_DIR` is unset;
  retain source-checkout behavior when set. Guard optional user install callback.
  Correct `Iconv` package casing and request SFML network explicitly.
- `Sources/UnitTests/CMakeLists.txt`: use imported GTest targets for installed packages.
- `.gitignore`: exclude native build directory.

Reproduce after `brew install cmake wxwidgets sfml@2 googletest`:

```sh
mkdir -p build-macos-arm64/deps
curl -fL https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h \
  -o build-macos-arm64/deps/picojson.h
cmake -S . -B build-macos-arm64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 \
  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DFS_BUILD_BENCHMARKS=OFF -DFS_USE_STATIC_LIBS=OFF \
  -DPICOJSON_DIR="$PWD/build-macos-arm64/deps" \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sfml@2
cmake --build build-macos-arm64 --target FloatingSandbox -j 8
```

Initial compilation confirms ARM64, macOS and NEON selected; Core and OpenGLCore
compiled. Full executable build is in progress. Logs are in the ignored build tree.

### Native launch and audio crash (2026-09-15)

The development executable and ShipBuilder compiled and installed. `file` confirms
arm64. Initial recursive linkage inspection found arm64 support in all 25 Homebrew
library paths. A sandboxed launch could not connect to macOS window services;
launching normally reached the main window and simulation.

Runtime logs confirm Apple M5, OpenGL 2.1, required extensions, successful shader
initialization, texture loading, ship loading and first rendered frame. The user
confirmed the game worked, then reported a crash when Titanic collapsed and sank.

Crash report `FloatingSandbox-2026-09-15-164608.ips` shows EXC_BAD_ACCESS at null on
an audio thread: Apple's `OALSource::RemoveBuffersFromQueue` →
`alSourceUnqueueBuffers` → SFML `SoundStream::clearQueue` / `streamData`.
This identifies the failing audio path, not a proven physics or graphics defect.

Rebuilt the same SFML 2.6.2 release against Homebrew OpenAL Soft 1.25.2, preserving
SFML and its public API. Sources and binaries stay in the ignored build directory.
No dependency source patches were required. SFML's Apple OpenAL finder assumes a
framework, so `OPENAL_FULL_PATH` must also be set to the dylib explicitly.

```sh
brew install openal-soft
curl -fL https://github.com/SFML/SFML/archive/refs/tags/2.6.2.tar.gz \
  -o build-macos-arm64/deps/sfml-2.6.2.tar.gz
tar -xzf build-macos-arm64/deps/sfml-2.6.2.tar.gz -C build-macos-arm64/deps
cmake -S build-macos-arm64/deps/SFML-2.6.2 \
  -B build-macos-arm64/deps/sfml-build \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-macos-arm64/deps/sfml-install" \
  -DSFML_BUILD_WINDOW=OFF -DSFML_BUILD_GRAPHICS=OFF \
  -DSFML_BUILD_AUDIO=ON -DSFML_BUILD_NETWORK=ON \
  -DSFML_BUILD_FRAMEWORKS=OFF -DSFML_USE_SYSTEM_DEPS=ON \
  -DOPENAL_INCLUDE_DIR=/opt/homebrew/opt/openal-soft/include/AL \
  -DOPENAL_LIBRARY=/opt/homebrew/opt/openal-soft/lib/libopenal.dylib \
  -DOPENAL_FULL_PATH=/opt/homebrew/opt/openal-soft/lib/libopenal.dylib
cmake --build build-macos-arm64/deps/sfml-build -j 8
cmake --install build-macos-arm64/deps/sfml-build
cmake -S . -B build-macos-arm64 \
  -DSFML_DIR="$PWD/build-macos-arm64/deps/sfml-install/lib/cmake/SFML" \
  -DCMAKE_INSTALL_RPATH="$PWD/build-macos-arm64/deps/sfml-install/lib"
cmake --build build-macos-arm64 --target FloatingSandbox ShipBuilder UnitTests -j 8
cmake --install build-macos-arm64
./build-macos-arm64/Install/FloatingSandbox
```

The absolute install RPATH is intentional for this local development tree. It is
not a distributable bundle; do not move/delete its SFML dependency directory.

### Unit tests

All 1,014 tests passed after fixing existing test portability problems:
- ARM lighting fixtures still passed interleaved positions to the newer split-X/Y
  API; corrected fixture layout. Approximate NEON reciprocal lighting checks now
  use 0.001 absolute tolerance, preserving the intentionally approximate algorithm.
- ARM smoothing fixture contains 16 samples; naive test now uses that count.
  Fixed stale helper name in the NEON test.
- Image resize tests applied a GCC rounding adjustment to Clang; restrict it to GCC.
- Enabled top-level CTest discovery and removed duplicate test registration.

Changed additional files: `Sources/UnitTests/AlgorithmsTests.cpp`,
`Sources/UnitTests/ImageToolsTests.cpp`. Production C++ remains unchanged.

A local muted SFML stress executable completed 100 sinking-music stream
start/stop/restart/destruction cycles against OpenAL Soft without errors.
The rebuilt game launched successfully; user sinking replay is pending.

### Remaining acceptance checks

- Confirm Titanic sinking no longer crashes with OpenAL Soft.
- Validate saving/loading through the UI, extended simulation, audio transitions.
- Retina/resizing/fullscreen, scrolling, focus, dialogs and save/config locations.
- App bundle and Finder launch deferred until development runtime is reliable.
- Windows/Linux builds were preserved structurally but not executed on this host.
- UI automation cannot currently select the unbundled executable; user provides
  visual/input verification while logs and native tests provide technical evidence.

### Audio fix verification and current handoff

The identical 100-cycle audio harness crashes with the original Homebrew SFML /
Apple OpenAL backend (exit 139). Its crash report has the same
`OALSource::RemoveBuffersFromQueue` → `alSourceUnqueueBuffers` →
`SoundStream::clearQueue` stack. OpenAL Soft completes all 100 cycles.
The user confirmed the rebuilt game survives Titanic breaking/sinking with sound.

Final CTest run passed (1 registered suite executable, 1,014 internal tests).
Recursive inspection of the executable and 24 resolved native dependency libraries
found arm64 in every binary. Reopened the fixed development executable on request.
Ship loading UI is reachable (ship-preview completion appears in the latest log).
A second ship load and save/reload still need user confirmation.

## Open and test the current development build

From Terminal, regardless of the current directory:

```sh
/Users/bradene/repos/Floating-Sandbox/build-macos-arm64/Install/FloatingSandbox
```

1. Use **File → Load Ship…** in the macOS menu bar and double-click a ship thumbnail.
2. Press **Space** to pause/resume. Try mouse tools and scroll zoom.
3. Break Titanic and let it sink; check sound and music transitions.
4. Load a different ship, then use **File → Reload Current Ship**.
5. Test **Options → Full Screen**, resizing and switching to another app and back.
6. For ship saving, use **ShipBuilder → Edit This Ship…**, then **Save Ship As**
   to create a new test copy; load that copy from the simulator. Do not overwrite
   a bundled original. Saving a ship design is distinct from saving simulation state.

Report any crash, missing sound, blank rendering, incorrect pointer position or
failed save/load. Runtime logs are under `build-macos-arm64/`; system crash reports
are in `~/Library/Logs/DiagnosticReports/`. `.app` packaging remains the next
stage after remaining development acceptance checks. No commits or pushes made.

## Application bundle milestone

User confirmed the native runtime mostly works and requested a single-click
package. Implemented a conventional `Contents/MacOS`, `Contents/Resources`,
`Contents/Frameworks` bundle with the original ship icon, version metadata,
resources and 24 arm64 dependency libraries, including SFML + OpenAL Soft.

- `Scripts/package_macos.py`: traverses actual binary dependencies, requires
  arm64-only binaries, copies assets and available dependency licenses, rewrites
  linkage to bundle-relative paths, removes external RPATHs and ad-hoc signs.
  Rejects unresolved/colliding libraries and external binary dependencies.
- `Sources/FloatingSandbox/CMakeLists.txt`: Apple-only `macos-app` and
  `macos-package` build targets. Flat development install remains available.
- `Sources/Game/GameAssetManager.cpp`: resolves conventional bundle Resources
  from the executable location while preserving flat executable behavior.
- `BootSettings.h/.cpp`, `MainApp.cpp`, `UI/BootSettingsDialog.cpp`: macOS boot
  settings use the existing user configuration directory, outside signed assets.
- `Sources/UILib/ShipPreviewWindow.h/.cpp`: macOS thumbnail databases use a
  per-directory user cache instead of writing into the signed Ships directory.

### Packaging issues found and resolved

Legacy `.dat` ship sidecars are required assets and are included. Initial loading
of Varyag exposed their omission; Varyag subsequently loaded/rendered successfully.
Thumbnail browsing initially invalidated the signature by adding a database inside
the bundle; the user cache fixes this and strict signature checks now pass after use.

Save As successfully wrote a test ship. Returning from the editor exposed a second
crash, distinct from audio: `glUseProgram` reported INVALID_VALUE after editor
cleanup, followed by termination while destroying RenderContext.

The editor now explicitly binds its owned OpenGL context before rendering and
resource destruction, and releases resources before hiding its canvas. Files:
`Sources/ShipBuilderLib/OpenGLManager.h`, `View.h/.cpp`, `MainFrame.cpp`.
The same save-and-return replay and a repeated abandon-and-return both succeeded
with rendering and simulation continuing. Normal Command-Q exited with status 0.

### Verification

- LaunchServices opened the bundle; default Titanic rendered successfully.
- A copy outside the repository launched and loaded S.S. Valence and legacy Varyag.
- Launch with `/` as working directory succeeded and loaded a saved `.shp2`.
- Native Command-O, menus, ship previews, editor and macOS save dialog worked.
- Saved test ship, loaded it, returned from editor twice; clean shutdown passed.
- Strict deep code-signature verification passed after browsing and saving.
- Every packaged executable/library is arm64-only; dependencies resolve inside
  the bundle or Apple's system directories.
- ZIP extracted successfully and retained its valid signature.
- Full CTest suite passed after changes (1,014 internal tests).

No Metal migration, Intel build, notarization, Developer ID, DMG or public
distribution work performed. Extended gameplay and other Mac hardware remain
unverified; Windows/Linux were not built on this host.

### Clean ZIP metadata correction

The ZIP target now uses `ditto -c -k --norsrc --noextattr --noacl --keepParent`.
Removed `--sequesterRsrc`, which created AppleDouble files under `__MACOSX/`.
The resource copy excludes source-side `._*` metadata as well. The replacement
archive is checked for forbidden metadata entries, valid CRCs, and a valid code
signature after extraction. See `BUILD-macOS.md` for the complete procedure.

Clean archive verification completed: 2,536 entries, zero `__MACOSX`, `._*` or
`.DS_Store` entries, all CRCs valid. Extracted app passed strict deep signature
verification; all 25 executable/library binaries remain arm64-only.
