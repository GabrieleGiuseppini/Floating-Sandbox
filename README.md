# Floating Sandbox
Two-dimensional particle system in C++, simulating physical bodies floating in water and sinking.

# Overview
This game is a realistic two-dimensional particle system, which uses mass-spring networks to simulate rigid bodies (such as ships) floating in water. You can punch holes in an object, slice it, apply forces to it, and smash it with bomb explosions. Once water starts rushing inside the object, it will start sinking and, if you're lucky enough, it will start breaking up!

<img src="https://i.imgur.com/c8fTsgY.png">
<img src="https://i.imgur.com/kovxCty.png">

You can create your own physical objects by drawing images using colors that correspond to materials in the game's library. Each material has its own physical properties, such as mass, strength, stiffness, water permeability, sound properties, and so on. The game comes with a handy template image that you can use to quickly select the right materials for your object.

If you want, you can also apply a higher-resolution image to be used as a more realistic texture for the object. Once you load your object, watch it float and explore how it behaves under stress!

For the physics in the simulation I'm trying to shy away from tricks that exist solely for the purpose of delivering an eye-candy; every bit is grounded as close as possible into real physics, and the material system has been put together using physical attributes of real-world materials. This makes it sometimes hard to build structures that sustain their own weight or float easily - as it is in reality, after all - but the reward is a realistic world-in-a-sandbox where every action and corresponding reaction are not pre-programmed but, rather, are generated automatically by the physics engine calculations.

The game currently comes with a few example objects - mostly ships - and I'm always busy making new ships and objects. Anyone is encouraged to make their own objects, and if you'd like them to be included in the game, just get in touch with me - you'll get proper recognition in the About dialog, of course.

The original idea for the game is from Luke Wren, who wrote a Sinking Ship Simulator to simulate sinking ships. I have adopted his idea, completely reimplemented his simulator, and revamped its feature set; at this moment it is really a generic physics simulator that can be used to simulate just about any floating rigid body under stress.

There are lots of ideas that I'm currently working on; some of these are:
- Water drag to simulate parts gliding underwater
- Waves and splashes originating from collisions with water
- Debris when parts break, and bubbles when water enters the ship
- Sparkles when metal is sliced
- Smoke and fire
- Multiple ships and collision detection among parts of the ships
- Water pumps and anti-flood walls controllable via electrical switches (Mathias Garbe's idea)

These and other ideas will come out with frequent releases.

The game is also featured at [GameJolt](https://gamejolt.com/games/floating-sandbox/353572)!

And a Youtube video is at [https://youtu.be/LkYAFKVdZec](https://youtu.be/LkYAFKVdZec).

# Technical Details
This game is a C++ implementation of a particular class of particle systems, namely a *mass-spring-damper* network. With a mass-spring-damper network it is possible to simulate a rigid body by decomposing it into a number of infinitesimal particles ("points"), which are linked to each other via spring-damper pairs. Springs help maintain the rigidity of the body, while dampers are mostly to maintain the numerical stability of the system.

At any given moment, the forces acting on a point are:
- Spring forces, proportional to the elongation of the spring (Hooke's law) and thus to the positions of the two endpoints
- Damper forces, proportional to the relative velocity of the endpoints of the spring and thus to the velocity of the two endpoints
- Gravity and buoyance forces, proportional to the mass and "wetness" of the points
- Forces deriving from the interactions with the user, who can apply radial or angular forces, generate explosions, and so on

Water that enters the body moves following gravity and pressure gradients, and it adds to the mass of each "wet" point rendering parts of the body heavier.

Bodies are loaded from *png* images; each pixel in the image becomes a point in the simulated world, and springs connect each point to all of its neighbours. The color of the pixel in the original image determines the material of the corresponding point, based on a dictionary containing tens of materials; the material of a point in turn determines the physical properties of the point (e.g. mass, water permeability, electrical conductivity) and of the springs attached to it (e.g. stiffness, strength).

An optional texture map may be applied on top of the body, which will be drawn according to a tessellation of the network of points.

Users can interact with a body in different ways:
- Break parts of the body
- Slice the body in pieces
- Apply radial forces and angular forces
- Deploy bombs
- Pin individual points of the body so that their position (and velocity) becomes frozen
- Add or remove water to and from the body

<img src="https://i.imgur.com/WUk7qGv.png">

# History
I started coding this game after stumbling upon Luke Wren's and Francis Racicot's (Pac0master) [Ship Sandbox](https://github.com/Wren6991/Ship-Sandbox). After becoming fascinated by it, I [forked](https://github.com/GabrieleGiuseppini/Ship-Sandbox) his GitHub repo and started playing with the source code. Here's a list of the major changes I've been doing on the original codebase:
- Completely rewritten physics layer, making simulation more grounded in reality
- Completely rewritten data structures to maximize data locality
- Rewritten dynamics integration step to make full use of packed SSE floating point instructions on Intel x86
- Restructured interactions between the UI and the game, splitting settings between physics-related settings and render-related settings
- Rearchitected lifetime management of elements - originally elements were removed from vectors while these are being iterated, and the entire "points-to" graph was a tad too complex
- Completely rewritten the graphics layer, targeting OpenGL 2.0 "core profile" (i.e. no compatibility API) with custom shaders and texture mapping
- Added sounds and cued music
- Added lights that are powered by generators connected by cables, and flicker and turn off when the circuit is interrupted
- Added bombs
- Added connected component detection, used to correctly draw ship break-away parts on top of each other, among other things
- Upgraded to C++17

...all of this while improving the game's FPS rate from 7 to 27 (on my 2009 laptop!)

After a while I realized that I had rewritten all of the original code, and that my new project was thus worthy of a new name and a new source code repository, the one you are looking at now.

Here's a rought list of the major remaining changes I want to do:
- Directional water drag forces, to simulate underwater gliding
	- Requires maintaining convex hull and ship perimeter normals
- Better waves, also generated by the ship's movements
	- Probably using shallow water equations
- Ephemeral particles to simulate fire, smoke from smokestacks, sparkles from the chainsaw, and water bubbles when water enters the ship
- Switches to control electrical circuits, e.g. lights
- Electrically-controlled water pumps - to control submarines' buoyancy! (Mathias Garbe's idea)
- Electrically-controlled flood walls
- Electrically-controlled ship-specific horn sounds
- A new ship file loader dialog with previews of ships
- Ocean floor that gets dents when an impact occurs (Louis Benedict's idea)
- Multiple ships and collision detection
- Controllable simulation quality (trade-off between better stiffness vs. frames per second)
- Differently-colored lights
- Time-of-day (i.e. day light change during the game)
- In-game screen snapshots
- Persistent settings
- Multiple layers for ship building
- Morse code sound before sinking
- Auto-zoom when a ship is loaded

# Building the Game
I build this game with Visual Studio 2017 (update 15.8, hence with std::filesystem).
I tried to do my best to craft the CMake files in a platform-independent way, but I'm working on this exclusively in Visual Studio, hence I'm sure some unportable features have slipped in. Feel free to send pull requests for CMake edits for other platforms.

In order to build the game, you will need the following dependencies:
- <a href="https://www.wxwidgets.org/">WxWidgets</a> (cross-platform GUI library)*
- <a href="http://openil.sourceforge.net/">DevIL</a> (cross-platform image library)*
- <a href="https://www.sfml-dev.org/index.php">SFML</a> (cross-platform multimedia library)*
- <a href="https://github.com/kazuho/picojson">picojson</a> (header-only JSON parser and serializer)
- <a href="https://github.com/p12tic/libsimdpp">libsimdpp</a> (header-only SIMD library)
- <a href="https://github.com/google/benchmark">Google Benchmark</a>
- <a href="https://github.com/google/googletest/">Google Test</a>

Dependencies marked with * may be statically linked by using the `MSVC_USE_STATIC_LINKING` option.

A custom `UserSettings.cmake` may be used in order to configure the locations of all dependencies. If you want to use it, copy the `UserSettings.example.cmake` to `UserSettings.cmake` and adapt it to your setup. In case you do not want to use this file, you can use the example to get an overview of all CMake variables you might need to use to configure the dependencies.

# Contributing
At this moment I'm looking for volunteers for three specific tasks: creating a "Ship Editor" for the game, creating new ships, and building the game on non-Windows platforms.

With the "Ship Editor" a user would be able to craft a ship from nothing, picking materials out of a dictionary, laying out ropes, and adjusting texture maps to the ship's structure. The UI would also provide feedback on the current buoyancy of the ship and its mass center. Contact me if you'd like to apply! You need to know C++ as most of the physics code you'll use in the UI will come straight from the game's core library.

If you're more on the graphics side, instead, I'd like to collect your ships - and whatever other bodies you can imagine floating and sinking in water! Just send your ships to me and you'll get a proper *thank you* in the About dialog!

Finally, I'm looking for builders for non-Windows platforms. I'll also gladly accept any code contributions that may be necessary to ensure the project builds on multiple platforms.