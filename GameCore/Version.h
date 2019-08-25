/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define APPLICATION_VERSION_MAJOR               1
#define APPLICATION_VERSION_MINOR               13
#define APPLICATION_VERSION_PATCH               0
#define APPLICATION_VERSION_BETA                1

#define APPLICATION_VERSION_SHORT_STR   STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_PATCH)

#if APPLICATION_VERSION_BETA != 0
#define APPLICATION_VERSION_STR         APPLICATION_VERSION_SHORT_STR               \
                                        ".beta" STRINGIZE(APPLICATION_VERSION_BETA)
#else
#define APPLICATION_VERSION_STR         APPLICATION_VERSION_SHORT_STR
#endif

#define APPLICATION_NAME                "Floating Sandbox"
#define APPLICATION_NAME_WITH_VERSION   APPLICATION_NAME " " APPLICATION_VERSION_STR

#define APPLICATION_DOWNLOAD_URL        "https://gamejolt.com/games/floating-sandbox/353572"

inline std::string ApplicationName = APPLICATION_NAME;
inline std::string ApplicationNameWithVersion = APPLICATION_NAME_WITH_VERSION;

enum class VersionFormat
{
    Short,
    Long,
    LongWithDate
};

inline std::string GetVersionInfo(VersionFormat versionFormat)
{
    switch (versionFormat)
    {
        case VersionFormat::Short:
        {
            return std::string(APPLICATION_VERSION_STR);
        }

        case VersionFormat::Long:
        {
            return std::string(APPLICATION_NAME_WITH_VERSION);
        }

        case VersionFormat::LongWithDate:
        {
            return std::string(APPLICATION_NAME_WITH_VERSION " (" __DATE__ ")");
        }

        default:
        {
            assert(false);
            return "";
        }
    }
}

//////////////////////////////////////////////////////////////

class Version
{
public:

    static Version CurrentVersion()
    {
        return Version(
            APPLICATION_VERSION_MAJOR,
            APPLICATION_VERSION_MINOR,
            APPLICATION_VERSION_PATCH,
            APPLICATION_VERSION_BETA);
    }

    Version(
        int major,
        int minor,
        int patch,
        int beta)
        : mMajor(major)
        , mMinor(minor)
        , mPatch(patch)
        , mBeta(beta)
    {}

    Version(Version const & other) = default;
    Version(Version && other) = default;

    Version & operator=(Version const & other) = default;
    Version & operator=(Version && other) = default;

    friend inline bool operator==(Version const & l, Version const & r)
    {
        return l.mMajor == r.mMajor
            && l.mMinor == r.mMinor
            && l.mPatch == r.mPatch
            && l.mBeta == r.mBeta;
    }

    friend inline bool operator!=(Version const & l, Version const & r)
    {
        return !(l == r);
    }

    friend inline bool operator<(Version const & l, Version const & r)
    {
        // Rule: (beta = 0) is always > (beta > 0)
        return std::tie(l.mMajor, l.mMinor, l.mPatch) < std::tie(r.mMajor, r.mMinor, r.mPatch)
            || (std::tie(l.mMajor, l.mMinor, l.mPatch) == std::tie(r.mMajor, r.mMinor, r.mPatch)
                && l.mBeta > 0 // if it were 0 then l would always be >= r
                && (r.mBeta == 0 || l.mBeta < r.mBeta));
    }

    friend inline bool operator>(Version const & l, Version const & r)
    {
        return r < l;
    }

    friend inline bool operator<=(Version const & l, Version const & r)
    {
        return !(l > r);
    }

    friend inline bool operator>=(Version const & l, Version const & r)
    {
        return !(l < r);
    }

    static Version FromString(std::string const & str)
    {
        static std::regex VersionRegex(R"(^\s*(\d+)\.(\d+)\.(\d+)(?:\.beta(\d+))?\s*$)");
        std::smatch versionMatch;
        if (std::regex_match(str, versionMatch, VersionRegex))
        {
            assert(versionMatch.size() == 1 + 4);

            unsigned int major = std::stoul(versionMatch[1].str());
            unsigned int minor = std::stoul(versionMatch[2].str());
            unsigned int patch = std::stoul(versionMatch[3].str());
            unsigned int beta = (versionMatch[4].matched)
                ? std::stoul(versionMatch[4].str())
                : 0;

            return Version(major, minor, patch, beta);
        }
        else
        {
            throw std::runtime_error("Invalid version: " + str);
        }
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss
            << mMajor << "."
            << mMinor << "."
            << mPatch;

        if (mBeta > 0)
            ss << ".beta" << mBeta;

        return ss.str();
    }

private:

    unsigned int mMajor;
    unsigned int mMinor;
    unsigned int mPatch;
    unsigned int mBeta;
};