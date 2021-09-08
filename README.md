# Floating Sandbox
A two-dimensional particle system in C++, simulating physical bodies floating in water and sinking.

# Overview
This game is a realistic two-dimensional particle system, which uses mass-spring networks to simulate rigid bodies (such as ships) floating in water and exchanging heat with their surroundings. You can punch holes in an object, slice it, apply forces and heat to it, set it on fire, and smash it with bomb explosions. Once water starts rushing inside the object, it will start sinking and, if you're lucky enough, it will start breaking up!

<img src="https://i.imgur.com/c8fTsgY.png">

The game is really a generic physics simulator that can be used to simulate just about any 2D floating rigid body under stress.

As of now the simulator implements the following aspects of physics:
- Classical mechanics - Hookean laws for springs, impacts
- Thermodynamics - heat transfer, dissipation, combustion
- Fluid dynamics - buoyancy, drag, hydrostatic and atmospheric pressure
- Basic electrotechnics - conductivity

<img src="https://i.imgur.com/kovxCty.png">
<img src="https://i.imgur.com/XHw3Jrl.png">

You can create your own physical objects by drawing images using colors that correspond to materials in the game's library. Each material has its own physical properties, such as mass, strength, stiffness, water permeability, specific heat, sound properties, and so on. The game comes with a handy template image that you can use to quickly select the right materials for your object.

If you want, you can also apply a higher-resolution image to be used as a more realistic texture for the object. Once you load your object, watch it float and explore how it behaves under stress!

For the physics in the simulation I'm trying to shy away from tricks that exist solely for the purpose of delivering an eye-candy; every bit is grounded as close as possible into real physics, and the material system has been put together using physical attributes of real-world materials. This makes it sometimes hard to build structures that sustain their own weight or float easily - as it is in reality, after all - but the reward is a realistic world-in-a-sandbox where every action and corresponding reaction are not pre-programmed but, rather, are generated automatically by the physics engine calculations.

The game currently comes with a few example objects - mostly ships - and I'm always busy making new ships and objects. Anyone is encouraged to make their own objects, and if you'd like them to be included in the game, just get in touch with me - you'll get proper recognition in the About dialog, of course.

There are lots of improvements that I'm currently working on; some of these are:
- In-game Ship Builder
- Improved rigid body simulation algorithm
- Splashes originating from collisions with water
- Smoke from fire
- Multiple ships and collision detection among parts of the ships
- Ocean floor getting dents upon impact
- NPC's that move freely within ships

These and other ideas will come out with frequent releases.

The game is also featured at [GameJolt](https://gamejolt.com/games/floating-sandbox/353572), and plenty of videos may be found on Youtube.

# System Requirements
- Windows:
	- Windows 7, 8, or 10, either 64-bit or 32-bit
		- The 64-bit build of Floating Sandbox runs ~7% faster than the 32-bit build, so if you're running a 64-bit Windows it is advisable to install the 64-bit build of Floating Sandbox
	- OpenGL 2.1 or later
		- If your graphics card does not support OpenGL 2.1, try upgrading its drivers - most likely there's a newer version with support for 2.1
- Linux:
	- Either 64-bit or 32-bit
	- X11 and GTK3
	- OpenGL 2.1 or later, MESA drivers are fine
	- OpenAL, Vorbis and FLAC

<img src="https://i.imgur.com/6LOVsqX.jpg">

# History
I started coding this game after stumbling upon Luke Wren's and Francis Racicot's (Pac0master) [Ship Sandbox](https://github.com/Wren6991/Ship-Sandbox). After becoming fascinated by it, I [forked](https://github.com/GabrieleGiuseppini/Ship-Sandbox) Luke's GitHub repo and started playing with the source code. After less than a year I realized I had rewritten all of the original code, while improving the game's FPS rate from 7 to 30 (on my 2009 laptop!). At this moment I decided that my new project was worthy of a new name and a new source code repository, the one you are looking at now.

# Performance Characteristics
The bottleneck at the moment is the spring relaxation algorithm, which requires about 80% of the time spent for the simulation of each single frame. I have an alternative version of the same algorithm written with intrinsics in the Benchmarks project, which shows a 20%-27% perf improvement. Sooner or later I'll integrate that in the game, but it's not gonna be a...game changer (pun intended). Instead, I plan to revisit the spring relaxation algorithm altogether after the next two major versions (see roadmap at https://gamejolt.com/games/floating-sandbox/353572/devlog/the-future-of-floating-sandbox-cdk2c9yi). There is a different family of algorithms based on minimization of potential energy, which supposedly requires less iterations and on top of that is easily parallelizable - the current iterative algorithm is not (easily) parallelize-able.
This said, in the current implementation, what matters the most is CPU speed - the whole simulation is basically single-threaded (some small steps are parallel, but they're puny compared with the spring relaxation). My laptop is a single-core, 2.2GHz Intel box, and the plain Titanic runs at ~22 FPS. The same ship on a 4-core, 1GHz Intel laptop runs at ~9FPS.

Rendering is a different story. At some point I've moved all the rendering code to a separate thread, allowing simulation updates and rendering to run in parallel. Obviously, only multi-core boxes benefit from parallel rendering, and boxes with very slow or emulated graphics hardware benefit the most. In any case, at this moment rendering requires a fraction of the time needed for updating the simulation, so CPU speed still dominates the performance you get, compared to GPU speed.

# Building the Game
I build this game with Visual Studio 2019 (thus with full C++ 17 support) on Windows. From time to time I also build on Ubuntu to ensure the codebase is still portable.

In order to build the game, you will need the following dependencies:
- <a href="https://www.wxwidgets.org/">WxWidgets</a> v3.1.4 (cross-platform GUI library)
- <a href="http://openil.sourceforge.net/">DevIL</a> (cross-platform image manipulation library)
- <a href="https://www.sfml-dev.org/index.php">SFML</a> (cross-platform multimedia library)
- <a href="https://github.com/kazuho/picojson">picojson</a> (header-only JSON parser and serializer)
- <a href="https://github.com/google/benchmark">Google Benchmark</a>
- <a href="https://github.com/google/googletest/">Google Test</a> release-1.10.0

A custom `UserSettings.cmake` may be used in order to configure the locations of all dependencies. If you want to use it, copy the `UserSettings.example-<platform>.cmake` example file to `UserSettings.cmake` and adapt it to your setup. In case you do not want to use this file, you can use the example to get an overview of all CMake variables you might need to use to configure the dependencies.

Over the years I've been writing down OS-specific build steps:
- [Ubuntu](https://github.com/GabrieleGiuseppini/Floating-Sandbox/blob/master/BUILD-Ubuntu.md)

# Contributing
At this moment I'm looking for volunteers for two specific tasks: creating new ships, and building the game on non-Windows platforms.

If you like building ships and you are making nice models, I'd like to collect your ships - and whatever other bodies you can imagine floating and sinking in water! Just send your ships to me and you'll get a proper *thank you* in the About dialog!

Also, I'm looking for builders for non-Windows platforms. I'll also gladly accept any code contributions that may be necessary to ensure the project builds on multiple platforms.