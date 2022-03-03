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

// Linear

enum class LinearTextureGroups : uint16_t
{
    WaterlineMarker,

    _Last = WaterlineMarker
};

struct LinearTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "ShipBuilderLinearTexture";

    using TextureGroups = LinearTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "WaterlineMarker"))
            return TextureGroups::WaterlineMarker;
        else
            throw GameException("Unrecognized LinearTexture texture group \"" + str + "\"");
    }
};

}
