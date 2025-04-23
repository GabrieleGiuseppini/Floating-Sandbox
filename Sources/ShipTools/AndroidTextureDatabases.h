/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-26
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Utils.h>

#include <string>

namespace UITextureDatabases {

enum class UITextureGroups : uint16_t
{
    // Icons
    DotDotDotIcon = 0,
    ShipSelectionIcon,
    ToolsIcon,
    SettingsIcon,
    TimeOfDayIcon,
    ViewIcon,
    ExitIcon,
    CheckboxFrame,
    ToolsEyeQuadrant,
    SliderGuideTip,
    SliderGuideBody,
    SliderKnob,
    ToolIcons,
    TriangleH,
    TriangleV,
    UnderContructionIcon,

    _Last = UnderContructionIcon
};

struct UITextureDatabase
{
    static inline std::string DatabaseName = "ui";

    using TextureGroupsType = UITextureGroups;

    static UITextureGroups StrToTextureGroup(std::string const & str)
    {
        if (Utils::CaseInsensitiveEquals(str, "dot_dot_dot_icon"))
            return UITextureGroups::DotDotDotIcon;
        else if (Utils::CaseInsensitiveEquals(str, "ship_selection_icon"))
            return UITextureGroups::ShipSelectionIcon;
        else if (Utils::CaseInsensitiveEquals(str, "tools_icon"))
            return UITextureGroups::ToolsIcon;
        else if (Utils::CaseInsensitiveEquals(str, "settings_icon"))
            return UITextureGroups::SettingsIcon;
        else if (Utils::CaseInsensitiveEquals(str, "time_of_day_icon"))
            return UITextureGroups::TimeOfDayIcon;
        else if (Utils::CaseInsensitiveEquals(str, "view_icon"))
            return UITextureGroups::ViewIcon;
        else if (Utils::CaseInsensitiveEquals(str, "exit_icon"))
            return UITextureGroups::ExitIcon;
        else if (Utils::CaseInsensitiveEquals(str, "checkbox_frame"))
            return UITextureGroups::CheckboxFrame;
        else if (Utils::CaseInsensitiveEquals(str, "tools_eye_quadrant"))
            return UITextureGroups::ToolsEyeQuadrant;
        else if (Utils::CaseInsensitiveEquals(str, "slider_guide_tip"))
            return UITextureGroups::SliderGuideTip;
        else if (Utils::CaseInsensitiveEquals(str, "slider_guide_body"))
            return UITextureGroups::SliderGuideBody;
        else if (Utils::CaseInsensitiveEquals(str, "slider_knob"))
            return UITextureGroups::SliderKnob;
        else if (Utils::CaseInsensitiveEquals(str, "tool_icons"))
            return UITextureGroups::ToolIcons;
        else if (Utils::CaseInsensitiveEquals(str, "triangle_h"))
            return UITextureGroups::TriangleH;
        else if (Utils::CaseInsensitiveEquals(str, "triangle_v"))
            return UITextureGroups::TriangleV;
        else if (Utils::CaseInsensitiveEquals(str, "under_construction_icon"))
            return UITextureGroups::UnderContructionIcon;
        else
            throw GameException("Unrecognized UI texture group \"" + str + "\"");
    }
};

}