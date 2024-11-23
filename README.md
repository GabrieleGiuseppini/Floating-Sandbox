# Floating Sandbox
A two-dimensional physics simulation written in C++.

# Overview
Floating Sandbox is a realistic 2D physics simulator. It is essentially a particle system that uses mass-spring networks to simulate rigid bodies, with added thermodynamics, fluid dynamics, and basic electrotechnics. The simulation is mostly focused around ships floating on water, but you can build any kind of object using the integrated ShipBuilder and a database of over 1,000 different materials. Once you build an object you can punch holes into it, slice it, apply forces and heat, set it on fire, smash it with bomb explosions - anything you want. And when it starts sinking, you can watch it slowly dive its way into the abyss, where it will rot for eternity!

<img src="https://i.imgur.com/Rl9K9w1.png">

The simulator is _crammed_ with physics; every conceivable aspect of the gameplay is governed exclusively by classical mechanics (e.g. friction and elasticity, conservation of momentum, Hooke's spring forces, impacts), thermodynamics (e.g. heat transfer, dissipation, combustion and melting), fluid dynamics (e.g. buoyancy, drag, hydrostatic and atmospheric pressure, wind), and so on, while the extensive material system has been put together using real physical characteristics specific to each of the materials - ranging from density, mass, and friction coefficients, up to elasticity and thermal expansion coefficients.

In coding this game I've been trying hard to stay true to my mission of avoiding "visual tricks", striving instead to obtain behaviors by means of careful, detailed, and honest simulations. The end result has been very rewarding for me, as I'm constantly surprised by the natural feeling and unexpected side-effects that spring out of the game. For example, round objects rotate when sliding downhill - which is completely due to friction, without a single line of code imposing some "magical" rotational forces - and different shapes sink with different velocities and trajectories - a side-effect of fluid dynamics acting on surfaces.

<img src="https://i.imgur.com/kovxCty.png">
<img src="https://i.imgur.com/XHw3Jrl.png">

The world of Floating Sandbox is rich with interactions, and new ones are being continuously added. To make a few examples, you can:
* Detonate different kinds of bombs with different behaviors
* Trigger storms with rain, hurricane winds, and lightnings - all interacting with the ship in a way or the other
* Overheat materials and reach either their combustion point or melting point
* Use tools to damage the ship by e.g. hitting, pulling parts, overheating, and inflating
* Use tools to impart tremendous radial or rotational force fields
* Control waves on the surface of the sea, creating monster waves and triggering tsunami's
* Spawn NPCs walking about the ship and being subject to what's happening around them

The simulator comes with a built-in ShipBuilder that allows you to create ships by drawing individual particles choosing materials out of the game's library. Each material has its own physical properties, such as mass, strength, stiffness, water permeability, specific heat, sound properties, and so on. You can also create electrical layers with electrical materials (lamps, engines, generators, switches, etc.), layers with ropes, and texture layers for a final, high-definition look'n'feel of the ship.

<img src="https://i.imgur.com/lSUj90c.png">
<img src="https://imgur.com/E0X3n93.png">

The game currently comes with a few example objects - mostly ships - and I'm always busy making new ships and objects. Anyone is encouraged to make their own objects, and if you'd like them to be included in the game, just get in touch with me - you'll get proper recognition in the About dialog, of course. Also have a look at [the official Floating Sandbox web site](https://floatingsandbox.com/), where you can find plenty of Ship Packs!

Floating Sandbox is featured at [GameJolt](https://gamejolt.com/games/floating-sandbox/353572), and plenty of videos may be found on any major social media platforms.

# Implementation Details
Some nerdy facts here.
* The most CPU-hungry algorithms are implemented with x86 instrinsics (targeting SSE-2) to take advantage of vectorized operations; for example, the newtonian integration step operates on 4 floats at a time
* The physical state of particles and springs is maintained in memory-aligned buffers, grouping quantities together as a function of the main algorithms operating on them - thus improving locality and reducing cache misses
* The spring relaxation algorithm (aka rigidity simulation) is at the core of the simulation, consuming about 60% of the time spent for the simulation of each single frame. At each frame we solve 40 micro-iterations, each distributed on as many cores as possible and implemented with finely-optimized routines written with x86 intrinsics; you may read more about these optimizations on [my technical blog](https://gabrielegiuseppini.wordpress.com/2023/04/01/adventures-with-2d-mass-spring-networks-part-i/)
* As the topology of the ship's mesh changes during the game due to destruction and wrecking, the simulator constantly re-calculates the external boundaries of each connected component via an algorithm that only operates on the neighborhood of topology changes
* The ocean surface is simulated with modified (Shallow Water Equations)[https://en.wikipedia.org/wiki/Shallow_water_equations] (SWE's), coupled with rigid bodies to generate surface perturbations
* The physics of NPC's takes place in a [barycentric coordinate system](https://en.wikipedia.org/wiki/Barycentric_coordinate_system), tracing NPC's particles' trajectories within the triangular ship mesh
* Rendering is implemented with OpenGL (targeting a very old but widely-adopted 2.1) and happens on a separate thread, allowing the game to update the next simulation step while rendering the previous one. All of the final shading is implemented in _glsl_, with many renderings being completely procedural (e.g. flames, lightnings, rain)

And even with all of this, the simulator still adapts itself to the characteristics of the machine it's running on, obtaining perfectly reasonable FPS rates on old computers - even on single-core laptops!

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
		- Many Linux distributions now use Wayland for their desktop environments, and Floating Sandbox will encounter an error when launching. To rectify this, set the environment variable "GDK_BACKEND" to "x11"
	- Note: the game has been built and tested only on Ubuntu 24.04

<img src="https://i.imgur.com/6LOVsqX.jpg">

# History
I started coding this game after stumbling upon Luke Wren's and Francis Racicot's (Pac0master) [Ship Sandbox](https://github.com/Wren6991/Ship-Sandbox). After becoming fascinated by it, I [forked](https://github.com/GabrieleGiuseppini/Ship-Sandbox) Luke's GitHub repo and started playing with the source code. After less than a year I realized I had rewritten all of the original code, while improving the game's FPS rate from 7 to 30 (on my old 2009 laptop!). At that moment I decided that my new project was worthy of a new name and a new source code repository, the one you are looking at now.

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

If you like building ships and you are making nice models, I'd like to collect your ships - and whatever other bodies you can imagine floating and sinking in water! Just send your ships to me and you'll get a proper *thank you* in the _About_ window!

Also, I'm looking for builders for non-Windows platforms. I'll also gladly accept any code contributions that may be necessary to ensure the project builds on multiple platforms.
