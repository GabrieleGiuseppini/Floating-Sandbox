/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameTypes.h"

#include "GameException.h"
#include "Utils.h"

DurationShortLongType StrToDurationShortLongType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "short")
        return DurationShortLongType::Short;
    else if (lstr == "long")
        return DurationShortLongType::Long;
    else
        throw GameException("Unrecognized DurationShortLongType \"" + str + "\"");
}

