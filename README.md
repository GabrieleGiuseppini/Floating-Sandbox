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


There are lots of improvements that I'm currently working on; some of these are:
- Improved rigid body simulation algorithm
- Splashes originating from collisions with water
- Smoke from fire
- Multiple ships and collision detection among parts of the ships
- Ocean floor getting dents upon impact
- NPC's that move freely within ships

These and other ideas will come out with frequent releases.


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
