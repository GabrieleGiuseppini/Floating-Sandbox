/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-01-27
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/IAssetManager.h>

#include <filesystem>
#include <string>

class GameAssetManager : public IAssetManager
{
public:

	explicit GameAssetManager(std::string const & argv0);

	explicit GameAssetManager(std::filesystem::path const & textureDatabaseRoot);

	//
	// IAssetManager
	//

	picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName);
	ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameFileName);
	RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameFileName);
	std::vector<std::string> EnumerateTextureDatabaseFrames(std::string const & databaseName);

	picojson::value LoadTetureAtlasSpecification(std::string const & textureDatabaseName);
	RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName);

	//
	// Platform-specific
	//

private:

	std::filesystem::path const mDataRoot;
	std::filesystem::path const mTextureDatabaseRoot;
};
