/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-02-04
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Colors.h"

#include <iomanip>
#include <sstream>

rgbColor rgbColor::fromString(std::string const & str)
{
    std::stringstream ss(str);
    ss << std::hex << std::setfill('0') << std::setw(2);

    unsigned int r, g, b;
    ss >> r >> g >> b;

    return rgbColor(
        static_cast<rgbColor::data_type>(r),
        static_cast<rgbColor::data_type>(g),
        static_cast<rgbColor::data_type>(b));
}

std::string rgbColor::toString() const
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<unsigned int>(r)
        << ' '
        << static_cast<unsigned int>(g)
        << ' '
        << static_cast<unsigned int>(b);

    return ss.str();
}

rgbaColor rgbaColor::fromString(std::string const & str)
{
    std::stringstream ss(str);
    ss << std::hex << std::setfill('0') << std::setw(2);

    unsigned int r, g, b, a;
    ss >> r >> g >> b >> a;

    return rgbaColor(
        static_cast<rgbaColor::data_type>(r),
        static_cast<rgbaColor::data_type>(g),
        static_cast<rgbaColor::data_type>(b),
        static_cast<rgbaColor::data_type>(a));
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