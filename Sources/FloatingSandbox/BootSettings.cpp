/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-01-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "BootSettings.h"

#include <Game/GameAssetManager.h>
#include <Game/GameVersion.h>

#include <Core/Utils.h>

BootSettings BootSettings::Load(std::filesystem::path const & filePath)
{
    BootSettings settings;

    try
    {
        auto const rootValue = GameAssetManager::LoadJson(filePath);
        if (rootValue.is<picojson::object>())
        {
            auto const rootObject = rootValue.get<picojson::object>();

            auto const version = Utils::GetMandatoryJsonMember<std::string>(rootObject, "version");
            if (version == CurrentGameVersion.ToString()) // Boot settings are only valid on the version they're created for
            {
                settings.DoForceNoGlFinish = Utils::GetOptionalJsonMember<bool>(rootObject, "force_no_glfinish");
                settings.DoForceNoMultithreadedRendering = Utils::GetOptionalJsonMember<bool>(rootObject, "force_no_multithreaded_rendering");
            }
        }
    }
    catch (...)
    {
        // Ignore
    }

    return settings;
}

void BootSettings::Save(
    BootSettings const & settings,
    std::filesystem::path const & filePath)
{
    picojson::object rootObject;

    rootObject["version"] = picojson::value(CurrentGameVersion.ToString());

    if (settings.DoForceNoGlFinish.has_value())
        rootObject["force_no_glfinish"] = picojson::value(*(settings.DoForceNoGlFinish));

    if (settings.DoForceNoMultithreadedRendering.has_value())
        rootObject["force_no_multithreaded_rendering"] = picojson::value(*(settings.DoForceNoMultithreadedRendering));

    // Save
    GameAssetManager::SaveJson(
        picojson::value(rootObject),
        filePath);
}