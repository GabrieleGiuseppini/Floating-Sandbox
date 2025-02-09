/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-04-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipNameNormalizer.h"

#include <Game/FileStreams.h>

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cassert>
#include <cctype>
#include <regex>

namespace ShipBuilder {

ShipNameNormalizer::ShipNameNormalizer(GameAssetManager const & gameAssetManager)
    : ShipNameNormalizer(FileTextReadStream(gameAssetManager.GetShipNamePrefixListFilePath()).ReadAllLines())
{
}

ShipNameNormalizer::ShipNameNormalizer(std::vector<std::string> && prefixes)
{
    //
    // Build prefix map
    //

    for (std::string const & normalPrefix : prefixes)
    {
        // Stem
        std::string stemPrefix;
        for (auto const ch : normalPrefix)
        {
            if (!std::isspace(ch) && !std::ispunct(ch))
            {
                stemPrefix.append(1, static_cast<typename std::string::value_type>(std::toupper(ch)));
            }
        }

        if (stemPrefix.empty())
        {
            throw GameException("Ship prefix \"" + normalPrefix + "\" stems to an empty stem");
        }

        // Insert in map
        if (auto const & res = mPrefixMap.try_emplace(stemPrefix, normalPrefix);
            !res.second)
        {
            throw GameException("Ship prefix \"" + normalPrefix + "\" stems to the same stem as prefix \"" + mPrefixMap[stemPrefix] + "\"");
        }
    }
}

std::string ShipNameNormalizer::NormalizeName(std::string const & shipName) const
{
    //
    // Normalize prefix
    //

    std::string bestPrefix;
    size_t iBestPrefixRestStart = 0; // Points to first usable name character after best prefix

    std::string currentStem;
    bool isInWord = false; // Used to detect word boundaries
    for (size_t i = 0; ; ++i)
    {
        if (i < shipName.length())
        {
            char const ch = shipName[i];

            if (std::isspace(ch) || std::ispunct(ch))
            {
                isInWord = false;

                // Eat it
                continue;
            }
        }
        else
        {
            isInWord = false;
        }

        // We are at a letter or at EOS

        if (!isInWord)
        {
            // We are at a word boundary (evt. including EOS)

            // See if can get a match with the stem we've been ccumulating so far
            auto const it = mPrefixMap.find(currentStem);
            if (it != mPrefixMap.end())
            {
                bestPrefix = it->second;
                iBestPrefixRestStart = i;
            }
        }

        if (i == shipName.length())
        {
            break;
        }

        // It's a letter...
        // incorporate it in stem
        currentStem.append(1, static_cast<typename std::string::value_type>(std::toupper(shipName[i])));

        isInWord = true;
    }

    // Build result

    std::string result = bestPrefix;

    bool hasAccumulatedSpaces = !(result.empty());
    for (size_t i = iBestPrefixRestStart; i < shipName.length(); ++i)
    {
        char const ch = shipName[i];

        if (std::isspace(ch))
        {
            hasAccumulatedSpaces = true;
        }
        else
        {
            if (hasAccumulatedSpaces)
            {
                result += " ";
                hasAccumulatedSpaces = false;
            }

            result += ch;
        }
    }

    //
    // Normalize year
    //

    static std::regex const YearRegex(R"(^.*?\b(\s*[-\(]?\s*(\d{4})\s*[\)]?)\s*$)");

    std::smatch yearMatch;
    if (std::regex_match(result, yearMatch, YearRegex))
    {
        assert(yearMatch.size() == 1 + 2); // @1: ugly, @2: year

        // Build result

        // Prefix
        assert(yearMatch[1].matched);
        std::string yearResult = result.substr(0, yearMatch.position(1));

        // Year
        assert(yearMatch[2].matched);
        if (!yearResult.empty())
        {
            yearResult += " ";
        }
        yearResult += "(" + yearMatch.str(2) + ")";

        result = yearResult;
    }

    return result;
}

}