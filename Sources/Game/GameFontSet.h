/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-31
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <cstdint>
#include <string>

namespace GameFontSet {

/*
 * The different types of fonts.
 */
enum class FontKind : std::uint32_t
{
    Font0 = 0, // TODO: descriptive name
    Font1 = 1, // TODO: descriptive name
    SevenSegments = 2,

    _Last = SevenSegments
};

struct FontSet
{
    using FontKindType = FontKind;

    static inline std::string FontSetName = "Game";

    static FontKind FontNameToFontKind(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "Font0"))
            return FontKind::Font0;
        else if (Utils::CaseInsensitiveEquals(str, "Font1"))
            return FontKind::Font1;
        else if (Utils::CaseInsensitiveEquals(str, "SevenSegments"))
            return FontKind::SevenSegments;
        else
            throw GameException("Unrecognized font \"" + str + "\"");
    }
};

}