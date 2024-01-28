# Floating Sandbox
A two-dimensional physics simulation written in C++.

# Overview
Floating Sandbox is a realistic 2D physics simulator. It is essentially a particle system that uses mass-spring networks to simulate rigid bodies, with added thermodynamics, fluid dynamics, and basic electrotechnics. The simulation is mostly focused around ships floating on water, but you can build any kind of object using the integrated ShipBuilder and a database of over 1,000 different materials. Once you build an object you can punch holes into it, slice it, apply forces and heat, set it on fire, smash it with bomb explosions - anything you want. And when it starts sinking, you can watch it slowly dive its way into the abyss, where it will rot for eternity!

<img src="https://i.imgur.com/c8fTsgY.png">

The game is really a generic physics simulator that can be used to simulate just about any 2D floating rigid body under stress.

As of now the simulator implements the following aspects of physics:
- Classical mechanics - Hooke's law of springs, impacts with rigid surfaces, thrust from engines
- Thermodynamics - heat transfer, dissipation, combustion, melting
- Fluid dynamics - buoyancy, drag, hydrostatic and atmospheric pressure, wind
- Basic electrotechnics - conductivity

<img src="https://i.imgur.com/kovxCty.png">
<img src="https://i.imgur.com/XHw3Jrl.png">

The simulator comes with a built-in ShipBuilder that allows you to create ships by drawing individual particles choosing materials out of the game's library. Each material has its own physical properties, such as mass, strength, stiffness, water permeability, specific heat, sound properties, and so on. You can also create electrical layers with electrical materials (lamps, engines, generators, switches, etc.), layers with ropes, and texture layers for a final, high-definition look'n'feel of the ship.

<img src="https://i.imgur.com/lSUj90c.png">
<img src="https://imgur.com/E0X3n93.png">

In coding this game I'm trying to avoid as much as possible tricks that just please the eye; every bit of the simulation is instead grounded as close as possible into real physics. For example, the material system has been put together using actual physical attributes of real-world materials, and all of the interactions are constructed according to the laws of physics. This makes it sometimes hard to build structures that sustain their own weight or float easily - as it is in reality, after all - but the reward is a realistic world-in-a-sandbox where every action and corresponding reaction are not pre-programmed, but rather are evolved naturally by the physics simulation engine.

The game currently comes with a few example objects - mostly ships - and I'm always busy making new ships and objects. Anyone is encouraged to make their own objects, and if you'd like them to be included in the game, just get in touch with me - you'll get proper recognition in the About dialog, of course. Also have a look at [the official Floating Sandbox web site](https://floatingsandbox.com/), where you can find plenty of Ship Packs!

There are lots of improvements that I'm currently working on; some of these are:
- Improved rigid body simulation algorithm
- Splashes originating from collisions with water
- Smoke from fire
- Multiple ships and collision detection among parts of the ships
- Ocean floor getting dents upon impact
- NPC's that move freely within ships

These and other ideas will come out with frequent releases.

The game is also featured at [GameJolt](https://gamejolt.com/games/floating-sandbox/353572), and plenty of videos may be found on any major social media. 

# System Requirements
- Windows:
	- Windows 7, 8, 10, or 11, either 64-bit or 32-bit
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
The bottleneck at the moment is the spring relaxation algorithm (aka rigidity simulation), which requires about 60% of the time spent for the simulation of each single frame. This algorithm has been heavily optimized as of version 1.18.0 (you may read about these optimizations on [my technical blog](https://gabrielegiuseppini.wordpress.com/2023/04/01/adventures-with-2d-mass-spring-networks-part-i/)), and the entire simulation can now make use of multiple threads; nonetheless, rigidity simulation is still the bottleneck. 

Rendering is a different story. At some point I've moved all the rendering code to a separate thread, allowing simulation updates and rendering to run in parallel. Obviously, only multi-core boxes benefit from parallel rendering, and boxes with very slow or emulated graphics hardware benefit the most. In any case, at this moment rendering requires a fraction of the time needed for updating the simulation, so CPU speed still dominates the performance you get, compared to GPU speed.

# Building the Game
I build this game with Visual Studio 2022 (thus with full C++ 17 support) on Windows. From time to time I also build on Ubuntu to ensure the codebase is still portable.

In order to build the game, you will need the following dependencies:
- <a href="https://www.wxwidgets.org/">WxWidgets</a> (cross-platform GUI library) (v3.1.4)
- <a href="http://openil.sourceforge.net/">DevIL</a> (cross-platform image manipulation library) (1.8.0)
- <a href="https://www.sfml-dev.org/index.php">SFML</a> (cross-platform multimedia library) (2.5.1)
- <a href="https://github.com/kazuho/picojson">picojson</a> (header-only JSON parser and serializer)
- <a href="https://github.com/google/benchmark">Google Benchmark</a> (1.7.0)
- <a href="https://github.com/google/googletest/">Google Test</a> (release-1.12.0)
- ...and the ubiquitous _zlib_, _jpeg_, and _libpng_ libraries.

A custom `UserSettings.cmake` may be used in order to configure the locations of all dependencies. If you want to use it, copy the `UserSettings.example-<platform>.cmake` example file to `UserSettings.cmake` and adapt it to your setup. In case you do not want to use this file, you can use the example to get an overview of all CMake variables you might need to use to configure the dependencies.

Over the years I've been writing down OS-specific build steps:
- [Windows](https://github.com/GabrieleGiuseppini/Floating-Sandbox/blob/master/BUILD-Windows.md)
- [Ubuntu](https://github.com/GabrieleGiuseppini/Floating-Sandbox/blob/master/BUILD-Ubuntu.md)

# Contributing
At this moment I'm looking for volunteers for two specific tasks: creating new ships, and building the game on non-Windows platforms.

If you like building ships and you are making nice models, I'd like to collect your ships - and whatever other bodies you can imagine floating and sinking in water! Just send your ships to me and you'll get a proper *thank you* in the About dialog!

Also, I'm looking for builders for non-Windows platforms. I'll also gladly accept any code contributions that may be necessary to ensure the project builds on multiple platforms.
