/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-04-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipNameNormalizer.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cassert>
#include <cctype>

namespace ShipBuilder {

ShipNameNormalizer::ShipNameNormalizer(ResourceLocator const & resourceLocator)
    : ShipNameNormalizer(Utils::LoadTextFileLines(resourceLocator.GetShipNamePrefixListFilePath()))
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
            if (std::isalpha(ch))
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
    size_t i = 0;
    for (; ; ++i)
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

            if (!std::isalpha(ch))
            {
                // We're done
                break;
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

            // See if can get a match so far
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

    // Consume trailing spaces before rest
    for (;
        iBestPrefixRestStart < shipName.length() && std::isspace(shipName[iBestPrefixRestStart]);
        ++iBestPrefixRestStart);

    // Consume trailing spaces after rest
    size_t iRestEnd = shipName.length();
    for (;
        iRestEnd > iBestPrefixRestStart && std::isspace(shipName[iRestEnd - 1]);
        --iRestEnd);

    // Build result
    std::string result = bestPrefix;
    if (iBestPrefixRestStart < iRestEnd)
    {
        if (!result.empty())
        {
            result += " ";

        }

        result += shipName.substr(iBestPrefixRestStart, iRestEnd - iBestPrefixRestStart);
    }

    return result;
}

}