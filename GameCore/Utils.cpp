/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Utils.h"

#include "GameException.h"

#include <cassert>
#include <memory>
#include <regex>

namespace /* anonymous */ {

    std::string RemoveJSONComments(std::string const & source)
    {
        static std::regex CommentRegex(R"!(^(.*?)(//.*)$)!");

        std::stringstream sSource(source);
        std::stringstream sSubstitutedSource;
        std::string line;
        std::smatch match;
        while (std::getline(sSource, line))
        {
            if (std::regex_match(line, match, CommentRegex))
            {
                assert(3 == match.size());
                sSubstitutedSource << std::string(match[1].first, match[1].second);
            }
            else
                sSubstitutedSource << line;
        }

        return sSubstitutedSource.str();
    }
}

picojson::value Utils::ParseJSONFile(std::filesystem::path const & filepath)
{
	std::string fileContents = RemoveJSONComments(Utils::LoadTextFile(filepath));

	picojson::value jsonContent;
	std::string parseError = picojson::parse(jsonContent, fileContents);
	if (!parseError.empty())
	{
		throw GameException("Error parsing JSON file \"" + filepath.string() + "\": " + parseError);
	}

	return jsonContent;
}

void Utils::SaveJSONFile(
    picojson::value const & value,
    std::filesystem::path const & filepath)
{
    std::string serializedJson = value.serialize(true);

    Utils::SaveTextFile(
        serializedJson,
        filepath);
}