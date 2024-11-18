These are instructions on how to build Floating Sandbox on a *clean* Ubuntu 20.04. These instructions were written at the time of Floating Sandbox 1.19.0, and tested on a completely clean VM. Note that I'm writing this mostly for myself, as I'm a Linux newbie (I'm a hardcore Windows developer), so forgive the verbosity.

# Installing Prerequisite Tooling and SDKs

Follow these instructions to setup your Ubuntu with development tools and the necessary SDKs. You may skip any steps when you already have the indicated versions.

### Prepare APT
```
sudo apt update
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
```
### gcc 8.4.0 (at least)
```
sudo apt install gcc-8
sudo apt install g++-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 80 --slave /usr/bin/g++ g++ /usr/bin/g++-8
```
Check the installed versions now:
```
gcc --version
g++ --version
```
### cmake 3.12 (at least)
If the `cmake` package for your Ubuntu version is later than or equal 3.12, just do the following:
```
sudo apt install cmake
```
Otherwise, if you're running an older Ubuntu whose `cmake` package is earlier than 3.12 (Ubuntu 18.04 comes with cmake 3.10 for example), then follow these instructions.

First of all, go to https://cmake.org/download/ and download the latest `Unix/Linux Source` tar package - at the time of writing, that would be `cmake-3.20.2.tar.gz`.
Unpack the tar, go into its output directory, and build and install it as follows:
```
sudo apt install libssl-dev
./configure
make
sudo make install
```
The install step will copy cmake under `/usr/local/bin`. Verify your cmake version as follows:
```
hash -r
cmake --version
```
### git
```
sudo apt install git
```
### X11 SDK
```
sudo apt install libx11-dev
```
### GTK3 SDK
```
sudo apt install libgtk-3-dev
```
### Vorbis SDK
```
sudo apt install libvorbis-dev
```
### FLAC SDK
```
sudo apt install libflac++-dev
```
### OpenGL SDK
```
sudo apt install libgl1-mesa-dev
sudo apt install libglu1-mesa-dev
```
### zlib SDK
```
sudo apt install zlib1g-dev
```
### libpng SDK
```
sudo apt install libpng-dev
```
### libjpeg SDK
```
sudo apt install libjpeg-dev
```
### SFML SDK
SFML is a multi-media library. Floating Sandbox uses it mostly for sound support. We're going to need version 2.5.
```
sudo apt install libsfml-dev
```
# Preparing Prerequisite Libraries
Here we clone and build the libraries required by Floating Sandbox. Some notes:
* In these instructions we'll be cloning all library sources under `~/git`; change as you like
* We'll build static libraries and link statically, as I prefer one single executable with everything in it, over depending on carefully prepared target environments
* We'll build `Release`, though you may as well build `Debug` or `RelWithDebInfo`
* Finally, we'll install all library binaries and include files under `~/fs_libs`

## Picojson 1.3.0
Picojson is a handy header-only library, and thus we only need to clone it locally. No build necessary here!
```
cd ~/git
git clone https://github.com/kazuho/picojson.git
cd picojson
git checkout v1.3.0
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
Before we can build, we need to apply a patch to fix [an issue in DevIL with building static libraries](https://github.com/DentonW/DevIL/issues/95). Since DevIL seems pretty much dead, I couldn't get my patch to the repo and thus you'll be better off applying the `devil-issue-95.patch` patch - which you can find in the root of the Floating Sandbox repo - to your checkout:
```
cd ~/git/DevIL
git apply devil-issue-95.patch
```
We are now ready to build DevIL in a folder named `build` under its checkout root.
```
cd ~/git/DevIL
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DIL_NO_TIF=1 -DIL_NO_JP2=1 -DIL_USE_DXTC_SQUISH=0 -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=~/fs_libs/DevIL ../DevIL 
make install
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/DevIL` directory:
```
drwxrwxr-x 3 gg gg 4096 mei 22 17:52 include/
drwxrwxr-x 3 gg gg 4096 mei 22 17:52 lib/
```
## WxWidgets 3.1.4
Finally, we're gonna build _wxWidgets_, a cross-platform library for windowing and user interfaces. Floating Sandbox uses wxWidgets for the "administrative" UI of the simulator, such as the menu bars and the various dialogs for settings and preferences.
### Cloning
```
cd ~/git
git clone --recurse-submodules https://github.com/wxWidgets/wxWidgets.git
cd wxWidgets
git checkout v3.1.4
```
### Building
We're going to build wxWidgets in a folder named `my_wx_build` under its checkout root.

First of all, we're going to configure the build for GTK, with UNICODE and OpenGL support, stating that we only want static linking:
```
cd ~/git/wxWidgets
mkdir my_wx_build
cd my_wx_build
../configure --disable-shared --with-gtk=3 --with-libpng --with-libxpm --with-libjpeg --without-libtiff --without-expat --disable-pnm --disable-gif --disable-pcx --disable-iff --with-opengl --prefix=${HOME}/fs_libs/wxWidgets --exec_prefix=${HOME}/fs_libs/wxWidgets --disable-tests --disable-rpath
```
The output of the last `configure` step should look like this:
```
Configured wxWidgets 3.1.4 for `x86_64-pc-linux-gnu'

  Which GUI toolkit should wxWidgets use?                 GTK+ 3 with support for GTK+ printing
  Should wxWidgets be compiled into single library?       no
  Should wxWidgets be linked as a shared library?         no
  Should wxWidgets support Unicode?                       yes (using wchar_t)
  What level of wxWidgets compatibility should be enabled?
                                       wxWidgets 2.8      no
                                       wxWidgets 3.0      yes
  Which libraries should wxWidgets use?
                                       STL                no
                                       jpeg               sys
                                       png                sys
                                       regex              builtin
                                       tiff               no
                                       lzma               no
                                       zlib               sys
                                       expat              no
                                       libmspack          no
                                       sdl                no
```
Now, it's time to build wxWidgets - launch this and go grab a cup of coffee:
```
make install -j$(nproc)
```
After the build is complete and installed, you should see the following under your new `~/fs_libs/wxWidgets` directory:
```
drwxrwxr-x 2 gg gg 4096 mei 23 12:54 bin/
drwxrwxr-x 3 gg gg 4096 mei 23 12:54 include/
drwxrwxr-x 3 gg gg 4096 mei 23 12:54 lib/
drwxrwxr-x 5 gg gg 4096 mei 23 12:54 share/
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
Now that all static libraries and 3-rd party repo's are ready, it's time to _finally_ build Floating Sandbox. We are going to clone and build `master`, though this might be risky as you might catch some latest commits that break gcc (I do my daily dev work with Visual Studio and only check gcc build-ability every so often). If this happens to you, feel free to create a ticket for me on github.
### Cloning
```
cd ~/git
git clone https://github.com/GabrieleGiuseppini/Floating-Sandbox.git
```
### Configuring
Before we build, we must tell Floating Sandbox where to find all of the libraries. To this end you'll have to craft a `UserSettings.cmake` file in Floating Sandbox's root directory, which populates variables containing the paths of all of the roots of the libraries we've been building so far.
Luckily you're going to find a pre-cooked `UserSettings.example-linux.cmake` file in the root of the repo which, if you've been following these instructions verbatim, is ready for use; in that case, copy it as `UserSettings.cmake`:
```
cd ~/git/Floating-Sandbox
cp UserSettings.example-linux.cmake UserSettings.cmake
```
Remember to change the user name in the file to reflect your home folder.
On the other hand, if you've customized paths of checkouts and library roots, just make your own `UserSettings.cmake`, eventually using `UserSettings.example-linux.cmake` as a template.
### Building
We're gonna build Floating Sandbox in a folder named `build` under its checkout root, and make it install under `~/floating-sandbox`. Note that the `INSTALL` target will create the whole directory structure as expected by the simulator, including all resource and ship files.
```
cd ~/git/Floating-Sandbox
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DFS_BUILD_BENCHMARKS=OFF -DFS_USE_STATIC_LIBS=ON -DwxWidgets_USE_DEBUG=OFF -DwxWidgets_USE_UNICODE=ON -DwxWidgets_USE_STATIC=ON -DFS_INSTALL_DIRECTORY=~/floating-sandbox ..
make install -j$(nproc)
```
### Running
At this moment you should have the game neatly laid out under your `~/floating-sandbox` directory:
```
gg@ubuntu1804-vm:~$ ll ~/floating-sandbox/
total 183572
drwxrwxr-x  5 gg gg      4096 mei 23 15:57 ./
drwxr-xr-x 18 gg gg      4096 mei 23 15:57 ../
-rw-r--r--  1 gg gg     20235 mei 23 11:06 changes.txt
drwxr-xr-x 13 gg gg      4096 mei 23 15:57 Data/
-rwxr-xr-x  1 gg gg 187856824 mei 23 15:44 FloatingSandbox*
drwxr-xr-x  2 gg gg      4096 mei 23 15:57 Guides/
-rw-r--r--  1 gg gg      1270 mei 22 14:25 license.txt
-rw-r--r--  1 gg gg     12903 mei 23 14:43 README.md
drwxr-xr-x  2 gg gg     57344 mei 23 15:57 Ships/
```
To start the game, go into that directory and launch `FloatingSandbox`. 

Note that many Linux distributions nowadays use Wayland for their desktop environments, and Floating Sandbox will sometimes encounter an error when launching. To rectify this, set the environment variable "GDK_BACKEND" to "x11":
```
export GDK_BACKEND=x11
```

Enjoy!
