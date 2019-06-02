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
#define APPLICATION_VERSION_MINOR               12
#define APPLICATION_VERSION_REVISION            0
#define APPLICATION_VERSION_BUILD               0

#define APPLICATION_VERSION_LONG_STR    STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_REVISION) \
                                        "." STRINGIZE(APPLICATION_VERSION_BUILD)

#define APPLICATION_VERSION_SHORT_STR   STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_REVISION)

#define APPLICATION_NAME                "Floating Sandbox"
#define APPLICATION_NAME_WITH_VERSION   APPLICATION_NAME " " APPLICATION_VERSION_SHORT_STR

#define APPLICATION_DOWNLOAD_PAGE       "https://gamejolt.com/games/floating-sandbox/353572"

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
            return std::string(APPLICATION_VERSION_LONG_STR);
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
            APPLICATION_VERSION_REVISION,
            APPLICATION_VERSION_BUILD);
    }

    Version(
        int major,
        int minor,
        int revision,
        int build)
        : mMajor(major)
        , mMinor(minor)
        , mRevision(revision)
        , mBuild(build)
    {}

    Version(Version const & other) = default;
    Version(Version && other) = default;

    Version & operator=(Version const & other) = default;
    Version & operator=(Version && other) = default;

    friend inline bool operator==(Version const & l, Version const & r)
    {
        return l.mMajor == r.mMajor
            && l.mMinor == r.mMinor
            && l.mRevision == r.mRevision
            && l.mBuild == r.mBuild;
    }

    friend inline bool operator!=(Version const & l, Version const & r)
    {
        return !(l == r);
    }

    friend inline bool operator<(Version const & l, Version const & r)
    {
        return std::tie(l.mMajor, l.mMinor, l.mRevision, l.mBuild)
            < std::tie(r.mMajor, r.mMinor, r.mRevision, r.mBuild);
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
        static std::regex VersionRegex(R"(^\s*(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?\s*$)");
        std::smatch versionMatch;
        if (std::regex_match(str, versionMatch, VersionRegex))
        {
            assert(versionMatch.size() == 1 + 4);

            int major = std::stoi(versionMatch[1].str());
            int minor = std::stoi(versionMatch[2].str());
            int revision = std::stoi(versionMatch[3].str());
            int build = (versionMatch[4].matched)
                ? std::stoi(versionMatch[4].str())
                : 0;

            return Version(major, minor, revision, build);
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
            << mRevision << "."
            << mBuild;

        return ss.str();
    }

private:

    int mMajor;
    int mMinor;
    int mRevision;
    int mBuild;
};