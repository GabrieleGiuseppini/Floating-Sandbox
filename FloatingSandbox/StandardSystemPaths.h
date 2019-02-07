/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

class StandardSystemPaths
{
public:

    static StandardSystemPaths & GetInstance()
    {
        if (nullptr == mSingleInstance)
        {
            // Note: we're not multi-threaded, hence no need to lock
            mSingleInstance = new StandardSystemPaths();
        }

        return *mSingleInstance;
    }

    std::filesystem::path GetUserPicturesGameFolderPath() const;

    std::filesystem::path GetUserSettingsGameFolderPath() const;

private:

    StandardSystemPaths()
    {}

    static StandardSystemPaths * mSingleInstance;
};
