/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-03-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Utils.h>

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

struct MipMappedTextureDatabase
{
    static inline std::string DatabaseName = "ShipBuilderMipMappedTexture";

    using TextureGroupsType = MipMappedTextureGroups;

    static TextureGroupsType StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "WaterlineMarker"))
            return MipMappedTextureGroups::WaterlineMarker;
        else
            throw GameException("Unrecognized MipMappedTexture texture group \"" + str + "\"");
    }
};

}
