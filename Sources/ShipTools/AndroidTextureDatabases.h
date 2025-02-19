/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-02-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <string>

//
// All the texture databases in the Android port. This file is to be kept in-sync
// (manually) with the Android project, so that we can bake that project's atlases.
//

namespace AndroidTextureDatabases {

// UI

enum class UITextureGroups : uint16_t
{
    ToolsIcon = 0,
    SwipeIcon = 1,
    SettingsIcon = 2,

    _Last = SettingsIcon
};

struct UITextureDatabase
{
    static inline std::string DatabaseName = "ui";

    using TextureGroupsType = UITextureGroups;

    static UITextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "tools_icon"))
            return UITextureGroups::ToolsIcon;
        else if (Utils::CaseInsensitiveEquals(str, "swipe_icon"))
            return UITextureGroups::SwipeIcon;
        else if (Utils::CaseInsensitiveEquals(str, "settings_icon"))
            return UITextureGroups::SettingsIcon;
        else
            throw GameException("Unrecognized UI texture group \"" + str + "\"");
    }
};

}