/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundAtlasBaker.h"

#include <Game/FileStreams.h>

#include <SoundCore/SoundAtlas.h>
#include <SoundCore/SoundTypes.h>

#include <Core/GameException.h>
#include <Core/SysSpecifics.h>
#include <Core/Utils.h>

#include <cassert>
#include <iostream>

static std::string const SoundAssetFileExtension = ".raw";

std::tuple<size_t, size_t> SoundAtlasBaker::Bake(
	std::filesystem::path const & soundsRootDirectoryPath,
	std::filesystem::path const & outputDirectoryPath)
{
	//
	// Enumerate assets and load asset property overrides json
	//

	std::vector<std::string> assetNames;
	std::unordered_map<std::string, SoundAssetProperties> assetPropertiesProvider;
	bool hasFoundJson = false;

	for (auto const & entryIt : std::filesystem::directory_iterator(soundsRootDirectoryPath))
	{
		if (std::filesystem::is_regular_file(entryIt.path()))
		{
			if (entryIt.path().extension().string() == SoundAssetFileExtension)
			{
				// Sound

				assetNames.emplace_back(entryIt.path().filename().stem().string());
			}
			else if (entryIt.path().extension().string() == ".json")
			{
				// Json

				if (hasFoundJson)
				{
					throw GameException("Found more than one json file in input directory");
				}

				picojson::value const overridesJsonValue = Utils::ParseJSONString(FileTextReadStream(entryIt.path()).ReadAll());
				picojson::object const & overridesJsonMap = Utils::GetJsonValueAsObject(overridesJsonValue, "root");
				for (auto const & entry : overridesJsonMap)
				{
					assetPropertiesProvider.emplace(entry.first, SoundAssetProperties::Deserialize(entry.first, entry.second));
				}

				hasFoundJson = true;
			}
			else
			{
				throw GameException("Found unexpected file in input directory: \"" + entryIt.path().filename().string() + "\"");
			}
		}
	}

	std::cout << "Enumerated " << assetNames.size() << " assets and deserialized " << assetPropertiesProvider.size() << " asset property overrides." << std::endl;

	//
	// Prepare output file
	//

	auto const outputAtlasDataFilePathTmp = outputDirectoryPath / "atlas.dat.tmp";

	if (std::filesystem::exists(outputAtlasDataFilePathTmp))
	{
		std::filesystem::remove(outputAtlasDataFilePathTmp);
	}

	auto outputStream = std::unique_ptr<BinaryWriteStream>(new FileBinaryWriteStream(outputAtlasDataFilePathTmp));

	//
	// Build atlas
	//

	auto const atlasMetadata = SoundAtlasBuilder::BuildAtlas(
		assetNames,
		assetPropertiesProvider,
		[&](std::string const & assetName) -> Buffer<float>
		{
			auto const assetFilePath = soundsRootDirectoryPath / (assetName + SoundAssetFileExtension);
			auto const assetFileSizeBytes = static_cast<size_t>(std::filesystem::file_size(assetFilePath));
			assert((assetFileSizeBytes % sizeof(float)) == 0);
			auto const assetFileSizeFloats = assetFileSizeBytes / sizeof(float);
			auto const bufferSizeFloats = make_aligned_float_element_count(assetFileSizeFloats);

			Buffer<float> buf(bufferSizeFloats);
			FileBinaryReadStream(assetFilePath).Read(reinterpret_cast<std::uint8_t *>(buf.data()), assetFileSizeBytes);

			// Check buffer
			for (size_t i = 0; i < assetFileSizeFloats; ++i)
			{
				if (std::fabs(buf[i]) > 1.0f)
				{
					if (std::fabs(buf[i]) - 1.0f > 0.15f)
					{
						throw GameException("Sound \"" + assetName + "\" is not normalized! (" + std::to_string(buf[i]) + ")");
					}
					else
					{
						// Just flatten
						buf[i] = (buf[i] >= 0.0f) ? 1.0f : -1.0f;
					}

				}
			}

			// Pad with zeroes
			for (size_t i = assetFileSizeFloats; i < bufferSizeFloats; ++i)
			{
				buf[i] = 0.0f;
			}

			return buf;
		},
		*outputStream);

	outputStream.reset();

	//
	// Finalize atlas
	//

	auto const outputAtlasDataFilePath = outputDirectoryPath / "atlas.dat";

	if (std::filesystem::exists(outputAtlasDataFilePath))
	{
		std::filesystem::remove(outputAtlasDataFilePath);
	}

	std::filesystem::rename(outputAtlasDataFilePathTmp, outputAtlasDataFilePath);

	auto const outputAssetMetadataFilePath = outputDirectoryPath / "atlas.json";

	FileTextWriteStream(outputAssetMetadataFilePath).Write(Utils::MakeStringFromJSON(atlasMetadata.Serialize()));

	return { atlasMetadata.Entries.size(), static_cast<size_t>(std::filesystem::file_size(outputAtlasDataFilePath)) };
}