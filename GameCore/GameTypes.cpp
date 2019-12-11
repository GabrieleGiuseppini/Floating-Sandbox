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
    if (Utils::CaseInsensitiveEquals(str, "Short"))
        return DurationShortLongType::Short;
    else if (Utils::CaseInsensitiveEquals(str, "Long"))
        return DurationShortLongType::Long;
    else
        throw GameException("Unrecognized DurationShortLongType \"" + str + "\"");
}