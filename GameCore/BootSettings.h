/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-01-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

struct BootSettings
{
public:

    bool DoForceNoGlFinish;
    bool DoForceNoMultithreadedRendering;

    // Defaults are here
    BootSettings()
        : DoForceNoGlFinish(false)
        , DoForceNoMultithreadedRendering(false)
    {}

    BootSettings(
        bool doForceNoGlFinish,
        bool doForceNoMultithreadedRendering)
        : DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
    {}

public:

    static BootSettings Load(std::filesystem::path const & filePath);
    static void Save(BootSettings const & settings, std::filesystem::path const & filePath);
};