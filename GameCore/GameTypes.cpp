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

TextureGroupType StrToTextureGroupType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "AirBubble"))
        return TextureGroupType::AirBubble;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombArmor"))
        return TextureGroupType::AntiMatterBombArmor;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphereCloud"))
        return TextureGroupType::AntiMatterBombSphereCloud;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombSphere"))
        return TextureGroupType::AntiMatterBombSphere;
    else if (Utils::CaseInsensitiveEquals(str, "Cloud"))
        return TextureGroupType::Cloud;
    else if (Utils::CaseInsensitiveEquals(str, "ImpactBomb"))
        return TextureGroupType::ImpactBomb;
    else if (Utils::CaseInsensitiveEquals(str, "Land"))
        return TextureGroupType::Land;
    else if (Utils::CaseInsensitiveEquals(str, "PinnedPoint"))
        return TextureGroupType::PinnedPoint;
    else if (Utils::CaseInsensitiveEquals(str, "RCBomb"))
        return TextureGroupType::RcBomb;
    else if (Utils::CaseInsensitiveEquals(str, "RCBombExplosion"))
        return TextureGroupType::RcBombExplosion;
    else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
        return TextureGroupType::RcBombPing;
    else if (Utils::CaseInsensitiveEquals(str, "SawSparkle"))
        return TextureGroupType::SawSparkle;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBomb"))
        return TextureGroupType::TimerBomb;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombDefuse"))
        return TextureGroupType::TimerBombDefuse;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombExplosion"))
        return TextureGroupType::TimerBombExplosion;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombFuse"))
        return TextureGroupType::TimerBombFuse;
    else if (Utils::CaseInsensitiveEquals(str, "Water"))
        return TextureGroupType::Water;
    else
        throw GameException("Unrecognized TextureGroupType \"" + str + "\"");
}