/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2021-10-20
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>

#include <picojson.h>

#include <cstdint>

struct ShipAutoTexturizationSettings
{
    ShipAutoTexturizationModeType Mode;
    float MaterialTextureMagnification;
    float MaterialTextureTransparency;

    ShipAutoTexturizationSettings(
        ShipAutoTexturizationModeType mode,
        float materialTextureMagnification,
        float materialTextureTransparency)
        : Mode(mode)
        , MaterialTextureMagnification(materialTextureMagnification)
        , MaterialTextureTransparency(materialTextureTransparency)
    {}

    // Defaults
    ShipAutoTexturizationSettings()
        : Mode(ShipAutoTexturizationModeType::MaterialTextures)
        , MaterialTextureMagnification(1.0f)
        , MaterialTextureTransparency(0.0f)
    {}

    static ShipAutoTexturizationSettings FromJSON(picojson::object const & jsonObject);
    picojson::object ToJSON() const;
};
