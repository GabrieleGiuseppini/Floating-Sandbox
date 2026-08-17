These are instructions on how to build Floating Sandbox on macOS. They were written at the time of Floating Sandbox 1.21.0 and verified on macOS 26 ("Tahoe") on Apple Silicon (arm64), using [Homebrew](https://brew.sh/) for the prerequisite libraries.

macOS support is community-contributed and not part of the maintainer's regular build matrix. The original macOS build recipe was worked out by [@matschaffer](https://gist.github.com/matschaffer/87220798c79a5d0ace1eaa9afb729d28); these instructions update that work for the current dependency set (Floating Sandbox no longer depends on DevIL, and SFML is now taken from Homebrew).

# Installing Prerequisite Tooling and SDKs

### Xcode Command Line Tools
The C/C++ toolchain (`clang`, `git`, the macOS SDK) is provided by Apple's Command Line Tools:
```
xcode-select --install
```

### Homebrew
If you don't already have it, install Homebrew by following the instructions at https://brew.sh/.

### Build tools and libraries
```
brew install cmake sfml@2 wxwidgets jpeg-turbo libpng openal-soft
```
Notes:
* `sfml@2` is required. Floating Sandbox uses the SFML 2.x API; the current `sfml` formula (3.x) has an incompatible API. `sfml@2` is keg-only — the example settings file below accounts for that.
* `wxwidgets` installs 3.3 at the time of writing, which builds fine.
* `zlib` is supplied by the macOS SDK; no separate package is needed.
* `openal-soft` is not a build dependency — it is needed to work around an audio crash at runtime. See [Audio: OpenAL](#audio-openal) below.

# Preparing Prerequisite Libraries

As on the other platforms, we clone two source-only dependencies. Here we use `~/git`; change it as you like (and adjust `UserSettings.cmake` accordingly).

### PicoJSON
Picojson is a header-only library, so cloning it is enough — no build necessary.
```
cd ~/git
git clone https://github.com/kazuho/picojson.git
```

### GoogleTest
GoogleTest is built together with Floating Sandbox's unit tests, so we only clone it.
```
cd ~/git
git clone https://github.com/google/googletest.git
```

# Building Floating Sandbox

### Cloning
```
cd ~/git
git clone https://github.com/GabrieleGiuseppini/Floating-Sandbox.git
```

### Configuring
Before building, tell Floating Sandbox where to find the libraries by creating a `UserSettings.cmake` file in the repo root. A ready-made template for macOS is provided:
```
cd ~/git/Floating-Sandbox
cp UserSettings.example-macos.cmake UserSettings.cmake
```
If you cloned picojson/googletest somewhere other than `~/git`, or your Homebrew prefix is not `/opt/homebrew` (Intel Macs use `/usr/local`), edit the paths in `UserSettings.cmake` accordingly.

### Building
We build in a `build` folder under the checkout root, and install under `~/floating-sandbox`:
```
cd ~/git/Floating-Sandbox
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DFS_BUILD_BENCHMARKS=OFF -DFS_USE_STATIC_LIBS=OFF -DFS_INSTALL_DIRECTORY=~/floating-sandbox ..
cmake --build . --target FloatingSandbox ShipBuilder -j
cmake --install .
```
Note: the `FloatingSandbox` (game) and `ShipBuilder` (ship editor) targets are built explicitly. Building everything (plain `cmake --build .`) also builds the `UnitTests` target, which at the time of writing does not compile on recent Clang.

### Running
The `install` step lays the game out under `~/floating-sandbox`, including all resource and ship files. To start it:
```
cd ~/floating-sandbox
./FloatingSandbox
```

# Audio: OpenAL

Homebrew's `sfml@2` bottle links its audio library (`libsfml-audio`) against Apple's system `OpenAL.framework`. Apple deprecated OpenAL in 2019 and no longer maintains it; on recent macOS it crashes — `EXC_BAD_ACCESS` (null dereference) in `OALSource::RemoveBuffersFromQueue`, on SFML's `SoundStream` thread — a short while into play.

The fix is to use [`openal-soft`](https://github.com/kcat/openal-soft) instead — the actively-maintained, ABI-compatible OpenAL implementation that SFML already uses on Windows and Linux. Because `sfml@2` is a prebuilt bottle, relink its audio dylib to point at openal-soft:
```
install_name_tool -change \
  /System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL \
  $(brew --prefix openal-soft)/lib/libopenal.1.dylib \
  $(brew --prefix sfml@2)/lib/libsfml-audio.2.6.2.dylib
```
(Adjust the `libsfml-audio` version to match what `sfml@2` installed.)

If you redistribute the game as a self-contained `.app` bundle, instead copy `libopenal.1.dylib` in alongside the other bundled dylibs, give it an `@executable_path`-relative install name, point `libsfml-audio` at it with the same `install_name_tool -change`, and re-sign the bundle afterwards.

Alternatively, build SFML itself from source against `openal-soft` rather than using the Homebrew bottle.

Enjoy!
