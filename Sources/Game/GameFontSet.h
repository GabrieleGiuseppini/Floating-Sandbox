/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-31
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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

namespace _detail {

FontKind FontNameToFontKind(std::string const & str);

}

struct FontSet
{
    using FontKindType = FontKind;

    static inline std::string FontSetName = "Game";

    static constexpr auto FontNameToFontKind = _detail::FontNameToFontKind;
};

}