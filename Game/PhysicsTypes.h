/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Vectors.h>

//
// Wind field (interactive)
//

struct WindField
{
    vec2f FieldCenterPos;
    float FieldRadius;
    float WindSpeed;

    WindField(
        vec2f const & fieldCenterPos,
        float fieldRadius,
        float windSpeed)
        : FieldCenterPos(fieldCenterPos)
        , FieldRadius(fieldRadius)
        , WindSpeed(windSpeed)
    {}
};