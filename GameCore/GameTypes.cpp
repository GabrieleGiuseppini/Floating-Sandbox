/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameTypes.h"

#include "GameException.h"
#include "Utils.h"

int const TessellationCircularOrderDirections[8][2] = {
    {  1,  0 },  // 0: E
    {  1, -1 },  // 1: SE
    {  0, -1 },  // 2: S
    { -1, -1 },  // 3: SW
    { -1,  0 },  // 4: W
    { -1,  1 },  // 5: NW
    {  0,  1 },  // 6: N
    {  1,  1 }   // 7: NE
};

DurationShortLongType StrToDurationShortLongType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Short"))
        return DurationShortLongType::Short;
    else if (Utils::CaseInsensitiveEquals(str, "Long"))
        return DurationShortLongType::Long;
    else
        throw GameException("Unrecognized DurationShortLongType \"" + str + "\"");
}