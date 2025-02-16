/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-07-18
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameTypes.h"

#include "GameException.h"
#include "Utils.h"

#include <chrono>

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

SessionId SessionId::New()
{
    static std::uint64_t currentValue = 0;

    return SessionId((++currentValue));
}

std::string NpcKindTypeToStr(NpcKindType npcKind)
{
    switch (npcKind)
    {
        case NpcKindType::Furniture:
        {
            return "Furniture";
        }

        case NpcKindType::Human:
        {
            return "Human";
        }
    }

    assert(false);
    return "";
}

NpcHumanRoleType StrToNpcHumanRoleType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Captain"))
        return NpcHumanRoleType::Captain;
    else if (Utils::CaseInsensitiveEquals(str, "Crew"))
        return NpcHumanRoleType::Crew;
    else if (Utils::CaseInsensitiveEquals(str, "Other"))
        return NpcHumanRoleType::Other;
    else if (Utils::CaseInsensitiveEquals(str, "Passenger"))
        return NpcHumanRoleType::Passenger;
    else
        throw GameException("Unrecognized NpcHumanRoleType \"" + str + "\"");
}

NpcFurnitureRoleType StrToNpcFurnitureRoleType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Furniture"))
        return NpcFurnitureRoleType::Furniture;
    else if (Utils::CaseInsensitiveEquals(str, "Other"))
        return NpcFurnitureRoleType::Other;
    else
        throw GameException("Unrecognized NpcFurnitureRoleType \"" + str + "\"");
}

DurationShortLongType StrToDurationShortLongType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Short"))
        return DurationShortLongType::Short;
    else if (Utils::CaseInsensitiveEquals(str, "Long"))
        return DurationShortLongType::Long;
    else
        throw GameException("Unrecognized DurationShortLongType \"" + str + "\"");
}