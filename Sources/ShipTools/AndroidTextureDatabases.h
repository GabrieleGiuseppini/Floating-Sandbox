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
    DotDotDotIcon = 0,
    ShipSelectionIcon,
    ToolsIcon,
    NpcIcon,
    SettingsIcon,
    TimeOfDayIcon,
    ViewIcon,

    CloseIcon,
    ElectricalPanelIcon,
    ExitIcon,
    NpcFurnitureGroupIcon,
    NpcHumanGroupIcon,
    NpcMoveIcon,
    NpcRemoveIcon,
    NpcTurnaroundIcon,
    ReloadIcon,
    RogueWaveIcon,
    TriggersIcon,
    TsunamiIcon,
    UnderConstructionIcon,
    UVModeIcon,

    CheckboxFrame,
    ToolsEyeQuadrant,
    ElecElem_AutomaticSwitch,
    ElecElem_EngineTelegraph,
    ElecElem_Gauge,
    ElecElem_InteractivePushSwitch,
    ElecElem_InteractiveToggleSwitch,
    ElecElem_JetEngineThrottle,
    ElecElem_JetEngineThrust,
    ElecElem_PowerMonitor,
    ElecElem_ShipSoundSwitch,
    ElecElem_WatertightDoor,
    SettingsIcons,
    SliderGuideTip,
    SliderGuideBody,
    SliderKnob,
    StormIcon,
    ToolIcons,
    LaserCannon,
    TriangleH,
    TriangleV,

    _Last = TriangleV
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
        else if (Utils::CaseInsensitiveEquals(str, "npc_icon"))
            return UITextureGroups::NpcIcon;
        else if (Utils::CaseInsensitiveEquals(str, "settings_icon"))
            return UITextureGroups::SettingsIcon;
        else if (Utils::CaseInsensitiveEquals(str, "time_of_day_icon"))
            return UITextureGroups::TimeOfDayIcon;
        else if (Utils::CaseInsensitiveEquals(str, "view_icon"))
            return UITextureGroups::ViewIcon;
        else if (Utils::CaseInsensitiveEquals(str, "close_icon"))
            return UITextureGroups::CloseIcon;
        else if (Utils::CaseInsensitiveEquals(str, "electrical_panel_icon"))
            return UITextureGroups::ElectricalPanelIcon;
        else if (Utils::CaseInsensitiveEquals(str, "exit_icon"))
            return UITextureGroups::ExitIcon;
        else if (Utils::CaseInsensitiveEquals(str, "npc_furniture_group_icon"))
            return UITextureGroups::NpcFurnitureGroupIcon;
        else if (Utils::CaseInsensitiveEquals(str, "npc_human_group_icon"))
            return UITextureGroups::NpcHumanGroupIcon;
        else if (Utils::CaseInsensitiveEquals(str, "npc_move_icon"))
            return UITextureGroups::NpcMoveIcon;
        else if (Utils::CaseInsensitiveEquals(str, "npc_remove_icon"))
            return UITextureGroups::NpcRemoveIcon;
        else if (Utils::CaseInsensitiveEquals(str, "npc_turnaround_icon"))
            return UITextureGroups::NpcTurnaroundIcon;
        else if (Utils::CaseInsensitiveEquals(str, "reload_icon"))
            return UITextureGroups::ReloadIcon;
        else if (Utils::CaseInsensitiveEquals(str, "rogue_wave_icon"))
            return UITextureGroups::RogueWaveIcon;
        else if (Utils::CaseInsensitiveEquals(str, "triggers_icon"))
            return UITextureGroups::TriggersIcon;
        else if (Utils::CaseInsensitiveEquals(str, "tsunami_icon"))
            return UITextureGroups::TsunamiIcon;
        else if (Utils::CaseInsensitiveEquals(str, "under_construction_icon"))
            return UITextureGroups::UnderConstructionIcon;
        else if (Utils::CaseInsensitiveEquals(str, "uv_mode_icon"))
            return UITextureGroups::UVModeIcon;
        else if (Utils::CaseInsensitiveEquals(str, "checkbox_frame"))
            return UITextureGroups::CheckboxFrame;
        else if (Utils::CaseInsensitiveEquals(str, "tools_eye_quadrant"))
            return UITextureGroups::ToolsEyeQuadrant;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_automatic_switch"))
            return UITextureGroups::ElecElem_AutomaticSwitch;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_engine_telegraph"))
            return UITextureGroups::ElecElem_EngineTelegraph;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_gauge"))
            return UITextureGroups::ElecElem_Gauge;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_interactive_push_switch"))
            return UITextureGroups::ElecElem_InteractivePushSwitch;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_interactive_toggle_switch"))
            return UITextureGroups::ElecElem_InteractiveToggleSwitch;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_jet_engine_throttle"))
            return UITextureGroups::ElecElem_JetEngineThrottle;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_jet_engine_thrust"))
            return UITextureGroups::ElecElem_JetEngineThrust;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_power_monitor"))
            return UITextureGroups::ElecElem_PowerMonitor;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_ship_sound_switch"))
            return UITextureGroups::ElecElem_ShipSoundSwitch;
        else if (Utils::CaseInsensitiveEquals(str, "elec_elem_watertight_door"))
            return UITextureGroups::ElecElem_WatertightDoor;
        else if (Utils::CaseInsensitiveEquals(str, "settings_icons"))
            return UITextureGroups::SettingsIcons;
        else if (Utils::CaseInsensitiveEquals(str, "slider_guide_tip"))
            return UITextureGroups::SliderGuideTip;
        else if (Utils::CaseInsensitiveEquals(str, "slider_guide_body"))
            return UITextureGroups::SliderGuideBody;
        else if (Utils::CaseInsensitiveEquals(str, "slider_knob"))
            return UITextureGroups::SliderKnob;
        else if (Utils::CaseInsensitiveEquals(str, "storm_icon"))
            return UITextureGroups::StormIcon;
        else if (Utils::CaseInsensitiveEquals(str, "tool_icons"))
            return UITextureGroups::ToolIcons;
        else if (Utils::CaseInsensitiveEquals(str, "laser_cannon"))
            return UITextureGroups::LaserCannon;
        else if (Utils::CaseInsensitiveEquals(str, "triangle_h"))
            return UITextureGroups::TriangleH;
        else if (Utils::CaseInsensitiveEquals(str, "triangle_v"))
            return UITextureGroups::TriangleV;
        else
            throw GameException("Unrecognized UI texture group \"" + str + "\"");
    }
};

}