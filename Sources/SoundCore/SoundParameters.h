/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <SoundCore/SoundTypes.h>

#include <cstdint>

struct SoundParameters
{
    static float constexpr SampleRate = 11025.0f; // Frames/sec

    static SoundChannelModeType constexpr SoundChannelMode = SoundChannelModeType::Mono;
    static std::size_t constexpr SoundChannelCount = (SoundChannelMode == SoundChannelModeType::Mono) ? 1 : 2;

    static size_t constexpr MaxPlayingSounds = 256;
};
