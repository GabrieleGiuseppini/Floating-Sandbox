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
        std::stringstream sSource(source);
        std::stringstream sSubstitutedSource;
        std::string line;
        while (std::getline(sSource, line))
        {
            auto commentStart = line.find("//");
            if (commentStart != std::string::npos)
            {
                sSubstitutedSource << line.substr(0, commentStart);
            }
            else
            {
                sSubstitutedSource << line;
            }
        }

        return sSubstitutedSource.str();
    }
}

picojson::value Utils::ParseJSONString(std::string const & jsonString)
{
    picojson::value jsonContent;

    if (!jsonString.empty())
    {
        std::string const parseError = picojson::parse(jsonContent, RemoveJSONComments(jsonString));
        if (!parseError.empty())
        {
            throw GameException("Error parsing JSON string: " + parseError);
        }
    }

    return jsonContent;
}

std::string Utils::MakeStringFromJSON(picojson::value const & value)
{
    return value.serialize(true);
}

////////////////////////////////////////////////////////
// Misc
////////////////////////////////////////////////////////

std::string Utils::ChangelistToHtml(std::istream & inputStream)
{
    static std::regex const FeatureRegex(R"(^(\s*)-\s*(.*)\s*$)");

    std::stringstream outputStream;

    // State
    int currentIndent = 0;
    bool isCurrentlyInBullet = false;

    while (true)
    {
        std::string line;
        std::getline(inputStream, line);
        if (line.empty() && !inputStream.good())
        {
            // We're done
            break;
        }

        line = RTrim(line);

        if (line.empty() && outputStream.tellp() != 0)
        {
            // We're done with this section
            break;
        }

        std::smatch featureMatch;
        if (std::regex_match(line, featureMatch, FeatureRegex))
        {
            //
            // Bullet
            //

            // Close previous
            if (isCurrentlyInBullet)
            {
                outputStream << "</li>";
                isCurrentlyInBullet = false;
            }

            assert(featureMatch.size() == 1 + 2);

            // Calculate indent size
            int indent = 0;
            for (char const ch : featureMatch[1].str())
            {
                if (ch == '\t')
                    indent += 4;
                else
                    indent += 1;
            }

            // Normalize in 1...N range
            indent = 1 + (indent / 4);

            // Indent

            while (indent > currentIndent)
            {
                outputStream << "<ul>";
                ++currentIndent;
            }

            while (indent < currentIndent)
            {
                outputStream << "</ul>";
                --currentIndent;
            }

            // Add bullet
            outputStream << "<li>" << Trim(featureMatch[2].str());

            isCurrentlyInBullet = true;
        }
        else
        {
            //
            // No new bullet
            //

            // Eventually new line
            if (outputStream.tellp() != 0)
            {
                outputStream << "<br/>";
            }

            outputStream << Trim(line);
        }
    }

    // Close

    if (isCurrentlyInBullet)
    {
        outputStream << "</li>";
        isCurrentlyInBullet = false;
    }

    while (currentIndent > 0)
    {
        outputStream << "</ul>";
        --currentIndent;
    }

    return outputStream.str();
}