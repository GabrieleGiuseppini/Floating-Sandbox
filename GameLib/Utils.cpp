/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Utils.h"

#include "GameException.h"

#include <cwchar>
#include <memory>

namespace /* anonymous */ {
}

picojson::value Utils::ParseJSONFile(std::filesystem::path const & filepath)
{
	std::string fileContents = Utils::LoadTextFile(filepath);

	picojson::value jsonContent;
	std::string parseError = picojson::parse(jsonContent, fileContents);
	if (!parseError.empty())
	{
		throw GameException("Error parsing JSON file \"" + filepath.string() + "\": " + parseError);
	}

	return jsonContent;
}
