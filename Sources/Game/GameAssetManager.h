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

	explicit GameAssetManager(std::filesystem::path const & textureRoot);

	//
	// IAssetManager
	//

	picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName) const override;
	ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const override;
	RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const override;
	std::vector<AssetDescriptor> EnumerateTextureDatabaseFrames(std::string const & databaseName) const override;

	picojson::value LoadTetureAtlasSpecification(std::string const & textureDatabaseName) const override;
	RgbaImageData LoadTextureAtlasImageRGBA(std::string const & textureDatabaseName) const override;

	std::vector<AssetDescriptor> EnumerateShaders(std::string const & shaderSetName) const override;
	std::string LoadShader(std::string const & shaderSetName, std::string const & shaderRelativePath) const override;

	std::vector<AssetDescriptor> EnumerateFonts(std::string const & fontSetName) const override;
	std::unique_ptr<BinaryReadStream> LoadFont(std::string const & fontSetName, std::string const & fontRelativePath) const override;

	picojson::value LoadStructuralMaterialDatabase() const override;
	picojson::value LoadElectricalMaterialDatabase() const override;
	picojson::value LoadFishSpeciesDatabase() const override;
	picojson::value LoadNpcDatabase() const override;


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
	std::filesystem::path const mTextureRoot;
	std::filesystem::path const mShaderRoot;
};
