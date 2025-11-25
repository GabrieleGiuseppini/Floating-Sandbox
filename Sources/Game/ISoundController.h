/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-11-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/Materials.h>

#include <optional>

 /*
  * Interface of SoundController to other projects.
  */
struct ISoundController
{
    virtual ~ISoundController() = default;

    virtual void PlayOneShotShipSound(
        std::optional<ElectricalMaterial::ShipSoundElementType> shipSoundElementType,
        float volume) = 0;

    virtual void PlayErrorSound() = 0;
};
