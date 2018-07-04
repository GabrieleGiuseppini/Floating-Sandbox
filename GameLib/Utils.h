/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameException.h"

#include <picojson/picojson.h>

#include <algorithm>
#include <cctype>
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
    static T const & GetOptionalJsonMember(
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
    static T const & GetMandatoryJsonMember(
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
};