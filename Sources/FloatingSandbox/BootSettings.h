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
    std::optional<bool> DoForceNoMultiSampling;

    BootSettings()
        : DoForceNoGlFinish()
        , DoForceNoMultithreadedRendering()
        , DoForceNoMultiSampling()
    {}

    BootSettings(
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering,
        std::optional<bool> doForceNoMultiSampling)
        : DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
        , DoForceNoMultiSampling(doForceNoMultiSampling)
    {}

    bool operator==(BootSettings const & rhs) const
    {
        return this->DoForceNoGlFinish == rhs.DoForceNoGlFinish
            && this->DoForceNoMultithreadedRendering == rhs.DoForceNoMultithreadedRendering
            && this->DoForceNoMultiSampling == rhs.DoForceNoMultiSampling;
    }

public:

    static BootSettings Load(std::filesystem::path const & filePath);
    static void Save(BootSettings const & settings, std::filesystem::path const & filePath);
};