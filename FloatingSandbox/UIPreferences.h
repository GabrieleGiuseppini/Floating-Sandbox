/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>
#include <vector>

/*
 * Maintains persistent UI preferences.
 */
class UIPreferences
{
public:

    UIPreferences();

    ~UIPreferences();

public:

    std::vector<std::filesystem::path> const & GetShipLoadDirectories() const
    {
        return mShipLoadDirectories;
    }

    void AddShipLoadDirectory(std::filesystem::path shipLoadDirectory)
    {
        // Check if we have one already
        for (auto it = mShipLoadDirectories.begin(); it != mShipLoadDirectories.end(); ++it)
        {
            if (*it == shipLoadDirectory)
            {
                // Move it to first place
                std::rotate(mShipLoadDirectories.begin(), it, it + 1);

                return;
            }
        }

        // Add to front
        mShipLoadDirectories.insert(mShipLoadDirectories.begin(), shipLoadDirectory);
    }

private:

    std::vector<std::filesystem::path> mShipLoadDirectories;
};
