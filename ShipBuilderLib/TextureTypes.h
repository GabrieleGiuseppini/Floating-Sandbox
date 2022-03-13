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

enum class MipMappedTextureGroups : uint16_t
{
    WaterlineMarker,

    _Last = WaterlineMarker
};

struct MipMappedTextureTextureDatabaseTraits
{
    static inline std::string DatabaseName = "ShipBuilderMipMappedTexture";

    using TextureGroups = MipMappedTextureGroups;

    static TextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "WaterlineMarker"))
            return TextureGroups::WaterlineMarker;
        else
            throw GameException("Unrecognized MipMappedTexture texture group \"" + str + "\"");
    }
};

}
