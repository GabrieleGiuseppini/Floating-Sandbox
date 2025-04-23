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

	explicit GameAssetManager(std::string const && argv0);

	explicit GameAssetManager(std::filesystem::path const & textureRoot);

	//
	// IAssetManager
	//

	picojson::value LoadTetureDatabaseSpecification(std::string const & databaseName) const override;
	ImageSize GetTextureDatabaseFrameSize(std::string const & databaseName, std::string const & frameRelativePath) const override;
	RgbaImageData LoadTextureDatabaseFrameRGBA(std::string const & databaseName, std::string const & frameRelativePath) const override;
	std::vector<AssetDescriptor> EnumerateTextureDatabaseFrames(std::string const & databaseName) const override;

	std::string GetMaterialTextureRelativePath(std::string const & materialTextureName) const override;
	RgbImageData LoadMaterialTexture(std::string const & frameRelativePath) const override;

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

	// Ships

	std::filesystem::path GetInstalledShipFolderPath() const;

	std::filesystem::path GetDefaultShipDefinitionFilePath() const;

	std::filesystem::path GetFallbackShipDefinitionFilePath() const;

	std::filesystem::path GetApril1stShipDefinitionFilePath() const;

	std::filesystem::path GetHolidaysShipDefinitionFilePath() const;

	// Music

	std::vector<std::string> GetMusicNames() const;

	std::filesystem::path GetMusicFilePath(std::string const & musicName) const;

	// Sounds

	std::vector<std::string> GetSoundNames() const;

	std::filesystem::path GetSoundFilePath(std::string const & soundName) const;

	// UI Resources

	std::filesystem::path GetCursorFilePath(std::string const & cursorName) const;

	std::filesystem::path GetIconFilePath(std::string const & iconName) const;

	std::filesystem::path GetArtFilePath(std::string const & artName) const;

	std::filesystem::path GetPngImageFilePath(std::string const & pngImageName) const;

	std::vector<std::filesystem::path> GetPngImageFilePaths(std::string const & pngImageNamePattern) const;

	std::filesystem::path GetShipNamePrefixListFilePath() const;

	// Theme Settings

	std::filesystem::path GetThemeSettingsRootFilePath() const;

	// Simulation

	std::filesystem::path GetDefaultOceanFloorHeightMapFilePath() const;

	// Help

	std::filesystem::path GetStartupTipFilePath(
		std::string const & desiredLanguageIdentifier,
		std::string const & defaultLanguageIdentifier) const;

	std::filesystem::path GetHelpFilePath(
		std::string const & desiredLanguageIdentifier,
		std::string const & defaultLanguageIdentifier) const;

	// Localization

	std::filesystem::path GetLanguagesRootPath() const;

	// Boot settings

	std::filesystem::path GetBootSettingsFilePath() const;

	// Helpers

	static bool Exists(std::filesystem::path const & filePath);

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

	static void SaveTextFile(
		std::string const & content,
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

	std::filesystem::path MakeMaterialTexturesRootPath() const
	{
		return mTextureRoot / "Material";
	}

private:

	std::filesystem::path const mGameRoot;
	std::filesystem::path const mDataRoot;
	std::filesystem::path const mResourcesRoot;
	std::filesystem::path const mTextureRoot;
	std::filesystem::path const mShaderRoot;
};
