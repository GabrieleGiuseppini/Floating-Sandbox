/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-02-04
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Colors.h"

#include <iomanip>
#include <sstream>

std::string rgbColor::toString() const
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<unsigned int>(r)
        << static_cast<unsigned int>(g)
        << static_cast<unsigned int>(b);

    return ss.str();
}

std::string rgbaColor::toString() const
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<unsigned int>(r)
        << static_cast<unsigned int>(g)
        << static_cast<unsigned int>(b)
        << static_cast<unsigned int>(a);

    return ss.str();
}