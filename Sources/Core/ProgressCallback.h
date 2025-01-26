/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <string>

enum class ProgressMessageType : std::size_t
{
	None = 0,						// Used when no message is propagated
	LoadingFonts,					// "Loading fonts..."
	InitializingOpenGL,				// "Initializing OpenGL..."
	LoadingShaders,					// "Loading shaders..."
	InitializingNoise,				// "Initializing noise..."
	LoadingGenericTextures,			// "Loading generic textures..."
	LoadingExplosionTextureAtlas,	// "Loading explosion texture atlas..."
	LoadingCloudTextureAtlas,		// "Loading cloud texture atlas..."
	LoadingFishTextureAtlas,		// "Loading fish texture atlas..."
	LoadingWorldTextures,			// "Loading world textures..."
	InitializingGraphics,			// "Initializing graphics..."
	LoadingSounds,					// "Loading sounds..."
	LoadingMusic,					// "Loading music..."
	LoadingElectricalPanel,			// "Loading electrical panel..."
	LoadingShipBuilder,				// "Loading ShipBuilder..."
	LoadingMaterialPalette,			// "Loading materials palette..."
	Calibrating,					// "Calibrating game on the computer..."
	Ready,							// "Ready!"

	_Last = Ready
};

// The progress value is the progress that will be reached at the end of the operation
using ProgressCallback = std::function<void(float progress, ProgressMessageType message)>;
