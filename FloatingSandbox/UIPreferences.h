/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
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
        // We always have the default ship directory in the first position
        assert(mShipLoadDirectories.size() >= 1);

        if (shipLoadDirectory != mShipLoadDirectories[0])
        {
            // Check if we have one already
            for (auto it = mShipLoadDirectories.begin(); it != mShipLoadDirectories.end(); ++it)
            {
                if (*it == shipLoadDirectory)
                {
                    // Move it to second place
                    std::rotate(mShipLoadDirectories.begin() + 1, it, it + 1);

                    return;
                }
            }

            // Add to second place
            mShipLoadDirectories.insert(mShipLoadDirectories.begin() + 1, shipLoadDirectory);
        }
    }

private:

    std::vector<std::filesystem::path> mShipLoadDirectories;
};
