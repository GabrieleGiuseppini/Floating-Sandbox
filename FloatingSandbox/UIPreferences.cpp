/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include "UIPreferences.h"

#include "StandardSystemPaths.h"

#include <Game/ResourceLoader.h>

UIPreferences::UIPreferences()
    : mShipLoadDirectories()
{
    //
    // Set defaults
    //

    mShipLoadDirectories.push_back(
        ResourceLoader::GetInstalledShipFolderPath());

    //
    // Load preferences
    //

    // TODOHERE
}

UIPreferences::~UIPreferences()
{
    //
    // Save preferences
    //

    // TODOHERE
}