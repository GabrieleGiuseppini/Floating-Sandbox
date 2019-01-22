/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

class UISettings
{
public:

    UISettings();

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
