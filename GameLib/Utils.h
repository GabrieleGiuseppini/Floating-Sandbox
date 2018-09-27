/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameException.h"
#include "Vectors.h"

#include <picojson/picojson.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>

class Utils 
{
public:

    //
    // JSON
    //

    static picojson::value ParseJSONFile(std::string const & filename);

    template<typename T>
    static T GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName,
        T const & defaultValue)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            return defaultValue;
        }

        if (!memberIt->second.is<T>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the specified type");
        }

        return memberIt->second.get<T>();
    }

    template<>
    static float GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName,
        float const & defaultValue)
    {
        double defaultValueDouble = static_cast<double>(defaultValue);

        return static_cast<float>(
            GetOptionalJsonMember<double>(
                obj,
                memberName,
                defaultValueDouble));
    }

    template<>
    static int GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName,
        int const & defaultValue)
    {
        int64_t defaultValueInt64 = static_cast<int64_t>(defaultValue);

        return static_cast<int>(
            GetOptionalJsonMember<int64_t>(
                obj,
                memberName,
                defaultValueInt64));
    }

    template<typename T>
    static std::optional<T> GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            return std::nullopt;
        }

        if (!memberIt->second.is<T>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the specified type");
        }

        return std::make_optional<T>(memberIt->second.get<T>());
    }

    template<>
    static std::optional<float> GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto r = GetOptionalJsonMember<double>(obj, memberName);
        if (r)
            return static_cast<float>(*r);
        else
            return std::nullopt;
    }

    template<>
    static std::optional<int> GetOptionalJsonMember(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto r = GetOptionalJsonMember<int64_t>(obj, memberName);
        if (r)
            return static_cast<int>(*r);
        else
            return std::nullopt;
    }

    static std::optional<picojson::object> GetOptionalJsonObject(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            return std::nullopt;
        }

        if (!memberIt->second.is<picojson::object>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the object type");
        }

        return memberIt->second.get<picojson::object>();
    }

    template<typename T>
    static T const GetMandatoryJsonMember(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            throw GameException("Error parsing JSON: cannot find member \"" + memberName + "\"");
        }

        if (!memberIt->second.is<T>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the specified type");
        }

        return memberIt->second.get<T>();
    }

    static picojson::object GetMandatoryJsonObject(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            throw GameException("Error parsing JSON: cannot find member \"" + memberName + "\"");
        }

        if (!memberIt->second.is<picojson::object>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the object type");
        }

        return memberIt->second.get<picojson::object>();
    }

    static picojson::array GetMandatoryJsonArray(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto const & memberIt = obj.find(memberName);
        if (obj.end() == memberIt)
        {
            throw GameException("Error parsing JSON: cannot find member \"" + memberName + "\"");
        }

        if (!memberIt->second.is<picojson::array>())
        {
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of the array type");
        }

        return memberIt->second.get<picojson::array>();
    }

    //
    // String
    //

    static std::string ToLower(std::string const & str)
    {
        std::string lstr = str;
        std::transform(
            lstr.begin(),
            lstr.end(),
            lstr.begin(),
            [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

        return lstr;
    }

    template <typename TIterable>
    static std::string Join(TIterable const & elements, std::string const & separator)
    {
        std::stringstream ss;
        bool first = true;
        for (auto const & str : elements)
        {
            if (!first)
                ss << separator;
            ss << str;

            first = false;
        }

        return ss.str();
    }

    static uint8_t Hex2Byte(std::string const & str)
    {
        std::stringstream ss;
        ss << std::hex << str;
        int x;
        ss >> x;

        return static_cast<uint8_t>(x);
    }

    static std::string Byte2Hex(uint8_t byte)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);

        return ss.str();
    }

    static std::array<uint8_t, 3u> Hex2RgbColour(std::string str)
    {
        if (str[0] == '#')
            str = str.substr(1);

        if (str.length() != 6)
            throw GameException("Error: badly formed hex colour value \"" + str + "\"");

        std::array<uint8_t, 3u> rgbColour;
        rgbColour[0] = Hex2Byte(str.substr(0, 2));
        rgbColour[1] = Hex2Byte(str.substr(2, 2));
        rgbColour[2] = Hex2Byte(str.substr(4, 2));

        return rgbColour;
    }

    static std::string RgbColour2Hex(std::array<uint8_t, 3u> const & rgbColour)
    {
        return std::string("#") + Byte2Hex(rgbColour[0]) + Byte2Hex(rgbColour[1]) + Byte2Hex(rgbColour[2]);
    }

    static vec3f RgbToVec(std::array<uint8_t, 3u> const & rgbColour)
    {
        return vec3f(
            rgbColour[0] / 255.f,
            rgbColour[1] / 255.f,
            rgbColour[2] / 255.f);
    }
};