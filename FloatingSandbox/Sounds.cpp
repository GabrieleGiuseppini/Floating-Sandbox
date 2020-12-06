/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Sounds.h"

#include <GameCore/Utils.h>

SoundType StrToSoundType(std::string const & str)
{
    if (Utils::CaseInsensitiveEquals(str, "Break"))
        return SoundType::Break;
    else if (Utils::CaseInsensitiveEquals(str, "Destroy"))
        return SoundType::Destroy;
    else if (Utils::CaseInsensitiveEquals(str, "LightningHit"))
        return SoundType::LightningHit;
    else if (Utils::CaseInsensitiveEquals(str, "RepairSpring"))
        return SoundType::RepairSpring;
    else if (Utils::CaseInsensitiveEquals(str, "RepairTriangle"))
        return SoundType::RepairTriangle;
    else if (Utils::CaseInsensitiveEquals(str, "Draw"))
        return SoundType::Draw;
    else if (Utils::CaseInsensitiveEquals(str, "Saw"))
        return SoundType::Saw;
    else if (Utils::CaseInsensitiveEquals(str, "Sawed"))
        return SoundType::Sawed;
    else if (Utils::CaseInsensitiveEquals(str, "HeatBlasterCool"))
        return SoundType::HeatBlasterCool;
    else if (Utils::CaseInsensitiveEquals(str, "HeatBlasterHeat"))
        return SoundType::HeatBlasterHeat;
    else if (Utils::CaseInsensitiveEquals(str, "FireExtinguisher"))
        return SoundType::FireExtinguisher;
    else if (Utils::CaseInsensitiveEquals(str, "Swirl"))
        return SoundType::Swirl;
    else if (Utils::CaseInsensitiveEquals(str, "PinPoint"))
        return SoundType::PinPoint;
    else if (Utils::CaseInsensitiveEquals(str, "UnpinPoint"))
        return SoundType::UnpinPoint;
    else if (Utils::CaseInsensitiveEquals(str, "AirBubbles"))
        return SoundType::AirBubbles;
    else if (Utils::CaseInsensitiveEquals(str, "FloodHose"))
        return SoundType::FloodHose;
    else if (Utils::CaseInsensitiveEquals(str, "Stress"))
        return SoundType::Stress;
    else if (Utils::CaseInsensitiveEquals(str, "LightFlicker"))
        return SoundType::LightFlicker;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveSwitchOn"))
        return SoundType::InteractiveSwitchOn;
    else if (Utils::CaseInsensitiveEquals(str, "InteractiveSwitchOff"))
        return SoundType::InteractiveSwitchOff;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricalPanelOpen"))
        return SoundType::ElectricalPanelOpen;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricalPanelClose"))
        return SoundType::ElectricalPanelClose;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricalPanelDock"))
        return SoundType::ElectricalPanelDock;
    else if (Utils::CaseInsensitiveEquals(str, "ElectricalPanelUndock"))
        return SoundType::ElectricalPanelUndock;
    else if (Utils::CaseInsensitiveEquals(str, "GlassTick"))
        return SoundType::GlassTick;
    else if (Utils::CaseInsensitiveEquals(str, "EngineDiesel1"))
        return SoundType::EngineDiesel1;
    else if (Utils::CaseInsensitiveEquals(str, "EngineOutboard1"))
        return SoundType::EngineOutboard1;
    else if (Utils::CaseInsensitiveEquals(str, "EngineSteam1"))
        return SoundType::EngineSteam1;
    else if (Utils::CaseInsensitiveEquals(str, "EngineSteam2"))
        return SoundType::EngineSteam2;
    else if (Utils::CaseInsensitiveEquals(str, "EngineTelegraph"))
        return SoundType::EngineTelegraph;
    else if (Utils::CaseInsensitiveEquals(str, "WaterPump"))
        return SoundType::WaterPump;
    else if (Utils::CaseInsensitiveEquals(str, "WatertightDoorClosed"))
        return SoundType::WatertightDoorClosed;
    else if (Utils::CaseInsensitiveEquals(str, "WatertightDoorOpened"))
        return SoundType::WatertightDoorOpened;
    else if (Utils::CaseInsensitiveEquals(str, "ShipBell1"))
        return SoundType::ShipBell1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipBell2"))
        return SoundType::ShipBell2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipHorn1"))
        return SoundType::ShipHorn1;
    else if (Utils::CaseInsensitiveEquals(str, "ShipHorn2"))
        return SoundType::ShipHorn2;
    else if (Utils::CaseInsensitiveEquals(str, "ShipHorn3"))
        return SoundType::ShipHorn3;
    else if (Utils::CaseInsensitiveEquals(str, "ShipKlaxon1"))
        return SoundType::ShipKlaxon1;
    else if (Utils::CaseInsensitiveEquals(str, "WaterRush"))
        return SoundType::WaterRush;
    else if (Utils::CaseInsensitiveEquals(str, "WaterSplash"))
        return SoundType::WaterSplash;
    else if (Utils::CaseInsensitiveEquals(str, "AirBubblesSurface"))
        return SoundType::AirBubblesSurface;
    else if (Utils::CaseInsensitiveEquals(str, "Wave"))
        return SoundType::Wave;
    else if (Utils::CaseInsensitiveEquals(str, "Wind"))
        return SoundType::Wind;
    else if (Utils::CaseInsensitiveEquals(str, "WindGust"))
        return SoundType::WindGust;
    else if (Utils::CaseInsensitiveEquals(str, "Rain"))
        return SoundType::Rain;
    else if (Utils::CaseInsensitiveEquals(str, "Thunder"))
        return SoundType::Thunder;
    else if (Utils::CaseInsensitiveEquals(str, "Lightning"))
        return SoundType::Lightning;
    else if (Utils::CaseInsensitiveEquals(str, "FireBurning"))
        return SoundType::FireBurning;
    else if (Utils::CaseInsensitiveEquals(str, "FireSizzling"))
        return SoundType::FireSizzling;
    else if (Utils::CaseInsensitiveEquals(str, "CombustionExplosion"))
        return SoundType::CombustionExplosion;
    else if (Utils::CaseInsensitiveEquals(str, "TsunamiTriggered"))
        return SoundType::TsunamiTriggered;
    else if (Utils::CaseInsensitiveEquals(str, "BombAttached"))
        return SoundType::BombAttached;
    else if (Utils::CaseInsensitiveEquals(str, "BombDetached"))
        return SoundType::BombDetached;
    else if (Utils::CaseInsensitiveEquals(str, "BombExplosion"))
        return SoundType::BombExplosion;
    else if (Utils::CaseInsensitiveEquals(str, "RCBombPing"))
        return SoundType::RCBombPing;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombSlowFuse"))
        return SoundType::TimerBombSlowFuse;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombFastFuse"))
        return SoundType::TimerBombFastFuse;
    else if (Utils::CaseInsensitiveEquals(str, "TimerBombDefused"))
        return SoundType::TimerBombDefused;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombContained"))
        return SoundType::AntiMatterBombContained;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombPreImplosion"))
        return SoundType::AntiMatterBombPreImplosion;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombImplosion"))
        return SoundType::AntiMatterBombImplosion;
    else if (Utils::CaseInsensitiveEquals(str, "AntiMatterBombExplosion"))
        return SoundType::AntiMatterBombExplosion;
    else if (Utils::CaseInsensitiveEquals(str, "Pliers"))
        return SoundType::Pliers;
    else if (Utils::CaseInsensitiveEquals(str, "Snapshot"))
        return SoundType::Snapshot;
    else if (Utils::CaseInsensitiveEquals(str, "TerrainAdjust"))
        return SoundType::TerrainAdjust;
    else if (Utils::CaseInsensitiveEquals(str, "Scrub"))
        return SoundType::Scrub;
    else if (Utils::CaseInsensitiveEquals(str, "RepairStructure"))
        return SoundType::RepairStructure;
    else if (Utils::CaseInsensitiveEquals(str, "ThanosSnap"))
        return SoundType::ThanosSnap;
    else if (Utils::CaseInsensitiveEquals(str, "WaveMaker"))
        return SoundType::WaveMaker;
    else if (Utils::CaseInsensitiveEquals(str, "FishScream"))
        return SoundType::FishScream;
    else if (Utils::CaseInsensitiveEquals(str, "FishShaker"))
        return SoundType::FishShaker;
    else if (Utils::CaseInsensitiveEquals(str, "Error"))
        return SoundType::Error;
    else
        throw GameException("Unrecognized SoundType \"" + str + "\"");
}

SizeType StrToSizeType(std::string const & str)
{
    std::string lstr = Utils::ToLower(str);

    if (lstr == "small")
        return SizeType::Small;
    else if (lstr == "medium")
        return SizeType::Medium;
    else if (lstr == "large")
        return SizeType::Large;
    else
        throw GameException("Unrecognized SizeType \"" + str + "\"");
}