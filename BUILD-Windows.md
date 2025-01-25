These are instructions on how to build Floating Sandbox 64-bit on Windows. These instructions were written at the time of Floating Sandbox 1.17. Note that I'm mostly a Windows guy, hence some things might be so obvious to me that I might have forgotten to document them here. If you find that this is the case, please let me know and/or update these instructions directly.

# Installing Prerequisite Tooling
Follow these instructions to setup your Windows box with Visual Studio and the necessary SDKs.
### Install Visual Studio
I'm using Visual Studio Professional 2019. You may probably use any other SKU of Visual Studio, but make sure it's at least 2019 as you'll need C++ 17 support.
### Install cmake 3.12 (at least)
Go to https://cmake.org/download/, download the latest Windows package, and install it. Make sure that CMake's `bin` folder is in your PATH afterwards.
### git
You want to install git to get all sources from github. Make sure that `git` is also in your PATH.

# Preparing Prerequisite Libraries
Here we clone and build the libraries required by Floating Sandbox. Some notes:
* We'll build static libraries and link statically, as I prefer one single executable with everything in it over having to distribute a zillion DLLs
* We'll build `Release`, though you may as well build `Debug` or `RelWithDebInfo`

Decide where you want your code to live at; I normally use `%USERPROFILE%\source\repos` for git repos, `%USERPROFILE%\source\build` for build directories, and `%USERPROFILE%\source\SDK` for the libraries' build output. Create these folders, open a command shell, and create these three environment variables which we'll use extensively throughout these instructions:
```
set SOURCE_ROOT="%USERPROFILE%\source\repos"
set BUILD_ROOT="%USERPROFILE%\source\build"
set SDK_ROOT="%USERPROFILE%\source\SDK"
```
## zlib 1.2.11
Get zlib 1.2.11 from https://github.com/wxWidgets/zlib, build it, and make sure to install it under `%SDK_ROOT%\ZLib-1.2.11`.
## LibPNG 1.6.37
Get libpng 1.6.37 from https://github.com/wxWidgets/libpng, build it, and make sure to install it under `%SDK_ROOT%\LibPNG-1.6.37`.
## jpeg
I use the jpeg reference source code from the [Independent JPEG Group (IJG)](https://jpegclub.org/reference/), but you may use other libraries as well. If you want to use the IJG sources, download the 9d version (or any later version) from https://jpegclub.org/reference/reference-sources/, open the Visual Studio solution file, build it (for x64 `Release`), and assemble the output library under `%SDK_ROOT%\jpeg-9d` as follows:
```
%SDK_ROOT%\jpeg-9d:
12/09/2021  07:57 PM    <DIR>          include
01/01/2022  06:11 PM    <DIR>          lib

%SDK_ROOT%\jpeg-9d\include:
07/29/2015  12:44 PM             1,605 jconfig.h
12/02/2018  07:04 PM            14,892 jerror.h
09/17/2013  10:20 AM            15,371 jmorecfg.h
02/03/2019  04:06 PM            50,591 jpeglib.h

%SDK_ROOT%\jpeg-9d\lib:
01/01/2022  06:13 PM    <DIR>          x64

%SDK_ROOT%\jpeg-9d\lib\x64:
01/01/2022  11:52 AM         4,662,216 jpeg.lib
```
## Picojson 1.3.0
Picojson is a handy header-only library, and thus we only need to clone it locally. No build necessary here!
```
cd %SOURCE_ROOT%
git clone https://github.com/kazuho/picojson.git
cd picojson
git checkout v1.3.0
```
## SFML 2.5.1
SFML is a multi-media library. Floating Sandbox uses it mostly for sound support.
### Cloning
```
cd %SOURCE_ROOT%
git clone https://github.com/SFML/SFML.git
cd SFML
git checkout 2.5.1
```
### Building
Build SFML and make sure its output goes into `%SDK_ROOT%\SFML-2.5.1`.
## WxWidgets 3.1.4
Finally, we're gonna build _wxWidgets_, a cross-platform library for windowing and user interfaces. Floating Sandbox uses wxWidgets for the "administrative" UI of the simulator, such as the menu bars and the various dialogs for settings and preferences.
### Cloning
Clone wxWidgets from `https://github.com/wxWidgets/wxWidgets.git`, and checkout the `v3.1.4` tag:
```
cd %SOURCE_ROOT%
git clone https://github.com/wxWidgets/wxWidgets.git
cd wxWidgets
git checkout v3.1.4
```
### Building
Generate the Visual Studio solution file as follows:
```
cd %BUILD_ROOT%
mkdir wxWidgets-3.1.4
cd wxWidgets-3.1.4
cmake %SOURCE_ROOT%\wxWidgets -D wxBUILD_OPTIMISE=ON -D wxBUILD_SHARED=OFF -D wxBUILD_USE_STATIC_RUNTIME=ON -D wxUSE_LIBJPEG=sys -D wxUSE_LIBPNG=sys -D wxUSE_ZLIB=sys -D wxUSE_LIBTIFF=OFF -D wxUSE_EXPAT=OFF -D wxUSE_PNM=OFF -D wxUSE_GIF=OFF -D wxUSE_PCX=OFF -D wxUSE_IFF=OFF -D ZLIB_ROOT=%SDK_ROOT%\ZLib-1.2.11 -D JPEG_ROOT=%SDK_ROOT%\jpeg-9d -D PNG_ROOT=%SDK_ROOT%\LibPNG-1.6.37 -D CMAKE_POLICY_DEFAULT_CMP0074=NEW -D CMAKE_LIBRARY_ARCHITECTURE=x64 -D CMAKE_INSTALL_PREFIX=%SDK_ROOT%\wxWidgets-3.1.4
```
The output of this last command should look like this - after substituting obvious Visual Studio version numbers and library file paths:
```
-- Building for: Visual Studio 16 2019
-- Selecting Windows SDK version 10.0.18362.0 to target Windows 10.0.19043.
-- The C compiler identification is MSVC 19.28.29913.0
-- The CXX compiler identification is MSVC 19.28.29913.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64/cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- cotire 1.8.0 loaded.
-- Found ZLIB: optimized;C:/Users/FSUser/source/SDK/ZLib-1.2.11/lib/x64/zlibstatic.lib;debug;C:/Users/FSUser/source/SDK/ZLib-1.2.11/lib/x64/zlibstaticd.lib (found version "1.2.11")
-- Found JPEG: optimized;C:/Users/FSUser/source/SDK/jpeg-9d/lib/x64/jpeg.lib;debug;C:/Users/FSUser/source/SDK/jpeg-9d/lib/x64/jpegd.lib (found version "90")
-- Found PNG: optimized;C:/Users/FSUser/source/SDK/LibPNG-1.6.37/lib/x64/libpng16_static.lib;debug;C:/Users/FSUser/source/SDK/LibPNG-1.6.37/lib/x64/libpng16_staticd.lib (found version "1.6.37")
...
-- Check size of unsigned short
-- Check size of unsigned short - done
-- Searching 16 bit integer - Using unsigned short
-- Check if the system is big endian - little endian
-- Which libraries should wxWidgets use?
    wxUSE_STL:      OFF      (use C++ STL classes)
    wxUSE_REGEX:    builtin  (enable support for wxRegEx class)
    wxUSE_ZLIB:     sys      (use zlib for LZW compression)
    wxUSE_EXPAT:    OFF      (use expat for XML parsing)
    wxUSE_LIBJPEG:  sys      (use libjpeg (JPEG file format))
    wxUSE_LIBPNG:   sys      (use libpng (PNG image format))
    wxUSE_LIBTIFF:  OFF      (use libtiff (TIFF file format))
    wxUSE_LIBLZMA:  OFF      (use liblzma for LZMA compression)

-- Configured wxWidgets 3.1.4 for Windows-10.0.19043
    Min OS Version required at runtime:                Windows Vista / Windows Server 2008 (x64 Edition)
    Which GUI toolkit should wxWidgets use?            msw
    Should wxWidgets be compiled into single library?  OFF
    Should wxWidgets be linked as a shared library?    OFF
    Should wxWidgets support Unicode?                  ON
    What wxWidgets compatibility level should be used? 3.0
-- Configuring done
-- Generating done
-- Build files have been written to: C:/Users/FSUser/source/build/wxWidgets-3.1.4
```
Now it's time to build wxWidgets. Open the solution in Visual Studio, select the `Release` configuration, and start a build for the `INSTALL` project. Do not go for a coffee yet as this build is gonna be quite quick.
After the build is complete and installed, you should see the following under your new `%SDK_ROOT%\wxWidgets-3.1.4` directory:
```
01/01/2022  06:31 PM    <DIR>          .
01/01/2022  06:31 PM    <DIR>          ..
01/01/2022  06:31 PM    <DIR>          include
01/01/2022  06:31 PM    <DIR>          lib
```
## GoogleTest
We also need GoogleTest, for running Floating Sandbox's unit tests. We won't build it, as GoogleTest is best built together with the unit test's sources.
### Cloning
```
cd %SOURCE_ROOT%
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.10.0
```
# Building Floating Sandbox
Now that all static libraries and 3-rd party repo's are ready, it's time to _finally_ build Floating Sandbox. We are going to clone and build `master`.
### Cloning
```
cd %SOURCE_ROOT%
git clone https://github.com/GabrieleGiuseppini/Floating-Sandbox.git
```
### Configuring
Before we build, we must tell Floating Sandbox where to find all of the libraries. To this end you'll have to craft a `UserSettings.cmake` file in Floating Sandbox's root directory, which populates variables containing the paths of all of the roots of the libraries we've been building so far.
Luckily you're going to find a pre-cooked `UserSettings.example-windows.cmake` file in the root of the repo which, if you've been following these instructions verbatim, is ready for use; in that case, copy it as `UserSettings.cmake`:
```
cd %SOURCE_ROOT%\Floating-Sandbox
copy UserSettings.example-windows.cmake UserSettings.cmake
```
On the other hand, if you've customized paths of checkouts and library roots, just make your own `UserSettings.cmake`, eventually using `UserSettings.example-windows.cmake` as a template.
### Building
We're gonna build Floating Sandbox in a folder named `build` under its checkout root, and make it install under `%USERPROFILE%\floating-sandbox`. Note that the `INSTALL` target will create the whole directory structure as expected by the simulator, including all resource and ship files.

Create the Visual Studio solution file as follows:
```
cd %SOURCE_ROOT%\Floating-Sandbox
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=Release -D FS_BUILD_BENCHMARKS=OFF -D FS_USE_STATIC_LIBS=ON -D FS_INSTALL_DIRECTORY=%USERPROFILE%\floating-sandbox ..
```
The output of this last command should look like this - after substituting obvious Visual Studio version numbers and library file paths:
```
-- Building for: Visual Studio 16 2019
-- Selecting Windows SDK version 10.0.18362.0 to target Windows 10.0.19043.
-- The C compiler identification is MSVC 19.28.29913.0
-- The CXX compiler identification is MSVC 19.28.29913.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64/cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29910/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Loading user specific settings from C:/Users/FSUser/source/repos/Floating-Sandbox/UserSettings.cmake
-- FS_USE_STATIC_LIBS:ON
-- FS_BUILD_BENCHMARKS:OFF
-- Found PicoJSON: C:/Users/FSUser/source/SDK/PicoJSON
-- Found ZLIB: optimized;C:/Users/FSUser/source/SDK/ZLib-1.2.11/lib/x64/zlibstatic.lib;debug;C:/Users/FSUser/source/SDK/ZLib-1.2.11/lib/x64/zlibstaticd.lib (found version "1.2.11")
-- Found JPEG: optimized;C:/Users/FSUser/source/SDK/jpeg-9d/lib/x64/jpeg.lib;debug;C:/Users/FSUser/source/SDK/jpeg-9d/lib/x64/jpegd.lib (found version "90")
-- Found PNG: optimized;C:/Users/FSUser/source/SDK/LibPNG-1.6.37/lib/x64/libpng16_static.lib;debug;C:/Users/FSUser/source/SDK/LibPNG-1.6.37/lib/x64/libpng16_staticd.lib (found version "1.6.37")
-- wxWidgets_ROOT:C:/Users/FSUser/source/SDK/wxWidgets-3.1.4
-- Found wxWidgets: debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxbase31ud.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxbase31u.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_gl.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_gl.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_core.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_core.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_html.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_html.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_propgrid.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_propgrid.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxregexud.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxregexu.lib;opengl32;glu32;winmm;comctl32;oleacc;rpcrt4;shlwapi;version;wsock32
-- SFML_ROOT:C:/Users/FSUser/source/SDK/SFML-2.5.1/lib/cmake/SFML
-- Found SFML 2.5.1 in C:/Users/FSUser/source/SDK/SFML-2.5.1/lib/cmake/SFML
-- Found OpenGL: opengl32
-- Found PythonInterp: C:/Users/FSUser/AppData/Local/Programs/Python/Python38/python.exe (found version "3.8.5")
-- Looking for pthread.h
-- Looking for pthread.h - not found
-- Found Threads: TRUE
-- Copying data files and runtime files for FloatingSandbox...
-- Copying data files for GPUCalcTest...
-- Copying data files for ShipBuilder...
-- -----------------------------------------------------
-- cxx Flags:/DWIN32 /D_WINDOWS  /GR /EHsc /D_CRT_SECURE_NO_WARNINGS /W4 /w44062
-- cxx Flags Release:/MT  /Ob2 /DNDEBUG /W4 /w44062 /fp:fast /Ox /GS- /MP
-- cxx Flags RelWithDebInfo:/MT /Zi /O2 /Ob1 /DNDEBUG /W4 /w44062 /fp:fast /Ox /GS- /MP
-- cxx Flags Debug:/MTd /Zi /Ob0 /Od /RTC1 /W4 /w44062 /fp:strict /DFLOATING_POINT_CHECKS /MP
-- c Flags:/DWIN32 /D_WINDOWS  /W4 /w44062
-- c Flags Release:/MT /O2 /Ob2 /DNDEBUG /W4 /w44062
-- c Flags RelWithDebInfo:/MT /Zi /O2 /Ob1 /DNDEBUG /W4 /w44062
-- c Flags Debug:/MTd /Zi /Ob0 /Od /RTC1 /W4 /w44062
-- exe Linker Flags Release: /INCREMENTAL:NO /LTCG
-- exe Linker Flags RelWithDebInfo:/debug  /INCREMENTAL:NO /LTCG:incremental
-- exe Linker Flags Debug:/debug  /INCREMENTAL:NO
-- wxWidgets libraries:debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxbase31ud.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxbase31u.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_gl.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_gl.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_core.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_core.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_html.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_html.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31ud_propgrid.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxmsw31u_propgrid.lib;debug;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxregexud.lib;optimized;C:/Users/FSUser/source/SDK/wxWidgets-3.1.4/lib/vc_x64_lib/wxregexu.lib;opengl32;glu32;winmm;comctl32;oleacc;rpcrt4;shlwapi;version;wsock32
-- Additional libraries:comctl32;rpcrt4;advapi32
-- Install directory:C:/Users/FSUser/floating-sandbox
-- -----------------------------------------------------
-- Configuring done
-- Generating done
-- Build files have been written to: C:/Users/FSUser/source/Floating-Sandbox/build
```
Open the solution in Visual Studio, select the `Release` configuration, and start a build for the `INSTALL` project. Now it's time for that cup of coffee.
### Running
When the build is complete you should have the game neatly laid out under your `%USERPROFILE%\floating-sandbox` directory:
```
C:\Users\FSUser\floating-sandbox>dir
 Volume in drive C is OS
 Volume Serial Number is 383F-C47B

 Directory of C:\Users\FSUser\floating-sandbox

01/02/2022  11:46 AM    <DIR>          .
01/02/2022  11:46 AM    <DIR>          ..
12/23/2021  06:07 PM            22,530 changes.txt
01/02/2022  11:46 AM    <DIR>          Data
01/02/2022  11:44 AM        25,792,512 FloatingSandbox.exe
01/02/2022  11:46 AM    <DIR>          Guides
03/07/2021  05:30 PM             1,270 license.txt
04/10/2019  03:30 PM           669,696 openal32.dll
01/01/2022  07:09 PM             8,446 README.md
05/30/2019  09:49 PM         1,287,680 sfml-audio-2.dll
05/30/2019  09:51 PM           421,888 sfml-network-2.dll
05/30/2019  09:48 PM           255,488 sfml-system-2.dll
01/02/2022  11:46 AM    <DIR>          Ships
               8 File(s)     28,459,510 bytes
               5 Dir(s)  725,495,750,656 bytes free
```
To start the game, go into that directory and launch `FloatingSandbox.exe`. 

Enjoy!
