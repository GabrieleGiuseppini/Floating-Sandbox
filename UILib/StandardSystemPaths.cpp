/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "StandardSystemPaths.h"

#include <Game/Version.h>

#include <GameCore/SysSpecifics.h>

#include <wx/stdpaths.h>

StandardSystemPaths * StandardSystemPaths::mSingleInstance = nullptr;

StandardSystemPaths::StandardSystemPaths()
{
    // Ensure directories that we need exist
    auto const userShipFolderPath = GetUserShipFolderPath();
    if (!std::filesystem::exists(userShipFolderPath))
    {
        try
        {
            std::filesystem::create_directories(userShipFolderPath);
        }
        catch (...)
        {
            // Ignore
        }
    }
}

std::filesystem::path StandardSystemPaths::GetUserShipFolderPath() const
{
    auto documentsFolder = wxStandardPaths::Get().GetUserDir(wxStandardPaths::Dir::Dir_Documents);

    return std::filesystem::path(documentsFolder.ToStdString())
        / ApplicationName // Without version - we want this to be sticky across upgrades
        / "Ships";
}

std::filesystem::path StandardSystemPaths::GetUserPicturesGameFolderPath() const
{
    auto picturesFolder = wxStandardPaths::Get().GetUserDir(wxStandardPaths::Dir::Dir_Pictures);

    return std::filesystem::path(picturesFolder.ToStdString())
        / ApplicationName; // Without version - we want this to be sticky across upgrades
}

std::filesystem::path StandardSystemPaths::GetUserGameRootFolderPath() const
{
    auto userFolder = wxStandardPaths::Get().GetUserConfigDir();

    return std::filesystem::path(userFolder.ToStdString())
#if FS_IS_OS_LINUX()
        / ".config"
#endif
        / ApplicationName; // Without version - we want this to be sticky across upgrades
}

std::filesystem::path StandardSystemPaths::GetUserGameSettingsRootFolderPath() const
{
    return GetUserGameRootFolderPath() / "Settings";
}

std::filesystem::path StandardSystemPaths::GetDiagnosticsFolderPath(bool ensureExists) const
{
    auto const folderPath = GetUserGameRootFolderPath() / "Diagnostics";

    if (ensureExists)
        std::filesystem::create_directories(folderPath);

    return folderPath;
}