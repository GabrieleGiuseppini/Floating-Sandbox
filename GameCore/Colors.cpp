/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-02-04
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Colors.h"

#include "GameException.h"

#include <iomanip>
#include <sstream>

rgbColor rgbColor::fromString(std::string const & str)
{
    unsigned int components[3];
    size_t iStart = 0;
    for (size_t c = 0; c < 3; ++c)
    {
        // Skip spaces
        while (iStart < str.length() && str[iStart] == ' ')
        {
            ++iStart;
        }

        if (iStart >= str.length())
        {
            throw GameException("RGB color string \"" + str + "\" is invalid");
        }

        // Find next 3rd char, space, or eos
        size_t iEnd = iStart + 1;
        if (iEnd < str.length()
            && str[iEnd] != ' ')
        {
            ++iEnd;
        }

        std::stringstream ss;
        ss << std::hex << str.substr(iStart, iEnd - iStart);
        ss >> components[c];

        iStart = iEnd;
    }

    return rgbColor(
        static_cast<rgbColor::data_type>(components[0]),
        static_cast<rgbColor::data_type>(components[1]),
        static_cast<rgbColor::data_type>(components[2]));
}

std::string rgbColor::toString() const
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0')
        << std::setw(2) << static_cast<unsigned int>(r)
        << std::setw(2) << static_cast<unsigned int>(g)
        << std::setw(2) << static_cast<unsigned int>(b);

    return ss.str();
}

rgbaColor rgbaColor::fromString(std::string const & str)
{
    unsigned int components[4];
    size_t iStart = 0;
    for (size_t c = 0; c < 4; ++c)
    {
        // Skip spaces
        while (iStart < str.length() && str[iStart] == ' ')
        {
            ++iStart;
        }

        if (iStart >= str.length())
        {
            throw GameException("RGBA color string \"" + str + "\" is invalid");
        }

        // Find next 3rd char, space, or eos
        size_t iEnd = iStart + 1;
        if (iEnd < str.length()
            && str[iEnd] != ' ')
        {
            ++iEnd;
        }

        std::stringstream ss;
        ss << std::hex << str.substr(iStart, iEnd - iStart);
        ss >> components[c];

        iStart = iEnd;
    }

    return rgbaColor(
        static_cast<rgbColor::data_type>(components[0]),
        static_cast<rgbColor::data_type>(components[1]),
        static_cast<rgbColor::data_type>(components[2]),
        static_cast<rgbColor::data_type>(components[3]));
}

std::string rgbaColor::toString() const
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0')
        << std::setw(2) << static_cast<unsigned int>(r)
        << std::setw(2) << static_cast<unsigned int>(g)
        << std::setw(2) << static_cast<unsigned int>(b)
        << std::setw(2) << static_cast<unsigned int>(a);

    return ss.str();
}