# Floating Sandbox: native Apple Silicon build and package

This guide builds the existing game with Apple Clang, wxWidgets Cocoa, SFML 2.6.2,
OpenAL Soft and Apple's OpenGL. It produces an arm64-only `.app` and a clean ZIP
for macOS 26 or later. No Wine, Rosetta, renderer rewrite or Metal migration.

Source baseline: `90b3472c` on this fork's `master`, plus the port changes listed
below. Run all commands from the repository root. Use the existing checkout;
there is no need to clone Floating Sandbox again.

## Applying the supplied source patch

`Floating-Sandbox-macOS-source.patch` contains all port source changes and these
documents relative to baseline `90b3472c`. The current working checkout already
contains them; do not apply the patch here again. On a separate clean checkout of
that baseline, check and apply it before following this guide:

```sh
git apply --check /path/to/Floating-Sandbox-macOS-source.patch
git apply /path/to/Floating-Sandbox-macOS-source.patch
```

## 1. Prerequisites and tested versions

Install Apple's Command Line Tools if not already installed:

```sh
xcode-select --install
```

Check that the shell and compiler are native arm64:

```sh
uname -m
xcrun clang++ --version
xcrun --sdk macosx --show-sdk-path
```

The tested compiler was Apple Clang 21.0.0 (`clang-2100.0.123.102`). Homebrew must
be the arm64 installation under `/opt/homebrew`.

```sh
brew install cmake python@3.14 wxwidgets googletest openal-soft \
  libpng jpeg-turbo flac libvorbis libogg
```

| Dependency | Tested version | Use |
| --- | --- | --- |
| CMake | 4.4.3 | Build configuration |
| Python | 3.14.4; script requires 3.9+ | App packaging |
| wxWidgets | 3.3.3 | Native Cocoa UI and OpenGL canvas |
| GoogleTest | 1.18.0 | Existing unit tests |
| PicoJSON | 1.3.0 | Header-only JSON |
| SFML | 2.6.2, built below | Audio, network, system |
| OpenAL Soft | 1.25.2 | Audio backend |
| libpng | 1.6.58 | PNG decoding |
| jpeg-turbo | 3.2.0 | JPEG decoding |
| FLAC | 1.5.0 | SFML audio decoding |
| libvorbis | 1.3.7 | SFML audio decoding |
| libogg | 1.3.6 | SFML audio decoding |
| OpenGL / zlib | macOS SDK/system versions | Rendering/compression |

Homebrew formulas are rolling releases. These instructions reproduce the build
procedure; they do not promise byte-identical output when Homebrew or the SDK
changes. For exact environment replication, retain these dependency versions and
the same Apple toolchain. The two downloaded source inputs below are versioned
and checksum-verified.

## 2. Download and verify source dependencies

All downloaded/generated files stay inside the ignored out-of-tree build folder.

```sh
mkdir -p build-macos-arm64/deps

curl -fL https://raw.githubusercontent.com/kazuho/picojson/v1.3.0/picojson.h \
  -o build-macos-arm64/deps/picojson.h

curl -fL https://github.com/SFML/SFML/archive/refs/tags/2.6.2.tar.gz \
  -o build-macos-arm64/deps/sfml-2.6.2.tar.gz

shasum -a 256 -c <<'CHECKSUMS'
5ddf7276d04926da7be243e7af49258e78cc27278ee9097ba45b942c7a6b5f9d  build-macos-arm64/deps/picojson.h
15ff4d608a018f287c6a885db0a2da86ea389e516d2323629e4d4407a7ce047f  build-macos-arm64/deps/sfml-2.6.2.tar.gz
CHECKSUMS

tar -xzf build-macos-arm64/deps/sfml-2.6.2.tar.gz \
  -C build-macos-arm64/deps
```

Stop if either checksum fails.

## 3. Build SFML against OpenAL Soft

Use this SFML build rather than Homebrew's prebuilt `sfml@2`: the prebuilt version
linked Apple's OpenAL, which reproducibly crashed in `OALSource::RemoveBuffersFromQueue`
during music stream cleanup. The same test passed with OpenAL Soft, and Titanic
sinking subsequently passed user testing. SFML's API and source remain unchanged.

```sh
cmake -S build-macos-arm64/deps/SFML-2.6.2 \
  -B build-macos-arm64/deps/sfml-build \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-macos-arm64/deps/sfml-install" \
  -DSFML_BUILD_WINDOW=OFF \
  -DSFML_BUILD_GRAPHICS=OFF \
  -DSFML_BUILD_AUDIO=ON \
  -DSFML_BUILD_NETWORK=ON \
  -DSFML_BUILD_FRAMEWORKS=OFF \
  -DSFML_USE_SYSTEM_DEPS=ON \
  -DOPENAL_INCLUDE_DIR=/opt/homebrew/opt/openal-soft/include/AL \
  -DOPENAL_LIBRARY=/opt/homebrew/opt/openal-soft/lib/libopenal.dylib \
  -DOPENAL_FULL_PATH=/opt/homebrew/opt/openal-soft/lib/libopenal.dylib

cmake --build build-macos-arm64/deps/sfml-build -j 8
cmake --install build-macos-arm64/deps/sfml-build
```

`OPENAL_FULL_PATH` is necessary because SFML 2.6.2's Apple finder otherwise assumes
an OpenAL framework. `CMAKE_POLICY_VERSION_MINIMUM` permits the older SFML CMake
project to configure with CMake 4. No SFML source patch is needed.

## 4. Configure and compile the game

If this checkout has `UserSettings.cmake`, inspect it first: it may override
command-line settings or dependency locations. The tested configuration did not
require that file.

```sh
cmake -S . -B build-macos-arm64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DFS_BUILD_BENCHMARKS=OFF \
  -DFS_USE_STATIC_LIBS=OFF \
  -DwxWidgets_USE_STATIC=OFF \
  -DwxWidgets_CONFIG_EXECUTABLE=/opt/homebrew/bin/wx-config \
  -DPython3_EXECUTABLE=/opt/homebrew/opt/python@3.14/bin/python3.14 \
  -DPICOJSON_DIR="$PWD/build-macos-arm64/deps" \
  -DSFML_DIR="$PWD/build-macos-arm64/deps/sfml-install/lib/cmake/SFML" \
  -DCMAKE_INSTALL_RPATH="$PWD/build-macos-arm64/deps/sfml-install/lib"

cmake --build build-macos-arm64 \
  --target FloatingSandbox ShipBuilder UnitTests -j 8

ctest --test-dir build-macos-arm64 --output-on-failure
```

CTest registers one executable containing 1,014 internal tests. All passed in the
tested environment. The absolute install RPATH supports the flat development
install only; packaging removes it and rewrites all non-system library links.

Optional flat development install:

```sh
cmake --install build-macos-arm64
./build-macos-arm64/Install/FloatingSandbox
```

## 5. Create the self-contained app and clean ZIP

```sh
cmake --build build-macos-arm64 --target macos-package -j 8
```

Outputs:

- `build-macos-arm64/package/Floating Sandbox.app`
- `build-macos-arm64/package/Floating-Sandbox-macOS-arm64.zip`

The package target rebuilds the app, so quit that packaged app before rebuilding.
For the app alone, use target `macos-app`.

The Python packaging script copies the built executable, assets, required legacy
`.dat` ship sidecars, original icon, and discovered native dependencies. It checks
arm64 architecture, resolves the dependency graph, rewrites install names to
bundle-relative paths, removes external RPATHs, and ad-hoc signs the completed app.
The tested package contains 24 native dependency libraries. The app can run without
Homebrew or this checkout. Copy the complete `.app`, not just its executable.

### Recreate only the ZIP from an existing app

```sh
ditto -c -k --norsrc --noextattr --noacl --keepParent \
  "build-macos-arm64/package/Floating Sandbox.app" \
  "build-macos-arm64/package/Floating-Sandbox-macOS-arm64.zip"
```

Do **not** use `--sequesterRsrc`: it deliberately creates the `__MACOSX/` directory.
The flags above omit resource forks, extended attributes, and ACL metadata.
The executable's embedded signature and `Contents/_CodeSignature` are ordinary
file data and remain intact. Source-side `._*`, `.DS_Store`, and thumbnail database
files are excluded when assembling resources.

## 6. Verify archive and extracted app

```sh
python3 - <<'PY'
from pathlib import PurePosixPath
from zipfile import ZipFile
archive = 'build-macos-arm64/package/Floating-Sandbox-macOS-arm64.zip'
with ZipFile(archive) as z:
    unwanted = [n for n in z.namelist()
                if any(p == '__MACOSX' or p == '.DS_Store' or p.startswith('._')
                       for p in PurePosixPath(n).parts)]
    assert not unwanted, unwanted
    assert z.testzip() is None
    print('ZIP verified: no metadata sidecars; CRCs valid')
PY

package_check_dir="$(mktemp -d /tmp/floating-sandbox-package.XXXXXX)"
ditto -x -k "build-macos-arm64/package/Floating-Sandbox-macOS-arm64.zip" \
  "$package_check_dir"
codesign --verify --deep --strict "$package_check_dir/Floating Sandbox.app"
file "$package_check_dir/Floating Sandbox.app/Contents/MacOS/FloatingSandbox"
otool -L "$package_check_dir/Floating Sandbox.app/Contents/MacOS/FloatingSandbox"
open "$package_check_dir/Floating Sandbox.app"
```

The executable should be arm64. Non-system libraries should reference the bundle,
not `/opt/homebrew` or the build directory. Launch from Finder, load Titanic and a
legacy ship such as Varyag, sink a ship with sound, then test ShipBuilder Save As
and return to the simulator. Quit and recheck the signature; normal gameplay and
ship browsing must not modify the bundle.

## 7. Source changes

| File(s) | Change and reason |
| --- | --- |
| `CMakeLists.txt` | Correct `Iconv` casing; request SFML network; accept installed GTest while retaining source-checkout support; guard optional install hook; enable root CTest discovery. |
| `Sources/FloatingSandbox/CMakeLists.txt` | Apple-only app and clean-ZIP targets; Python 3.9+ packaging dependency. |
| `Scripts/package_macos.py` | Reusable standalone bundle builder, native library resolution, relocation, icon, available licenses, metadata exclusions, signing and verification. |
| `Sources/Game/GameAssetManager.cpp` | Resolve `Contents/Resources` for bundled executables; preserve flat install paths. |
| `Sources/FloatingSandbox/BootSettings.h`, `BootSettings.cpp`, `MainApp.cpp`, `UI/BootSettingsDialog.cpp` | Store macOS boot settings in the existing user configuration directory, outside signed resources. |
| `Sources/UILib/ShipPreviewWindow.h`, `ShipPreviewWindow.cpp` | Store macOS preview databases in a user cache keyed by ship directory. |
| `Sources/ShipBuilderLib/OpenGLManager.h`, `View.h`, `View.cpp` | Bind the editor's own GL context before rendering and deleting GL objects. Fixes actual editor-return INVALID_VALUE/crash. |
| `Sources/ShipBuilderLib/MainFrame.cpp` | Release editor resources before hiding its canvas during return to simulator. |
| `Sources/UnitTests/CMakeLists.txt` | Support imported GTest targets; remove duplicate test registration. |
| `Sources/UnitTests/AlgorithmsTests.cpp` | Repair stale ARM fixture signatures, smoothing sample count/helper, and use tolerance for intentionally approximate NEON lighting. |
| `Sources/UnitTests/ImageToolsTests.cpp` | Apply GCC rounding adjustment only to GCC, not Apple Clang. |
| `.gitignore` | Exclude native build outputs and Finder metadata. |
| `MACOS_PORT.md` | Investigation, crash evidence, milestones and test history. |
| `BUILD-macOS.md` | This consolidated source-change and reproduction guide. |


