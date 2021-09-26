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
            // Note: no real need to lock
            mSingleInstance = new StandardSystemPaths();
        }

        return *mSingleInstance;
    }

    std::filesystem::path GetUserShipFolderPath() const;

    std::filesystem::path GetUserPicturesGameFolderPath() const;

    std::filesystem::path GetUserGameRootFolderPath() const;

    std::filesystem::path GetUserGameSettingsRootFolderPath() const;

    std::filesystem::path GetDiagnosticsFolderPath(bool ensureExists = false) const;

private:

    StandardSystemPaths();

    static StandardSystemPaths * mSingleInstance;
};
