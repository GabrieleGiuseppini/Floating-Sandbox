/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

/*
 * UI settings managed by the SettingsManager class.
 */
class UISettings
{
public:

    UISettings();

public:

    std::filesystem::path const & GetScreenshotsFolderPath() const
    {
        return mScreenshotsFolderPath;
    }

    void SetScreenshotsFolderPath(std::filesystem::path screenshotsFolderPath)
    {
        mScreenshotsFolderPath = std::move(screenshotsFolderPath);
    }

private:

    std::filesystem::path mScreenshotsFolderPath;
};
