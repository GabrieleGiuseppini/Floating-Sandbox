These are instructions on how to build Floating Sandbox on Ubuntu 18.04. These instructions were written at the time of Floating Sandbox 1.16.4.

# Installing Prerequisite Tooling

Follow these instructions to setup your Ubuntu with development tools and the necessary SDKs.

## gcc 8.4.0
The following is the relevant excerpt from https://linuxize.com/post/how-to-install-gcc-compiler-on-ubuntu-18-04/:
```
sudo apt update
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-8
sudo apt install g++-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 80 --slave /usr/bin/g++ g++ /usr/bin/g++-8
gcc --version
g++ --version
```
## cmake 3.10.2
```
sudo apt install cmake
cmake --version
```
## git
```
sudo apt install git
```
## X11 SDK
```
sudo apt install libx11-dev
```
## GTK3 SDK
```
sudo apt-get install libgtk-3-dev
```
## Vorbis SDK
```
sudo apt-get install libvorbis-dev
```
## FLAC SDK
```
sudo apt-get install libflac++-dev
```
# Preparing Prerequisite Libraries

Here we clone and build the libraries required by Floating Sandbox. Some notes:
* In these instructions we'll be cloning all library sources under `~/git`; change as you like
* We'll build static libraries and link statically, as I prefer one single executable with everything in it, over depending on carefully prepared target environments
* We'll build `RelWithDebInfo`, though you may as well build `Debug` or `Release`
* Finally, we'll install all library binaries and include files under `~/fs_libs`

## Picojson 1.3.0
Picojson is a handy header-only library, and thus we only need to clone it locally. No build necessary here!
```
cd ~/git
git clone https://github.com/kazuho/picojson.git
cd picojson
git checkout v1.3.0
```

## zlib 1.2.11
This library is required by a bit of everything. It's the master library that implements compression, and it's used by so many image formats.
### Cloning
```
cd ~/git
git clone https://github.com/madler/zlib.git
cd zlib
git checkout v1.2.11
```
### Building
We're going to build `zlib` in a folder named `build` under its checkout root, and we're going to ask it to install its library and header files under `~/fs_libs/zlib`.
```
cd ~/git/zlib
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=~/fs_libs/zlib ..
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/zlib` directory:
```
drwxrwxr-x 2 gg gg 4096 mei 22 17:03 include/
drwxrwxr-x 2 gg gg 4096 mei 22 17:03 lib/
drwxrwxr-x 4 gg gg 4096 mei 22 17:03 share/
```
## libpng 1.6.37
This library implements support for the PNG image format. Luckily this is the only image format supported by Floating Sandbox (it's a nice, compact *lossless* format) and thus it's the only image format library we'll ever have to tinker with.
### Cloning
```
cd ~/git
git clone https://github.com/glennrp/libpng
cd libpng
git checkout v1.6.37
```
### Building
We're going to build `libpng` in a folder named `build` under its checkout root, and we're going to ask it to install its library and header files under `~/fs_libs/libpng.
```
cd ~/git/libpng
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPNG_STATIC=ON -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DCMAKE_PREFIX_PATH=~/fs_libs/zlib  -DCMAKE_INSTALL_PREFIX=~/fs_libs/libpng ..
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/libpng directory:
```
drwxrwxr-x 2 gg gg 4096 mei 22 17:22 bin/
drwxrwxr-x 3 gg gg 4096 mei 22 17:22 include/
drwxrwxr-x 4 gg gg 4096 mei 22 17:22 lib/
drwxrwxr-x 3 gg gg 4096 mei 22 17:22 share/
```
## DevIL 1.8.0
DevIL is a cross-platform image manipulation library. We'll need to clone it _and_ build it as a static library.
### Cloning
```
cd ~/git
git clone https://github.com/DentonW/DevIL
cd DevIL
git checkout v1.8.0
```
### Building
We're gonna build DevIL in a folder named `build` under its checkout root.
```
cd ~/git/DevIL
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DZLIB_LIBRARY=~/fs_libs/zlib/lib -DZLIB_INCLUDE_DIR=~/fs_libs/zlib/include -DPNG_LIBRARY=~/fs_libs/libpng/lib -DPNG_PNG_INCLUDE_DIR=~/fs_libs/libpng/include -DCMAKE_INSTALL_PREFIX=~/fs_libs/DevIL ../DevIL 
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/DevIL` directory:
```
drwxrwxr-x 3 gg gg 4096 mei 22 17:52 include/
drwxrwxr-x 3 gg gg 4096 mei 22 17:52 lib/
```
## SFML 2.5.1
SFML is a multi-media library. Floating Sandbox uses it mostly for sound support.
### Cloning
```
cd ~/git
git clone https://github.com/SFML/SFML.git
cd SFML
git checkout 2.5.1
```
### Building
We're gonna build SFML in a folder named `build` under its checkout root.
```
cd ~/git/SFML
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=FALSE -DSFML_BUILD_WINDOW=FALSE -DSFML_BUILD_GRAPHICS=FALSE -DSFML_BUILD_DOC=FALSE -DSFML_BUILD_EXAMPLES=FALSE -DCMAKE_INSTALL_PREFIX=~/fs_libs/SFML ..
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/SFML` directory:
```
drwxr-xr-x 3 gg gg 4096 mei 22 18:39 include/
drwxrwxr-x 4 gg gg 4096 mei 22 18:39 lib/
drwxrwxr-x 3 gg gg 4096 mei 22 18:39 share/
```
## WxWidgets 3.1.4
Finally, we're gonna build _wxWidgets_, a cross-platform library for windowing and user interfaces. Floating Sandbox uses wxWidgets for the "administrative" UI of the simulator, such as the menu bars and the various dialogs for settings and preferences.
### Cloning
```
cd ~/git
git clone https://github.com/wxWidgets/wxWidgets.git
cd wxWidgets
git checkout v3.1.4
```
### Building
We're gonna build wxWidgets in a folder named `my_wx_build` under its checkout root. Note also that we want to build with UNICODE support, and with support for the PNG image format.
```
cd ~/git/wxWidgets
mkdir my_wx_build
cd my_wx_build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DwxBUILD_SAMPLES=OFF -DwxBUILD_TESTS=OFF -DwxBUILD_DEMOS=OFF -DwxUSE_LIBJPEG=OFF -DwxUSE_UNICODE=ON -DwxBUILD_SHARED=OFF -DCMAKE_INSTALL_PREFIX=~/fs_libs/wxWidgets ..
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/wxWidgets` directory:
```
drwxr-xr-x 2 gg gg 4096 mei 22 19:09 bin/
drwxrwxr-x 3 gg gg 4096 mei 22 19:09 include/
drwxrwxr-x 3 gg gg 4096 mei 22 19:09 lib/
```
## GoogleTest
We also need GoogleTest, for running Floating Sandbox's unit tests. We won't build it, as GoogleTest is best built together with the unit test's sources.
### Cloning
```
cd ~/git
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.10.0
```
# Building Floating Sandbox
Now that all static libraries and 3-rd party repo's are ready, it's time to build Floating Sandbox. We are going to clone and build `master`, though this might be risky as you might catch some late commits that break compilation in gcc. If this happens to you, feel free to create a ticket for me on github.
### Cloning
```
cd ~/git
git clone https://github.com/GabrieleGiuseppini/Floating-Sandbox.git
```
### Configuring
Before we build, we must tell Floating Sandbox where to find all of the libraries. To this end you'll have to craft a `UserSettings.cmake` file in Floating Sandbox's root directory, which populates variables containing the paths of all of the roots of the libraries we've been building so far.
Luckily you're going to find a pre-cooked `UserSettings.example-linux.cmake` file in the root of the repo which, if yuo've been following these instructions verbatim, is ready for use; in that case, copy it as `UserSettings.cmake`:
```
cd ~/git/Floating-Sandbox
cp UserSettings.example-linux.cmake UserSettings.cmake
```
On the other hand, if you've customized paths of checkouts and library roots, just make your own `UserSettings.cmake`, eventually using `UserSettings.example-linux.cmake` as a template.
### Building
We're gonna build Floating Sandbox in a folder named `build` under its checkout root, and make it install under `~/floating-sandbox`. Note that the `INSTALL` target will create the whole directory structure as expected by the simulator, including all resource and ship files.
```
cd ~/git/Floating-Sandbox
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DFS_BUILD_BENCHMARKS=OFF -DFS_USE_STATIC_LIBS=ON -DCMAKE_INSTALL_PREFIX=~/floating-sandbox ..
TODOHERE
```



TODO:
* Redo as:
	** RELEASE
	** Without libpng and zlib (which are already installed by gtk3)
	** Less wxWidgets libs (we only need: base gl core html media)
* Link to this from main Readme
* Update Readme for "I'm building on Ubuntu" and OSes
* Push patch for DevIL
