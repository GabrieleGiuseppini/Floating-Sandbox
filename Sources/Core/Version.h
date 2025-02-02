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

#pragma pack(push, 1)

class Version final
{
public:

    static constexpr Version Zero()
    {
        return Version(
            0,
            0,
            0,
            0);
    }

    constexpr Version(
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
