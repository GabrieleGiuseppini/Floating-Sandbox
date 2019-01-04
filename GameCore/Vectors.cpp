/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Vectors.h"

#include <sstream>

std::string vec2f::toString() const
{
    std::stringstream ss;
    ss << "(" << x << ", " << y << ")";
    return ss.str();
}

std::string vec3f::toString() const
{
    std::stringstream ss;
    ss << "(" << x << ", " << y << ", " << z << ")";
    return ss.str();
}

std::string vec4f::toString() const
{
    std::stringstream ss;
    ss << "(" << x << ", " << y << ", " << z << ", " << w << ")";
    return ss.str();
}

