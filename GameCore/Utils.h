/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Colors.h"
#include "GameException.h"
#include "Vectors.h"

#include <picojson.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <string>

namespace Utils
{
    ////////////////////////////////////////////////////////
    // JSON
    ////////////////////////////////////////////////////////

    picojson::value ParseJSONFile(std::filesystem::path const & filepath);

    picojson::value ParseJSONStream(std::istream const & stream);

    picojson::value ParseJSONString(std::string const & jsonString);

    void SaveJSONFile(
        picojson::value const & value,
        std::filesystem::path const & filepath);

    template<typename T>
    inline T const & GetJsonValueAs(
        picojson::value const & value,
        std::string const & memberName)
    {
        if (!value.is<T>())
        {
            throw GameException("Error parsing JSON: member \"" + memberName + "\" is not of the expected type");
        }

        return value.get<T>();
    }

    template<typename T>
    inline T GetOptionalJsonMember(
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
            throw GameException("Error parsing JSON: member \"" + memberName + "\" is not of the expected type");
        }

        return memberIt->second.get<T>();
    }

    template<>
    inline float GetOptionalJsonMember<float>(
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
    inline int GetOptionalJsonMember<int>(
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
    inline std::optional<T> GetOptionalJsonMember(
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
            throw GameException("Error parsing JSON: member \"" + memberName + "\" is not of the expected type");
        }

        return std::make_optional<T>(memberIt->second.get<T>());
    }

    template<>
    inline std::optional<float> GetOptionalJsonMember<float>(
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
    inline std::optional<int> GetOptionalJsonMember<int>(
        picojson::object const & obj,
        std::string const & memberName)
    {
        auto r = GetOptionalJsonMember<int64_t>(obj, memberName);
        if (r)
            return static_cast<int>(*r);
        else
            return std::nullopt;
    }

    inline std::optional<picojson::object> GetOptionalJsonObject(
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
            throw GameException("Error parsing JSON: member \"" + memberName + "\" is not of type 'object'");
        }

        return memberIt->second.get<picojson::object>();
    }

    template<typename T>
    inline T GetMandatoryJsonMember(
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
            throw GameException("Error parsing JSON: member \"" + memberName + "\" is not of the expected type");
        }

        return memberIt->second.get<T>();
    }

    template<>
    inline float GetMandatoryJsonMember<float>(
        picojson::object const & obj,
        std::string const & memberName)
    {
        return static_cast<float>(
            Utils::GetMandatoryJsonMember<double>(
                obj,
                memberName));
    }

    template<>
    inline int GetMandatoryJsonMember<int>(
        picojson::object const & obj,
        std::string const & memberName)
    {
        return static_cast<int>(
            Utils::GetMandatoryJsonMember<std::uint64_t>(
                obj,
                memberName));
    }

    inline picojson::object GetMandatoryJsonObject(
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
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of type 'object'");
        }

        return memberIt->second.get<picojson::object>();
    }

    inline picojson::array GetMandatoryJsonArray(
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
            throw GameException("Error parsing JSON: requested member \"" + memberName + "\" is not of type 'array'");
        }

        return memberIt->second.get<picojson::array>();
    }

    ////////////////////////////////////////////////////////
    // String
    ////////////////////////////////////////////////////////

    inline std::string Trim(std::string const & str)
    {
        std::string str2 = str;
        str2.erase(str2.begin(), std::find_if(str2.begin(), str2.end(), [](int ch) {
            return !std::isspace(ch);
        }));

        return str2;
    }

    inline std::string ToLower(std::string const & str)
    {
        std::string lstr = str;
        std::transform(
            lstr.begin(),
            lstr.end(),
            lstr.begin(),
            [](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

        return lstr;
    }

    inline bool CaseInsensitiveEquals(std::string const & str1, std::string const & str2)
    {
        if (str1.length() != str2.length())
            return false;

        for (size_t i = 0; i < str1.length(); ++i)
            if (std::tolower(str1[i]) != std::tolower(str2[i]))
                return false;

        return true;
    }

    template <typename TIterable>
    inline std::string Join(TIterable const & elements, std::string const & separator)
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

    inline uint8_t Hex2Byte(std::string const & str)
    {
        std::stringstream ss;
        ss << std::hex << str;
        int x;
        ss >> x;

        return static_cast<uint8_t>(x);
    }

    inline std::string Byte2Hex(uint8_t byte)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(byte);

        return ss.str();
    }

    inline rgbColor Hex2RgbColor(std::string str)
    {
        if (str[0] == '#')
            str = str.substr(1);

        if (str.length() != 6)
            throw GameException("Error: badly formed hex color value \"" + str + "\"");

        return rgbColor(
            Hex2Byte(str.substr(0, 2)),
            Hex2Byte(str.substr(2, 2)),
            Hex2Byte(str.substr(4, 2)));
    }

    inline std::string RgbColor2Hex(rgbColor const & rgbColor)
    {
        return std::string("#") + Byte2Hex(rgbColor.r) + Byte2Hex(rgbColor.g) + Byte2Hex(rgbColor.b);
    }

    template<typename TValue>
    inline bool LexicalCast(
        std::string const & str,
        TValue * outValue)
    {
        std::istringstream iss;
        iss.unsetf(std::ios::skipws);

        iss.str(str);

        TValue value;
        iss >> value;

        if (iss.bad() || iss.get() != EOF)
            return false;

        *outValue = value;
        return true;
    }

    template<>
    inline bool LexicalCast(
        std::string const & str,
        uint8_t * outValue)
    {
        std::istringstream iss;
        iss.unsetf(std::ios::skipws);

        iss.str(str);

        int value;
        iss >> value;

        if (iss.bad() || iss.get() != EOF || value < 0 || value > std::numeric_limits<uint8_t>::max())
            return false;

        *outValue = static_cast<uint8_t>(value);
        return true;
    }

    inline std::string FindAndReplaceAll(std::string const & str, std::string const & search, std::string const & replace)
    {
        std::string res = str;

        auto pos = res.find(search);
        while (pos != std::string::npos)
        {
            res.replace(pos, search.size(), replace);
            pos = res.find(search, pos + replace.size());
        }

        return res;
    }

    inline std::regex MakeFilenameMatchRegex(std::string const & pattern)
    {
        std::string regexPattern = FindAndReplaceAll(pattern, ".", "\\.");
        regexPattern = FindAndReplaceAll(regexPattern, "*", ".*");

        return std::regex(regexPattern, std::regex_constants::icase);
    }

    inline std::string MakeNowDateAndTimeString()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y%m%d%H%M%S");

        return ss.str();
    }

    ////////////////////////////////////////////////////////
    // Text files
    ////////////////////////////////////////////////////////

    inline std::string LoadTextFile(std::filesystem::path const & filepath)
    {
        std::ifstream file(filepath.string(), std::ios::in);
        if (!file.is_open())
        {
            throw GameException("Cannot open file \"" + filepath.string() + "\"");
        }

        std::stringstream ss;
        ss << file.rdbuf();

        return ss.str();
    }

    inline std::string LoadTextStream(std::istream const & stream)
    {
        std::stringstream ss;
        ss << stream.rdbuf();

        return ss.str();
    }

    inline void SaveTextFile(
        std::string const & content,
        std::filesystem::path const & filepath)
    {
        auto const directoryPath = filepath.parent_path();
        if (!std::filesystem::exists(directoryPath))
            std::filesystem::create_directories(directoryPath);

        std::ofstream file(filepath.string(), std::ios::out);
        if (!file.is_open())
        {
            throw GameException("Cannot open file \"" + filepath.string() + "\"");
        }

        file << content;
    }
};
