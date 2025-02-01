/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "ImageData.h"

#include <picojson.h>

#include <cstdint>
#include <string>
#include <vector>

/*
 * Abstracts away the details on how to retrieve game assets.
 * Provides asset retrieval services to anything underneath Game
 * (thus @ Simulation and down).
 *
 * Game assets are identified via the use of very-specific methods,
 * and this interface encapsulates the knowledge about their location
 * and their retrieval.
 *
 * This class is implemented platform-specific and the implementation
 * is passed around as a reference where needed.
 */
class IAssetManager
{
public:

	struct AssetDescriptor
	{
		std::string Name; // e.g. filename stem
		std::string RelativePath;
	};

	// Texture databases
	virtual picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName) const = 0;
	virtual ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const = 0;
	virtual RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const = 0;
	virtual std::vector<AssetDescriptor> EnumerateTextureDatabaseFrames(std::string const & databaseName) const = 0;

	// Texture atlases
	virtual picojson::value LoadTetureAtlasSpecification(std::string const & textureDatabaseName) const = 0;
	virtual RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) const = 0;

	// Shaders
	virtual std::vector<AssetDescriptor> EnumerateShaders(std::string const & shaderSetName) const = 0;
	virtual std::string LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath) const = 0;

	// Fonts
	virtual std::vector<AssetDescriptor> EnumerateFonts(std::string const & fontSetName) const = 0;
	virtual Buffer<std::uint8_t> LoadFont(std::string const & fontSetName, std::string const & fontRelativePath) const = 0;
};
