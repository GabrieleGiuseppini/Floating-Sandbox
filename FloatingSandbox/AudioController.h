/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-11-17
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <SFML/Audio.hpp>

class AudioController
{
public:

    static void SetGlobalMute(bool isMuted)
    {
        sf::Listener::setGlobalVolume(isMuted ? 0.0f : 100.0f);
    }

    static bool GetGlobalMute()
    {
        return !(sf::Listener::getGlobalVolume() > 0.0f);
    }
};
