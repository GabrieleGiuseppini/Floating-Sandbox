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
#include <Core/Utils.h>

#include <iostream>

std::tuple<size_t, size_t> SoundAtlasBaker::Bake(
	std::filesystem::path const & soundsRootDirectoryPath,
	std::string const & atlasName,
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
			if (entryIt.path().extension().string() == ".raw")
			{
				assetNames.emplace_back(entryIt.path().filename().stem().string());
			}
			else if (entryIt.path().extension().string() == ".json")
			{
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
	// TODOHERE
	//


	(void)atlasName; // TODO: nuke?
	(void)outputDirectoryPath;
	// TODOHERE
	return { 0, 0 };
}