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
#include <utility>

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define APPLICATION_VERSION_MAJOR               1
#define APPLICATION_VERSION_MINOR               19
#define APPLICATION_VERSION_PATCH               1
#define APPLICATION_VERSION_BUILD               3

#define APPLICATION_VERSION_LONG_STR    STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_PATCH)    \
                                        "." STRINGIZE(APPLICATION_VERSION_BUILD)

#define APPLICATION_VERSION_SHORT_STR   STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_PATCH)

#define APPLICATION_NAME                     "Floating Sandbox"
#define APPLICATION_NAME_WITH_SHORT_VERSION  APPLICATION_NAME " " APPLICATION_VERSION_SHORT_STR
#define APPLICATION_NAME_WITH_LONG_VERSION   APPLICATION_NAME " " APPLICATION_VERSION_LONG_STR

#define APPLICATION_DOWNLOAD_URL        "https://gamejolt.com/games/floating-sandbox/353572"

inline std::string ApplicationName = APPLICATION_NAME;

//////////////////////////////////////////////////////////////

#pragma pack(push, 1)

class Version
{
public:

    static Version CurrentVersion()
    {
        return Version(
            APPLICATION_VERSION_MAJOR,
            APPLICATION_VERSION_MINOR,
            APPLICATION_VERSION_PATCH,
            APPLICATION_VERSION_BUILD);
    }

    static Version Zero()
    {
        return Version(
            0,
            0,
            0,
            0);
    }

    Version(
        int major,
        int minor,
        int patch,
        int build)
        : mMajor(major)
        , mMinor(minor)
        , mPatch(patch)
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
            && l.mPatch == r.mPatch
            && l.mBuild == r.mBuild;
    }

    friend inline bool operator!=(Version const & l, Version const & r)
    {
        return !(l == r);
    }

    friend inline bool operator<(Version const & l, Version const & r)
    {
        return std::tie(l.mMajor, l.mMinor, l.mPatch, l.mBuild)
            < std::tie(r.mMajor, r.mMinor, r.mPatch, r.mBuild);
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

    int GetMajor() const
    {
        return mMajor;
    }

    int GetMinor() const
    {
        return mMinor;
    }

    int GetPatch() const
    {
        return mPatch;
    }

    int GetBuild() const
    {
        return mBuild;
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
            int patch = std::stoi(versionMatch[3].str());
            int build = (versionMatch[4].matched)
                ? std::stoi(versionMatch[4].str())
                : 0;

            return Version(major, minor, patch, build);
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
            << mPatch << "."
            << mBuild;

        return ss.str();
    }

    std::string ToMajorMinorPatchString() const
    {
        std::stringstream ss;

        ss
            << mMajor << "."
            << mMinor << "."
            << mPatch;

        return ss.str();
    }

private:

    int mMajor;
    int mMinor;
    int mPatch;
    int mBuild;
};

#pragma pack(pop)
