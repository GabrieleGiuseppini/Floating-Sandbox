/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-31
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameFontSet.h"

#include <Core/GameException.h>
#include <Core/Utils.h>

namespace GameFontSet::_detail {

FontKind FontNameToFontKind(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);
    if (lstr == "Font0")
        return FontKind::Font0;
    else if (lstr == "Font1")
        return FontKind::Font1;
    else if (lstr == "SevenSegments")
        return FontKind::SevenSegments;
    else
        throw GameException("Unrecognized font \"" + str + "\"");
}

}