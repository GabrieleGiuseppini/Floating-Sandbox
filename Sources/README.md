# Source Structure
This folder contains all the sources for Floating Sandbox. 

The structure has been designed for easy in-place porting to different platforms (PC, Mobile); the goal is that some of these folders - the "portable" ones - which contain the meat of the game, may be ported as-is to a different platform. Other folders may not, and would need to be re-written/customized for different platforms. In here, these sources are for the PC platform.

The project is structured as follows:

## Core
*Portable*

Contains the bottom-most layer of the game. Mostly definitions of quite-general types.

Since it needs to be portable, it has no filesystem/fstream dependencies, nor OpenGL.

Depends on nothing else.

## OpenGLCore
*Platform-specific* (probably)

Utility code on top of OpenGL, such as OpenGL library loading and initialization, ARB loading, and slightly-higher-level helpers on top of OpenGL.

Depends on _Core_.

## Render
*Platform-specific*

Implements the rendering engine of the game, which in turn is built on top of OpenGL.

Though this library is non-portable, it is depended on by the portable _Simulation_; this implies that any port to a different platform needs to implement this library's **interface** verbatim.

Depends on _Core_ and _OpenGLCore_.

## Simulation
*Portable*

This is the real meat. It contains everything needed for the physics simulation.

Since it needs to be portable, it has no filesystem/fstream dependencies, nor OpenGL.

Depends on _Core_ and _Render_.

## Game
*Platform-specific*

Mostly the platform-specific orchestration of the game. Different platforms might have completely different implementations of this library.

Depends on _Core_, _Render_, and _Simulation_.

## UILib
*Platform-specific*

Mostly helpers built on top of wxWidgets. Since UIs are assumed to be heavily platform-dependent, this is totally non-portable.

## FloatingSandbox
*Platform-specific*

The actual application for Floating Sandbox on PC. Implements UI, sounds, and tools.

Heavily built on top of wxWidgets.

Depends on everything else.

---

Following are secondary source folders - these do not contribute directly to the game.

## ShipBuilderLib

The meat of the ShipBuilder. The only reason for it to not be 100% portable is that there are no plans, at this moment, to have the ShipBuilder on non-PC platforms. I might revisit this decision one day.

## ShipBuilder

The actual application for the ShipBuilder on PC. Implements its UI.

## ShipTools

A command-line interface for development-time tasks around Floating Sandbox, such as the baking of textures into atlases.

## SoundCore

Core sound sub-system definitions for the Anroid port (the PC port uses OpenAL). It's here as it's used by _ShipTools_ to bake sound atlases for Android.

## UnitTests

Unit tests for some important functionalities of the game, which would be otherwise hard to thoroughly test via the game itself. Mostly tests _Core_ and _Simulation_ components.

## Benchmarks

Various benchmarks that I have developed over the years to test performance of alternative implementations of various algorithms, or to verify performance improvements.

## GPUCalc, GPUCalcTest

A proof of concept for using the GPU to perform generic calculations. Not used in the game, but it worked and I'm keeping it around until I have the guts to profile its performance vs. CPU-based performance.
