/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-01-27
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/IAssetManager.h>
#include <Core/ImageData.h>

#include <picojson.h>

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

	picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName) override;
	ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameFileName) override;
	RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameFileName) override;
	std::vector<std::string> EnumerateTextureDatabaseFrames(std::string const & databaseName) override;

	picojson::value LoadTetureAtlasSpecification(std::string const & textureDatabaseName) override;
	RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) override;

	//
	// Platform-specific
	//

	// Helpers

	static ImageSize GetImageSize(std::filesystem::path const & filePath);

	template<typename TColor>
	static ImageData<TColor> LoadPngImage(std::filesystem::path const & filePath);
	static RgbaImageData LoadPngImageRgba(std::filesystem::path const & filePath);
	static RgbImageData LoadPngImageRgb(std::filesystem::path const & filePath);

	static void SavePngImage(
		RgbaImageData const & image,
		std::filesystem::path filePath);

	static void SavePngImage(
		RgbImageData const & image,
		std::filesystem::path filePath);

	static picojson::value LoadJson(std::filesystem::path const & filePath);

	static void SaveJson(
		picojson::value const & json,
		std::filesystem::path const & filePath);

	static std::filesystem::path MakeAtlasSpecificationFilename(std::string const & textureDatabaseName)
	{
		return textureDatabaseName + ".atlas.json";
	}

	static std::filesystem::path MakeAtlasImageFilename(std::string const & textureDatabaseName)
	{
		return textureDatabaseName + ".atlas.png";
	}

private:

	std::filesystem::path const mDataRoot;
	std::filesystem::path const mTextureDatabaseRoot;
};
