/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2024-02-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "BarycentricCoords.h"

#include <iomanip>
#include <sstream>

std::string bcoords3f::toString() const
{
    std::stringstream ss;
    ss << std::setprecision(12) << "(" << coords[0] << ", " << coords[1] << ", " << coords[2] << ")";
    return ss.str();
}
