/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-03-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <cstdint>
#include <string>

namespace ShipBuilder {

//
// Texture databases
//

// Generic Linear

enum class GenericLinearTextureGroups : uint16_t
{
    WaterlineMarker,

    _Last = WaterlineMarker
};

struct GenericLinearTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "ShipBuilderGenericLinearTexture";

    using TextureGroups = GenericLinearTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "WaterlineMarker"))
            return TextureGroups::WaterlineMarker;
        else
            throw GameException("Unrecognized GenericLinearTexture texture group \"" + str + "\"");
    }
};

}
