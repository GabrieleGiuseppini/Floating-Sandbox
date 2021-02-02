/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-01-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>
#include <optional>

struct BootSettings
{
public:

    std::optional<bool> DoForceNoGlFinish;
    std::optional<bool> DoForceNoMultithreadedRendering;

    BootSettings()
        : DoForceNoGlFinish()
        , DoForceNoMultithreadedRendering()
    {}

    BootSettings(
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering)
        : DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
    {}

    bool operator==(BootSettings const & rhs) const
    {
        return this->DoForceNoGlFinish == rhs.DoForceNoGlFinish
            && this->DoForceNoMultithreadedRendering == rhs.DoForceNoMultithreadedRendering;
    }

public:

    static BootSettings Load(std::filesystem::path const & filePath);
    static void Save(BootSettings const & settings, std::filesystem::path const & filePath);
};