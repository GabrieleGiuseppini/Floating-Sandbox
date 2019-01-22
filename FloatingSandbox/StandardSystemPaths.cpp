/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include "StandardSystemPaths.h"

#include "Version.h"

#include <wx/stdpaths.h>

StandardSystemPaths * StandardSystemPaths::mSingleInstance = nullptr;

std::filesystem::path StandardSystemPaths::GetUserPicturesGameFolderPath() const
{
    auto picturesFolder = wxStandardPaths::Get().GetUserDir(wxStandardPaths::Dir::Dir_Pictures);

    return std::filesystem::path(picturesFolder.ToStdString())
        / ApplicationName;
}