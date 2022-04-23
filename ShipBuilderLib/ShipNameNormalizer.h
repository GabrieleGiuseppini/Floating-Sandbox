/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-04-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace ShipBuilder {

class ShipNameNormalizer
{
public:

    explicit ShipNameNormalizer(ResourceLocator const & resourceLocator);

    explicit ShipNameNormalizer(std::vector<std::string> && prefixes);

    std::string NormalizeName(std::string const & shipName) const;

private:

    std::unordered_map<std::string, std::string> mPrefixMap; // Stem -> Normal
};

}
