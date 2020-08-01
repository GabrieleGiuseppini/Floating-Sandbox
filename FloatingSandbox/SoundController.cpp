/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundController.h"

#include <Game/Materials.h>

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <limits>
#include <regex>

using namespace std::chrono_literals;

float constexpr RepairVolume = 40.0f;
float constexpr SawVolume = 50.0f;
float constexpr SawedVolume = 80.0f;
float constexpr StressSoundVolume = 20.0f;
std::chrono::milliseconds constexpr SawedInertiaDuration = std::chrono::milliseconds(200);
float constexpr WaveSplashTriggerSize = 0.5f;

SoundController::SoundController(
    ResourceLocator & resourceLocator,
    ProgressCallback const & progressCallback)
    : // State
      mMasterEffectsVolume(100.0f)
    , mMasterEffectsMuted(false)
    , mMasterToolsVolume(100.0f)
    , mMasterToolsMuted(false)
    , mPlayBreakSounds(true)
    , mPlayStressSounds(true)
    , mPlayWindSound(true)
    , mLastWaterSplashed(0.0f)
    , mCurrentWaterSplashedTrigger(WaveSplashTriggerSize)
    , mLastWindSpeedAbsoluteMagnitude(0.0f)
    , mWindVolumeRunningAverage()
    // One-shot sounds
    , mMSUOneShotMultipleChoiceSounds()
    , mMOneShotMultipleChoiceSounds()
    , mDslUOneShotMultipleChoiceSounds()
    , mUOneShotMultipleChoiceSounds()
    , mOneShotMultipleChoiceSounds()
    , mCurrentlyPlayingOneShotSounds()
    // Continuous sounds
    , mSawedMetalSound(SawedInertiaDuration)
    , mSawedWoodSound(SawedInertiaDuration)
    , mSawAbovewaterSound()
    , mSawUnderwaterSound()
    , mHeatBlasterCoolSound()
    , mHeatBlasterHeatSound()
    , mFireExtinguisherSound()
    , mDrawSound()
    , mSwirlSound()
    , mAirBubblesSound()
    , mFloodHoseSound()
    , mRepairStructureSound()
    , mWaveMakerSound()
    , mWaterRushSound()
    , mWaterSplashSound()
    , mAirBubblesSurfacingSound(0.23f, 0.12f)
    , mWindSound()
    , mRainSound()
    , mFireBurningSound()
    , mTimerBombSlowFuseSound()
    , mTimerBombFastFuseSound()
    , mAntiMatterBombContainedSounds()
    , mLoopedSounds(
        mMasterEffectsVolume,
        mMasterEffectsMuted)
{
    //
    // Initialize Sounds
    //

    auto soundNames = resourceLocator.GetSoundNames();

    for (size_t i = 0; i < soundNames.size(); ++i)
    {
        std::string const & soundName = soundNames[i];

        // Notify progress
        progressCallback(static_cast<float>(i + 1) / static_cast<float>(soundNames.size()), "Loading sounds...");

        //
        // Load sound buffer
        //

        std::unique_ptr<sf::SoundBuffer> soundBuffer = std::make_unique<sf::SoundBuffer>();
        if (!soundBuffer->loadFromFile(resourceLocator.GetSoundFilePath(soundName).string()))
        {
            throw GameException("Cannot load sound \"" + soundName + "\"");
        }


        //
        // Parse filename
        //

        std::regex soundTypeRegex(R"(([^_]+)(?:_.+)?)");
        std::smatch soundTypeMatch;
        if (!std::regex_match(soundName, soundTypeMatch, soundTypeRegex))
        {
            throw GameException("Sound filename \"" + soundName + "\" is not recognized");
        }

        assert(soundTypeMatch.size() == 1 + 1);
        SoundType soundType = StrToSoundType(soundTypeMatch[1].str());
        if (soundType == SoundType::Saw)
        {
            std::regex sawRegex(R"(([^_]+)(?:_(underwater))?)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, sawRegex))
            {
                throw GameException("Saw sound filename \"" + soundName + "\" is not recognized");
            }

            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                mSawUnderwaterSound.Initialize(
                    std::move(soundBuffer),
                    SawVolume,
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
            else
            {
                mSawAbovewaterSound.Initialize(
                    std::move(soundBuffer),
                    SawVolume,
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
        }
        else if (soundType == SoundType::Draw)
        {
            mDrawSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::Sawed)
        {
            std::regex mRegex(R"(([^_]+)_([^_]+))");
            std::smatch mMatch;
            if (!std::regex_match(soundName, mMatch, mRegex))
            {
                throw GameException("M sound filename \"" + soundName + "\" is not recognized");
            }

            assert(mMatch.size() == 1 + 2);

            // Parse SoundElementType
            StructuralMaterial::MaterialSoundType materialSound = StructuralMaterial::StrToMaterialSoundType(mMatch[2].str());

            if (StructuralMaterial::MaterialSoundType::Metal == materialSound)
            {
                mSawedMetalSound.Initialize(
                    std::move(soundBuffer),
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
            else
            {
                mSawedWoodSound.Initialize(
                    std::move(soundBuffer),
                    mMasterToolsVolume,
                    mMasterToolsMuted);
            }
        }
        else if (soundType == SoundType::HeatBlasterCool)
        {
            mHeatBlasterCoolSound.Initialize(
                std::move(soundBuffer),
                60.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::HeatBlasterHeat)
        {
            mHeatBlasterHeatSound.Initialize(
                std::move(soundBuffer),
                60.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::FireExtinguisher)
        {
            mFireExtinguisherSound.Initialize(
                std::move(soundBuffer),
                80.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::Swirl)
        {
            mSwirlSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::AirBubbles)
        {
            mAirBubblesSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::FloodHose)
        {
            mFloodHoseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::RepairStructure)
        {
            mRepairStructureSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterToolsVolume,
                mMasterToolsMuted);
        }
        else if (soundType == SoundType::WaveMaker)
        {
            mWaveMakerSound.Initialize(
                std::move(soundBuffer),
                40.0f,
                mMasterToolsVolume,
                mMasterToolsMuted,
                std::chrono::milliseconds(2500),
                std::chrono::milliseconds(5000));
        }
        else if (soundType == SoundType::WaterRush)
        {
            mWaterRushSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::WaterSplash)
        {
            mWaterSplashSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::AirBubblesSurface)
        {
            mAirBubblesSurfacingSound.Initialize(
                std::move(soundBuffer),
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::Wind)
        {
            mWindSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::Rain)
        {
            mRainSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::FireBurning)
        {
            mFireBurningSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted,
                1500ms,
                1500ms,
                0.2f);
        }
        else if (soundType == SoundType::TimerBombSlowFuse)
        {
            mTimerBombSlowFuseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::TimerBombFastFuse)
        {
            mTimerBombFastFuseSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::EngineDiesel1
                || soundType == SoundType::EngineOutboard1
                || soundType == SoundType::EngineSteam1
                || soundType == SoundType::EngineSteam2
                || soundType == SoundType::WaterPump)
        {
            mLoopedSounds.AddAlternativeForSoundType(
                soundType,
                false, // IsUnderwater
                resourceLocator.GetSoundFilePath(soundName));
        }
        else if (soundType == SoundType::Break
                || soundType == SoundType::Destroy
                || soundType == SoundType::Stress
                || soundType == SoundType::RepairSpring
                || soundType == SoundType::RepairTriangle)
        {
            //
            // MSU sound
            //

            std::regex msuRegex(R"(([^_]+)_([^_]+)_([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch msuMatch;
            if (!std::regex_match(soundName, msuMatch, msuRegex))
            {
                throw GameException("MSU sound filename \"" + soundName + "\" is not recognized");
            }

            assert(msuMatch.size() == 1 + 4);

            // 1. Parse MaterialSoundType
            StructuralMaterial::MaterialSoundType materialSound = StructuralMaterial::StrToMaterialSoundType(msuMatch[2].str());

            // 2. Parse Size
            SizeType sizeType = StrToSizeType(msuMatch[3].str());

            // 3. Parse Underwater
            bool isUnderwater;
            if (msuMatch[4].matched)
            {
                assert(msuMatch[4].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mMSUOneShotMultipleChoiceSounds[std::make_tuple(soundType, materialSound, sizeType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::LightningHit)
        {
            //
            // M sound
            //

            std::regex mRegex(R"(([^_]+)_([^_]+)_\d+)");
            std::smatch mMatch;
            if (!std::regex_match(soundName, mMatch, mRegex))
            {
                throw GameException("M sound filename \"" + soundName + "\" is not recognized");
            }

            assert(mMatch.size() == 1 + 2);

            // 1. Parse MaterialSoundType
            StructuralMaterial::MaterialSoundType materialSound = StructuralMaterial::StrToMaterialSoundType(mMatch[2].str());


            //
            // Store sound buffer
            //

            mMOneShotMultipleChoiceSounds[std::make_tuple(soundType, materialSound)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::LightFlicker)
        {
            //
            // DslU sound
            //

            std::regex dsluRegex(R"(([^_]+)_([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch dsluMatch;
            if (!std::regex_match(soundName, dsluMatch, dsluRegex))
            {
                throw GameException("DslU sound filename \"" + soundName + "\" is not recognized");
            }

            assert(dsluMatch.size() >= 1 + 2 && dsluMatch.size() <= 1 + 3);

            // 1. Parse Duration
            DurationShortLongType durationType = StrToDurationShortLongType(dsluMatch[2].str());

            // 2. Parse Underwater
            bool isUnderwater;
            if (dsluMatch[3].matched)
            {
                assert(dsluMatch[3].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mDslUOneShotMultipleChoiceSounds[std::make_tuple(soundType, durationType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::Wave
                || soundType == SoundType::WindGust
                || soundType == SoundType::Thunder
                || soundType == SoundType::Lightning
                || soundType == SoundType::FireSizzling
                || soundType == SoundType::TsunamiTriggered
                || soundType == SoundType::AntiMatterBombPreImplosion
                || soundType == SoundType::AntiMatterBombImplosion
                || soundType == SoundType::Snapshot
                || soundType == SoundType::TerrainAdjust
                || soundType == SoundType::ThanosSnap
                || soundType == SoundType::Scrub
                || soundType == SoundType::InteractiveSwitchOn
                || soundType == SoundType::InteractiveSwitchOff
                || soundType == SoundType::ElectricalPanelClose
                || soundType == SoundType::ElectricalPanelOpen
                || soundType == SoundType::ElectricalPanelDock
                || soundType == SoundType::ElectricalPanelUndock
                || soundType == SoundType::GlassTick
                || soundType == SoundType::EngineTelegraph
                || soundType == SoundType::WatertightDoorClosed
                || soundType == SoundType::WatertightDoorOpened
                || soundType == SoundType::Error)
        {
            //
            // - one-shot sound
            //

            std::regex sRegex(R"(([^_]+)_\d+)");
            std::smatch sMatch;
            if (!std::regex_match(soundName, sMatch, sRegex))
            {
                throw GameException("- sound filename \"" + soundName + "\" is not recognized");
            }

            assert(sMatch.size() == 1 + 1);

            //
            // Store sound buffer
            //

            mOneShotMultipleChoiceSounds[std::make_tuple(soundType)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
        else if (soundType == SoundType::AntiMatterBombContained)
        {
            //
            // - continuous sound
            //

            std::regex sRegex(R"(([^_]+)_\d+)");
            std::smatch sMatch;
            if (!std::regex_match(soundName, sMatch, sRegex))
            {
                throw GameException("- sound filename \"" + soundName + "\" is not recognized");
            }

            assert(sMatch.size() == 1 + 1);

            //
            // Initialize continuous sound
            //

            mAntiMatterBombContainedSounds.AddAlternative(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
        }
        else if (soundType == SoundType::ShipBell1
                || soundType == SoundType::ShipBell2
                || soundType == SoundType::ShipHorn1
                || soundType == SoundType::ShipHorn2
                || soundType == SoundType::ShipHorn3
                || soundType == SoundType::ShipKlaxon1)
        {
            //
            // Looped U sound
            //

            std::regex uRegex(R"(([^_]+)(?:_(underwater))?)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, uRegex))
            {
                throw GameException("U sound filename \"" + soundName + "\" is not recognized");
            }

            assert(uMatch.size() == 1 + 2);

            // 1. Parse Underwater
            bool isUnderwater;
            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound
            //

            float loopStartSample = 0.0f;
            float loopEndSample = 0.0f;
            switch (soundType)
            {
                case SoundType::ShipBell1:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 0.881723f;
                        loopEndSample = 1.84444f;
                    }
                    else
                    {
                        loopStartSample = 0.88127f;
                        loopEndSample = 1.77351f;
                    }

                    break;
                }

                case SoundType::ShipBell2:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 0.485896f;
                        loopEndSample = 0.936599f;
                    }
                    else
                    {
                        loopStartSample = 0.485986f;
                        loopEndSample = 0.936961f;
                    }

                    break;
                }

                case SoundType::ShipHorn1:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 0.678503f;
                        loopEndSample = 2.01508f;
                    }
                    else
                    {
                        loopStartSample = 0.507846f;
                        loopEndSample = 1.76757f;
                    }

                    break;
                }

                case SoundType::ShipHorn2:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 1.79079f;
                        loopEndSample = loopStartSample + 1.41587f;
                    }
                    else
                    {
                        loopStartSample = 1.79161f;
                        loopEndSample = loopStartSample + 1.41698f;
                    }

                    break;
                }

                case SoundType::ShipHorn3:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 1.73426f;
                        loopEndSample = loopStartSample + 1.09522f;
                    }
                    else
                    {
                        loopStartSample = 1.7388f;
                        loopEndSample = loopStartSample + 1.09977f;
                    }

                    break;
                }

                case SoundType::ShipKlaxon1:
                {
                    if (!isUnderwater)
                    {
                        loopStartSample = 0.81898f;
                        loopEndSample = loopStartSample + 0.429751f;
                    }
                    else
                    {
                        loopStartSample = 0.904989f;
                        loopEndSample = loopStartSample + 0.704739f;
                    }

                    break;
                }

                default:
                {
                    assert(false);
                }
            }

            mLoopedSounds.AddAlternativeForSoundType(
                soundType,
                isUnderwater,
                resourceLocator.GetSoundFilePath(soundName),
                loopStartSample,
                loopEndSample);
        }
        else
        {
            //
            // U sound
            //

            std::regex uRegex(R"(([^_]+)_(?:(underwater)_)?\d+)");
            std::smatch uMatch;
            if (!std::regex_match(soundName, uMatch, uRegex))
            {
                throw GameException("U sound filename \"" + soundName + "\" is not recognized");
            }

            assert(uMatch.size() == 1 + 2);

            // 1. Parse Underwater
            bool isUnderwater;
            if (uMatch[2].matched)
            {
                assert(uMatch[2].str() == "underwater");
                isUnderwater = true;
            }
            else
            {
                isUnderwater = false;
            }


            //
            // Store sound buffer
            //

            mUOneShotMultipleChoiceSounds[std::make_tuple(soundType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
        }
    }
}

SoundController::~SoundController()
{
    Reset();
}

void SoundController::SetPaused(bool isPaused)
{
    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        for (auto & playingSound : playingSoundIt.second)
        {
            if (isPaused)
            {
                playingSound.Sound->pause();
            }
            else
            {
                playingSound.Sound->resume();
            }
        }
    }

    // We don't pause the sounds of those continuous tools that keep "working" while paused;
    // we only pause the sounds of those that stop functioning
    mWaveMakerSound.SetPaused(isPaused);

    mWaterRushSound.SetPaused(isPaused);
    mWaterSplashSound.SetPaused(isPaused);
    mAirBubblesSurfacingSound.SetPaused(isPaused);
    mWindSound.SetPaused(isPaused);
    mRainSound.SetPaused(isPaused);
    mFireBurningSound.SetPaused(isPaused);
    mTimerBombSlowFuseSound.SetPaused(isPaused);
    mTimerBombFastFuseSound.SetPaused(isPaused);
    mAntiMatterBombContainedSounds.SetPaused(isPaused);
    mLoopedSounds.SetPaused(isPaused);
}

// Master effects

void SoundController::SetMasterEffectsVolume(float volume)
{
    mMasterEffectsVolume = volume;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first != SoundType::Draw
            && playingSoundIt.first != SoundType::Saw
            && playingSoundIt.first != SoundType::HeatBlasterCool
            && playingSoundIt.first != SoundType::HeatBlasterHeat
            && playingSoundIt.first != SoundType::FireExtinguisher
            && playingSoundIt.first != SoundType::Swirl
            && playingSoundIt.first != SoundType::AirBubbles
            && playingSoundIt.first != SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMasterVolume(mMasterEffectsVolume);
            }
        }
    }

    mWaterRushSound.SetMasterVolume(mMasterEffectsVolume);
    mWaterSplashSound.SetMasterVolume(mMasterEffectsVolume);
    mAirBubblesSurfacingSound.SetMasterVolume(mMasterEffectsVolume);
    mWindSound.SetMasterVolume(mMasterEffectsVolume);
    mRainSound.SetMasterVolume(mMasterEffectsVolume);
    mFireBurningSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombSlowFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombFastFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mAntiMatterBombContainedSounds.SetMasterVolume(mMasterEffectsVolume);
    mLoopedSounds.SetMasterVolume(mMasterEffectsVolume);
}

void SoundController::SetMasterEffectsMuted(bool isMuted)
{
    mMasterEffectsMuted = isMuted;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first != SoundType::Draw
            && playingSoundIt.first != SoundType::Saw
            && playingSoundIt.first != SoundType::HeatBlasterCool
            && playingSoundIt.first != SoundType::HeatBlasterHeat
            && playingSoundIt.first != SoundType::FireExtinguisher
            && playingSoundIt.first != SoundType::Swirl
            && playingSoundIt.first != SoundType::AirBubbles
            && playingSoundIt.first != SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMuted(mMasterEffectsMuted);
            }
        }
    }

    mWaterRushSound.SetMuted(mMasterEffectsMuted);
    mWaterSplashSound.SetMuted(mMasterEffectsMuted);
    mAirBubblesSurfacingSound.SetMuted(mMasterEffectsMuted);
    mWindSound.SetMuted(mMasterEffectsMuted);
    mRainSound.SetMuted(mMasterEffectsMuted);
    mFireBurningSound.SetMuted(mMasterEffectsMuted);
    mTimerBombSlowFuseSound.SetMuted(mMasterEffectsMuted);
    mTimerBombFastFuseSound.SetMuted(mMasterEffectsMuted);
    mAntiMatterBombContainedSounds.SetMuted(mMasterEffectsMuted);
    mLoopedSounds.SetMuted(mMasterEffectsMuted);
}

// Master tools

void SoundController::SetMasterToolsVolume(float volume)
{
    mMasterToolsVolume = volume;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first == SoundType::Draw
            || playingSoundIt.first == SoundType::Saw
            || playingSoundIt.first == SoundType::HeatBlasterCool
            || playingSoundIt.first == SoundType::HeatBlasterHeat
            || playingSoundIt.first == SoundType::FireExtinguisher
            || playingSoundIt.first == SoundType::Swirl
            || playingSoundIt.first == SoundType::AirBubbles
            || playingSoundIt.first == SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMasterVolume(mMasterToolsVolume);
            }
        }
    }

    mSawedMetalSound.SetMasterVolume(mMasterToolsVolume);
    mSawedWoodSound.SetMasterVolume(mMasterToolsVolume);

    mSawAbovewaterSound.SetMasterVolume(mMasterToolsVolume);
    mSawUnderwaterSound.SetMasterVolume(mMasterToolsVolume);
    mHeatBlasterCoolSound.SetMasterVolume(mMasterToolsVolume);
    mHeatBlasterHeatSound.SetMasterVolume(mMasterToolsVolume);
    mFireExtinguisherSound.SetMasterVolume(mMasterToolsVolume);
    mDrawSound.SetMasterVolume(mMasterToolsVolume);
    mSwirlSound.SetMasterVolume(mMasterToolsVolume);
    mAirBubblesSound.SetMasterVolume(mMasterToolsVolume);
    mFloodHoseSound.SetMasterVolume(mMasterToolsVolume);
    mRepairStructureSound.SetMasterVolume(mMasterToolsVolume);
    mWaveMakerSound.SetMasterVolume(mMasterToolsVolume);
}

void SoundController::SetMasterToolsMuted(bool isMuted)
{
    mMasterToolsMuted = isMuted;

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        if (playingSoundIt.first == SoundType::Draw
            || playingSoundIt.first == SoundType::Saw
            || playingSoundIt.first == SoundType::HeatBlasterCool
            || playingSoundIt.first == SoundType::HeatBlasterHeat
            || playingSoundIt.first == SoundType::FireExtinguisher
            || playingSoundIt.first == SoundType::Swirl
            || playingSoundIt.first == SoundType::AirBubbles
            || playingSoundIt.first == SoundType::FloodHose)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                playingSound.Sound->setMuted(mMasterToolsMuted);
            }
        }
    }

    mSawedMetalSound.SetMuted(mMasterToolsMuted);
    mSawedWoodSound.SetMuted(mMasterToolsMuted);

    mSawAbovewaterSound.SetMuted(mMasterToolsMuted);
    mSawUnderwaterSound.SetMuted(mMasterToolsMuted);
    mHeatBlasterCoolSound.SetMuted(mMasterToolsMuted);
    mHeatBlasterHeatSound.SetMuted(mMasterToolsMuted);
    mFireExtinguisherSound.SetMuted(mMasterToolsMuted);
    mDrawSound.SetMuted(mMasterToolsMuted);
    mSwirlSound.SetMuted(mMasterToolsMuted);
    mAirBubblesSound.SetMuted(mMasterToolsMuted);
    mFloodHoseSound.SetMuted(mMasterToolsMuted);
    mRepairStructureSound.SetMuted(mMasterToolsMuted);
    mWaveMakerSound.SetMuted(mMasterToolsMuted);
}

void SoundController::SetPlayBreakSounds(bool playBreakSounds)
{
    mPlayBreakSounds = playBreakSounds;

    if (!mPlayBreakSounds)
    {
        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::Break == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
}

void SoundController::SetPlayStressSounds(bool playStressSounds)
{
    mPlayStressSounds = playStressSounds;

    if (!mPlayStressSounds)
    {
        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::Stress == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
}

void SoundController::SetPlayWindSound(bool playWindSound)
{
    mPlayWindSound = playWindSound;

    if (!mPlayWindSound)
    {
        mWindSound.SetMuted(true);

        for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
        {
            for (auto & playingSound : playingSoundIt.second)
            {
                if (SoundType::WindGust == playingSound.Type)
                {
                    playingSound.Sound->stop();
                }
            }
        }
    }
    else
    {
        mWindSound.SetMuted(false);
    }
}

// Misc

void SoundController::PlayDrawSound(bool /*isUnderwater*/)
{
    // At the moment we ignore the water-ness
    mDrawSound.Start();
}

void SoundController::StopDrawSound()
{
    mDrawSound.Stop();
}

void SoundController::PlaySawSound(bool isUnderwater)
{
    if (isUnderwater)
    {
        mSawUnderwaterSound.Start();
        mSawAbovewaterSound.Stop();
    }
    else
    {
        mSawAbovewaterSound.Start();
        mSawUnderwaterSound.Stop();
    }

    mSawedMetalSound.Start();
    mSawedWoodSound.Start();
}

void SoundController::StopSawSound()
{
    mSawedMetalSound.Stop();
    mSawedWoodSound.Stop();

    mSawAbovewaterSound.Stop();
    mSawUnderwaterSound.Stop();
}

void SoundController::PlayHeatBlasterSound(HeatBlasterActionType action)
{
    switch (action)
    {
        case HeatBlasterActionType::Cool:
        {
            mHeatBlasterHeatSound.Stop();
            mHeatBlasterCoolSound.Start();
            break;
        }

        case HeatBlasterActionType::Heat:
        {
            mHeatBlasterCoolSound.Stop();
            mHeatBlasterHeatSound.Start();
            break;
        }
    }
}

void SoundController::StopHeatBlasterSound()
{
    mHeatBlasterCoolSound.Stop();
    mHeatBlasterHeatSound.Stop();
}

void SoundController::PlayFireExtinguisherSound()
{
    mFireExtinguisherSound.Start();
}

void SoundController::StopFireExtinguisherSound()
{
    mFireExtinguisherSound.Stop();
}

void SoundController::PlaySwirlSound(bool /*isUnderwater*/)
{
    // At the moment we ignore the water-ness
    mSwirlSound.Start();
}

void SoundController::StopSwirlSound()
{
    mSwirlSound.Stop();
}

void SoundController::PlayAirBubblesSound()
{
    mAirBubblesSound.Start();
}

void SoundController::StopAirBubblesSound()
{
    mAirBubblesSound.Stop();
}

void SoundController::PlayFloodHoseSound()
{
    mFloodHoseSound.Start();
}

void SoundController::StopFloodHoseSound()
{
    mFloodHoseSound.Stop();
}

void SoundController::PlayTerrainAdjustSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::TerrainAdjust,
        100.0f,
        true);
}

void SoundController::PlayRepairStructureSound()
{
    mRepairStructureSound.Start();
}

void SoundController::StopRepairStructureSound()
{
    mRepairStructureSound.Stop();
}

void SoundController::PlayThanosSnapSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::ThanosSnap,
        100.0f,
        true);
}

void SoundController::PlayWaveMakerSound()
{
    mWaveMakerSound.FadeIn();
}

void SoundController::StopWaveMakerSound()
{
    mWaveMakerSound.FadeOut();
}

void SoundController::PlayScrubSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Scrub,
        100.0f,
        true);
}

void SoundController::PlayPliersSound(bool isUnderwater)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::Pliers,
        isUnderwater,
        100.0f,
        true);
}

void SoundController::PlaySnapshotSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Snapshot,
        100.0f,
        true);
}

void SoundController::PlayElectricalPanelOpenSound(bool isClose)
{
    PlayOneShotMultipleChoiceSound(
        isClose ? SoundType::ElectricalPanelClose: SoundType::ElectricalPanelOpen,
        100.0f,
        true);
}

void SoundController::PlayElectricalPanelDockSound(bool isUndock)
{
    PlayOneShotMultipleChoiceSound(
        isUndock ? SoundType::ElectricalPanelUndock : SoundType::ElectricalPanelDock,
        100.0f,
        true);
}

void SoundController::PlayTickSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::GlassTick,
        100.0f,
        true);
}

void SoundController::PlayErrorSound()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Error,
        50.0f,
        true);
}

void SoundController::UpdateSimulation()
{
    mWaveMakerSound.UpdateSimulation();
    mAirBubblesSurfacingSound.UpdateSimulation();
    mFireBurningSound.UpdateSimulation();

    // Silence the inertial sounds - this will basically be a nop in case
    // they've just been started or will be started really soon
    mSawedMetalSound.SetVolume(0.0f);
    mSawedWoodSound.SetVolume(0.0f);
}

void SoundController::LowFrequencyUpdateSimulation()
{
}

void SoundController::Reset()
{
    //
    // Stop and clear all sounds
    //

    for (auto const & playingSoundIt : mCurrentlyPlayingOneShotSounds)
    {
        for (auto & playingSound : playingSoundIt.second)
        {
            assert(!!playingSound.Sound);
            if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
            {
                playingSound.Sound->stop();
            }
        }
    }

    mCurrentlyPlayingOneShotSounds.clear();

    mSawedMetalSound.Reset();
    mSawedWoodSound.Reset();
    mSawAbovewaterSound.Reset();
    mSawUnderwaterSound.Reset();
    mHeatBlasterCoolSound.Reset();
    mHeatBlasterHeatSound.Reset();
    mFireExtinguisherSound.Reset();
    mDrawSound.Reset();
    mSwirlSound.Reset();
    mAirBubblesSound.Reset();
    mFloodHoseSound.Reset();
    mRepairStructureSound.Reset();
    mWaveMakerSound.Reset();

    mWaterRushSound.Reset();
    mWaterSplashSound.Reset();
    mAirBubblesSurfacingSound.Reset();
    mWindSound.Reset();
    mRainSound.Reset();
    mFireBurningSound.Reset();
    mTimerBombSlowFuseSound.Reset();
    mTimerBombFastFuseSound.Reset();
    mAntiMatterBombContainedSounds.Reset();
    mLoopedSounds.Reset();

    //
    // Reset state
    //

    mLastWaterSplashed = 0.0f;
    mCurrentWaterSplashedTrigger = WaveSplashTriggerSize;
    mLastWindSpeedAbsoluteMagnitude = 0.0f;
    mWindVolumeRunningAverage.Reset();
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::OnDestroy(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Destroy,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            70.0f,
            true);
    }
}

void SoundController::OnLightningHit(StructuralMaterial const & structuralMaterial)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMOneShotMultipleChoiceSound(
            SoundType::LightningHit,
            *(structuralMaterial.MaterialSound),
            70.0f,
            true);
    }
}

void SoundController::OnSpringRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::RepairSpring,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            RepairVolume,
            true);
    }
}

void SoundController::OnTriangleRepaired(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (!!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::RepairTriangle,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            RepairVolume,
            true);
    }
}

void SoundController::OnSawed(
    bool isMetal,
    unsigned int size)
{
    if (isMetal)
        mSawedMetalSound.SetVolume(size > 0 ? SawedVolume : 0.0f);
    else
        mSawedWoodSound.SetVolume(size > 0 ? SawedVolume : 0.0f);
}

void SoundController::OnPinToggled(
    bool isPinned,
    bool isUnderwater)
{
    PlayUOneShotMultipleChoiceSound(
        isPinned ? SoundType::PinPoint : SoundType::UnpinPoint,
        isUnderwater,
        100.0f,
        true);
}

void SoundController::OnTsunamiNotification(float /*x*/)
{
    PlayOneShotMultipleChoiceSound(
        SoundType::TsunamiTriggered,
        100.0f,
        true);
}

void SoundController::OnPointCombustionBegin()
{
    mFireBurningSound.AddAggregateVolume();
}

void SoundController::OnPointCombustionEnd()
{
    mFireBurningSound.SubAggregateVolume();
}

void SoundController::OnCombustionSmothered()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::FireSizzling,
        100.0f,
        true);
}

void SoundController::OnCombustionExplosion(
    bool isUnderwater,
    unsigned int /*size*/)
{
    float const volume = 100.0f;

    PlayUOneShotMultipleChoiceSound(
        SoundType::CombustionExplosion,
        isUnderwater,
        volume,
        false);
}

void SoundController::OnStress(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (mPlayStressSounds
        && !!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Stress,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            StressSoundVolume,
            true);
    }
}

void SoundController::OnBreak(
    StructuralMaterial const & structuralMaterial,
    bool isUnderwater,
    unsigned int size)
{
    if (mPlayBreakSounds
        && !!(structuralMaterial.MaterialSound))
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Break,
            *(structuralMaterial.MaterialSound),
            size,
            isUnderwater,
            10.0f,
            true);
    }
}

void SoundController::OnWaterTaken(float waterTaken)
{
    // 50 * (-1 / 2.4^(0.3 * x) + 1)
    float rushVolume = 40.f * (-1.f / std::pow(2.4f, std::min(90.0f, 0.3f * std::abs(waterTaken))) + 1.f);

    // Starts automatically if volume greater than zero
    mWaterRushSound.SetVolume(rushVolume);
}

void SoundController::OnWaterSplashed(float waterSplashed)
{
    //
    // Trigger waves
    //

    if (waterSplashed > mLastWaterSplashed)
    {
        if (waterSplashed > mCurrentWaterSplashedTrigger)
        {
            // 10 * (-1 / 1.8^(0.08 * x) + 1)
            float const waveVolume = 10.f * (-1.f / std::pow(1.8f, 0.08f * std::min(1800.0f, std::abs(waterSplashed))) + 1.f);

            PlayOneShotMultipleChoiceSound(
                SoundType::Wave,
                waveVolume,
                true);

            // Advance trigger
            mCurrentWaterSplashedTrigger = waterSplashed + WaveSplashTriggerSize;
        }
    }
    else
    {
        // Lower trigger
        mCurrentWaterSplashedTrigger = waterSplashed + WaveSplashTriggerSize;
    }

    mLastWaterSplashed = waterSplashed;


    //
    // Adjust continuous splash sound
    //

    // 12 * (-1 / 1.3^(0.01*x) + 1)
    float const splashVolume = 12.f * (-1.f / std::pow(1.3f, 0.01f * std::abs(waterSplashed)) + 1.f);

    // Starts automatically if volume greater than zero
    mWaterSplashSound.SetVolume(splashVolume);
}

void SoundController::OnAirBubbleSurfaced(unsigned int size)
{
    auto const volume = std::min(100.0f, static_cast<float>(size) * 20.0f);

    mAirBubblesSurfacingSound.Pulse(volume);
}

void SoundController::OnWindSpeedUpdated(
    float const /*zeroSpeedMagnitude*/,
    float const baseSpeedMagnitude,
    float const /*baseAndStormSpeedMagnitude*/,
    float const /*preMaxSpeedMagnitude*/,
    float const maxSpeedMagnitude,
    vec2f const & windSpeed)
{
    float const windSpeedAbsoluteMagnitude = windSpeed.length();

    //
    // 1. Calculate volume of continuous sound
    //

    float windVolume;
    if (windSpeedAbsoluteMagnitude >= std::abs(baseSpeedMagnitude))
    {
        // 20 -> 43:
        // 100 * (-1 / 1.1^(0.3 * x) + 1)
        windVolume = 100.f * (-1.f / std::pow(1.1f, 0.3f * (windSpeedAbsoluteMagnitude - std::abs(baseSpeedMagnitude))) + 1.f);
    }
    else
    {
        // Raise volume only if goes up
        float const deltaUp = std::max(0.0f, windSpeedAbsoluteMagnitude - mLastWindSpeedAbsoluteMagnitude);

        // 100 * (-1 / 1.1^(0.3 * x) + 1)
        windVolume = 100.f * (-1.f / std::pow(1.1f, 0.3f * deltaUp) + 1.f);
    }

    // Smooth the volume
    float const smoothedWindVolume = mWindVolumeRunningAverage.Update(windVolume);

    // Set the volume - starts automatically if volume greater than zero
    mWindSound.SetVolume(smoothedWindVolume);


    //
    // 2. Decide if time to fire a gust
    //

    if (mPlayWindSound)
    {
        // Detect first arrival to max (gust) level
        if (windSpeedAbsoluteMagnitude > mLastWindSpeedAbsoluteMagnitude
            && std::abs(maxSpeedMagnitude) - windSpeedAbsoluteMagnitude < 0.001f)
        {
            PlayOneShotMultipleChoiceSound(
                SoundType::WindGust,
                smoothedWindVolume,
                true);
        }
    }

    mLastWindSpeedAbsoluteMagnitude = windSpeedAbsoluteMagnitude;
}

void SoundController::OnRainUpdated(float density)
{
    // Set the volume - starts automatically if greater than zero
    mRainSound.SetVolume(density / 0.4f * 100.0f);
}

void SoundController::OnThunder()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Thunder,
        100.0f,
        true);
}

void SoundController::OnLightning()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::Lightning,
        100.0f,
        true);
}

void SoundController::OnLightFlicker(
    DurationShortLongType duration,
    bool isUnderwater,
    unsigned int size)
{
    PlayDslUOneShotMultipleChoiceSound(
        SoundType::LightFlicker,
        duration,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnEngineMonitorCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex /*instanceIndex*/,
    ElectricalMaterial const & electricalMaterial,
    float /*thrustMagnitude*/,
    float /*rpm*/,
    std::optional<ElectricalPanelElementMetadata> const & /*panelElementMetadata*/)
{
    // Associate sound type with this element
    if (electricalMaterial.EngineType == ElectricalMaterial::EngineElementType::Diesel)
    {
        mLoopedSounds.AddSoundTypeForInstanceId(electricalElementId, SoundType::EngineDiesel1);
    }
    else if (electricalMaterial.EngineType == ElectricalMaterial::EngineElementType::Outboard)
    {
        mLoopedSounds.AddSoundTypeForInstanceId(electricalElementId, SoundType::EngineOutboard1);
    }
    else if (electricalMaterial.EngineType == ElectricalMaterial::EngineElementType::Steam)
    {
        if (electricalMaterial.EnginePower < 2000.0f)
        {
            mLoopedSounds.AddSoundTypeForInstanceId(electricalElementId, SoundType::EngineSteam1);
        }
        else
        {
            mLoopedSounds.AddSoundTypeForInstanceId(electricalElementId, SoundType::EngineSteam2);
        }
    }
}

void SoundController::OnWaterPumpCreated(
    ElectricalElementId electricalElementId,
    ElectricalElementInstanceIndex /*instanceIndex*/,
    ElectricalMaterial const & /*electricalMaterial*/,
    float /*normalizedForce*/,
    std::optional<ElectricalPanelElementMetadata> const & /*panelElementMetadata*/)
{
    // Associate sound type with this element
    mLoopedSounds.AddSoundTypeForInstanceId(electricalElementId, SoundType::WaterPump);
}

void SoundController::OnSwitchToggled(
    ElectricalElementId /*electricalElementId*/,
    ElectricalState newState)
{
    PlayOneShotMultipleChoiceSound(
        newState == ElectricalState::On ? SoundType::InteractiveSwitchOn : SoundType::InteractiveSwitchOff,
        100.0f,
        false);
}

void SoundController::OnEngineControllerUpdated(
    ElectricalElementId /*electricalElementId*/,
    int /*telegraphValue*/)
{
    PlayOneShotMultipleChoiceSound(
        SoundType::EngineTelegraph,
        100.0f,
        false);
}

void SoundController::OnEngineMonitorUpdated(
    ElectricalElementId electricalElementId,
    float /*thrustMagnitude*/,
    float rpm)
{
    if (rpm != 0.0f)
    {
        // Calculate pitch and volume
        float volume;
        float pitch;
        switch (mLoopedSounds.GetSoundTypeForInstanceId(electricalElementId))
        {
            case SoundType::EngineDiesel1:
            case SoundType::EngineOutboard1:
            {
                volume = 50.0f;
                pitch = rpm;
                break;
            }

            case SoundType::EngineSteam1:
            {
                volume = 50.0f;
                pitch = SmoothStep(0.0f, 1.0f, rpm) / 0.156f;  // rpm=0.25 => pitch=1; rpm=1.0 => pitch=6.4
                break;
            }

            case SoundType::EngineSteam2:
            {
                volume = 45.0f;
                pitch = SmoothStep(0.0f, 1.0f, rpm) / 0.334f;  // rpm=0.25 => pitch=0.47; rpm=1.0 => pitch=3.3
                break;
            }

            default:
            {
                assert(false);
                volume = 100.0f;
                pitch = 1.0;
                break;
            }
        }

        // Make sure sound is running
        if (!mLoopedSounds.IsPlaying(electricalElementId))
            mLoopedSounds.Start(electricalElementId, false, volume);

        // Set pitch
        mLoopedSounds.SetPitch(electricalElementId, pitch);
    }
    else
    {
        // Make sure sound is not running
        mLoopedSounds.Stop(electricalElementId);
    }
}

void SoundController::OnShipSoundUpdated(
    ElectricalElementId electricalElementId,
    ElectricalMaterial const & electricalMaterial,
    bool isPlaying,
    bool isUnderwater)
{
    if (isPlaying)
    {
        SoundType soundType = SoundType::ShipBell1; // Arbitrary
        switch (electricalMaterial.ShipSoundType)
        {
            case ElectricalMaterial::ShipSoundElementType::Bell1:
            {
                soundType = SoundType::ShipBell1;
                break;
            }

            case ElectricalMaterial::ShipSoundElementType::Bell2:
            {
                soundType = SoundType::ShipBell2;
                break;
            }

            case ElectricalMaterial::ShipSoundElementType::Horn1:
            {
                soundType = SoundType::ShipHorn1;
                break;
            }

            case ElectricalMaterial::ShipSoundElementType::Horn2:
            {
                soundType = SoundType::ShipHorn2;
                break;
            }

            case ElectricalMaterial::ShipSoundElementType::Horn3:
            {
                soundType = SoundType::ShipHorn3;
                break;
            }

            case ElectricalMaterial::ShipSoundElementType::Klaxon1:
            {
                soundType = SoundType::ShipKlaxon1;
                break;
            }
        }

        mLoopedSounds.Start(
            electricalElementId,
            soundType,
            isUnderwater,
            100.0f);
    }
    else
    {
        mLoopedSounds.Stop(electricalElementId);
    }
}

void SoundController::OnWaterPumpUpdated(
    ElectricalElementId electricalElementId,
    float normalizedForce)
{
    if (normalizedForce != 0.0f)
    {
        // Make sure sound is running
        if (!mLoopedSounds.IsPlaying(electricalElementId))
            mLoopedSounds.Start(electricalElementId, false, 100.0f);

        // Set pitch
        mLoopedSounds.SetPitch(electricalElementId, normalizedForce);
    }
    else
    {
        // Make sure sound is not running
        mLoopedSounds.Stop(electricalElementId);
    }
}

void SoundController::OnBombPlaced(
    BombId /*bombId*/,
    BombType /*bombType*/,
    bool isUnderwater)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::BombAttached,
        isUnderwater,
        100.0f,
        true);
}

void SoundController::OnBombRemoved(
    BombId /*bombId*/,
    BombType /*bombType*/,
    std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        PlayUOneShotMultipleChoiceSound(
            SoundType::BombDetached,
            *isUnderwater,
            100.0f,
            true);
    }
}

void SoundController::OnBombExplosion(
    BombType bombType,
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        BombType::AntiMatterBomb == bombType
            ? SoundType::AntiMatterBombExplosion
            : SoundType::BombExplosion,
        isUnderwater,
        std::max(
            100.0f,
            50.0f * size),
        true);
}

void SoundController::OnRCBombPing(
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::RCBombPing,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnTimerBombFuse(
    BombId bombId,
    std::optional<bool> isFast)
{
    if (!!isFast)
    {
        if (*isFast)
        {
            // Start fast

            // See if this bomb is emitting a slow fuse sound; if so, remove it
            // and update slow fuse sound
            mTimerBombSlowFuseSound.StopSoundForObject(bombId);

            // Start fast fuse sound
            mTimerBombFastFuseSound.StartSoundForObject(bombId);
        }
        else
        {
            // Start slow

            // See if this bomb is emitting a fast fuse sound; if so, remove it
            // and update fast fuse sound
            mTimerBombFastFuseSound.StopSoundForObject(bombId);

            // Start slow fuse sound
            mTimerBombSlowFuseSound.StartSoundForObject(bombId);
        }
    }
    else
    {
        // Stop the sound, whichever it is
        mTimerBombSlowFuseSound.StopSoundForObject(bombId);
        mTimerBombFastFuseSound.StopSoundForObject(bombId);
    }
}

void SoundController::OnTimerBombDefused(
    bool isUnderwater,
    unsigned int size)
{
    PlayUOneShotMultipleChoiceSound(
        SoundType::TimerBombDefused,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnAntiMatterBombContained(
    BombId bombId,
    bool isContained)
{
    if (isContained)
    {
        // Start sound
        mAntiMatterBombContainedSounds.StartSoundAlternativeForObject(bombId);
    }
    else
    {
        // Stop sound
        mAntiMatterBombContainedSounds.StopSoundAlternativeForObject(bombId);
    }
}

void SoundController::OnAntiMatterBombPreImploding()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::AntiMatterBombPreImplosion,
        100.0f,
        true);
}

void SoundController::OnAntiMatterBombImploding()
{
    PlayOneShotMultipleChoiceSound(
        SoundType::AntiMatterBombImplosion,
        100.0f,
        false);
}

void SoundController::OnWatertightDoorOpened(
    bool /*isUnderwater*/,
    unsigned int size)
{
    PlayOneShotMultipleChoiceSound(
        SoundType::WatertightDoorOpened,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

void SoundController::OnWatertightDoorClosed(
    bool /*isUnderwater*/,
    unsigned int size)
{
    PlayOneShotMultipleChoiceSound(
        SoundType::WatertightDoorClosed,
        std::max(
            100.0f,
            30.0f * size),
        true);
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::PlayMSUOneShotMultipleChoiceSound(
    SoundType soundType,
    StructuralMaterial::MaterialSoundType materialSound,
    unsigned int size,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    // Convert size
    SizeType sizeType;
    if (size < 2)
        sizeType = SizeType::Small;
    else if (size < 10)
        sizeType = SizeType::Medium;
    else
        sizeType = SizeType::Large;

    LogDebug("MSUSound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(materialSound),
        ",",
        static_cast<int>(sizeType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, sizeType, isUnderwater));
    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // Find a smaller one
        for (int s = static_cast<int>(sizeType) - 1; s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, static_cast<SizeType>(s), isUnderwater));
            if (it != mMSUOneShotMultipleChoiceSounds.end())
            {
                break;
            }
        }
    }

    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // Find this or smaller size with different underwater
        for (int s = static_cast<int>(sizeType); s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound, static_cast<SizeType>(s), !isUnderwater));
            if (it != mMSUOneShotMultipleChoiceSounds.end())
            {
                break;
            }
        }
    }

    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayMOneShotMultipleChoiceSound(
    SoundType soundType,
    StructuralMaterial::MaterialSoundType materialSound,
    float volume,
    bool isInterruptible)
{
    LogDebug("MSound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(materialSound),
        ">");

    //
    // Find vector
    //

    auto it = mMOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, materialSound));
    if (it == mMOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayDslUOneShotMultipleChoiceSound(
    SoundType soundType,
    DurationShortLongType duration,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    LogDebug("DslUSound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(duration),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mDslUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, duration, isUnderwater));
    if (it == mDslUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayUOneShotMultipleChoiceSound(
    SoundType soundType,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    LogDebug("USound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, isUnderwater));
    if (it == mUOneShotMultipleChoiceSounds.end())
    {
        // Find different underwater
        it = mUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, !isUnderwater));
    }

    if (it == mUOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::PlayOneShotMultipleChoiceSound(
    SoundType soundType,
    float volume,
    bool isInterruptible)
{
    LogDebug("Sound: <",
        static_cast<int>(soundType),
        ">");

    //
    // Find vector
    //

    auto it = mOneShotMultipleChoiceSounds.find(std::make_tuple(soundType));
    if (it == mOneShotMultipleChoiceSounds.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlayOneShotMultipleChoiceSound(
        soundType,
        it->second,
        volume,
        isInterruptible);
}

void SoundController::ChooseAndPlayOneShotMultipleChoiceSound(
    SoundType soundType,
    OneShotMultipleChoiceSound & sound,
    float volume,
    bool isInterruptible)
{
    //
    // Choose sound buffer
    //

    sf::SoundBuffer * chosenSoundBuffer = nullptr;

    assert(!sound.SoundBuffers.empty());
    if (1 == sound.SoundBuffers.size())
    {
        // Nothing to choose
        chosenSoundBuffer = sound.SoundBuffers[0].get();
    }
    else
    {
        assert(sound.SoundBuffers.size() >= 2);

        // Choose randomly, but avoid choosing the last-chosen sound again
        size_t chosenSoundIndex = GameRandomEngine::GetInstance().ChooseNew(
            sound.SoundBuffers.size(),
            sound.LastPlayedSoundIndex);

        chosenSoundBuffer = sound.SoundBuffers[chosenSoundIndex].get();

        sound.LastPlayedSoundIndex = chosenSoundIndex;
    }

    assert(nullptr != chosenSoundBuffer);

    PlayOneShotSound(
        soundType,
        chosenSoundBuffer,
        volume,
        isInterruptible);
}

void SoundController::PlayOneShotSound(
    SoundType soundType,
    sf::SoundBuffer * soundBuffer,
    float volume,
    bool isInterruptible)
{
    assert(nullptr != soundBuffer);

    //
    // Make sure there isn't already a sound with this sound buffer that started
    // playing too recently;
    // if there is, adjust its volume
    //

    auto & thisTypeCurrentlyPlayingSounds = mCurrentlyPlayingOneShotSounds[soundType];

    auto const now = GameWallClock::GetInstance().Now();
    auto const minDeltaTimeSoundForType = GetMinDeltaTimeSoundForType(soundType);

    for (auto & playingSound : thisTypeCurrentlyPlayingSounds)
    {
        assert(!!playingSound.Sound);
        if (playingSound.Sound->getBuffer() == soundBuffer
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - playingSound.StartedTimestamp) < minDeltaTimeSoundForType)
        {
            playingSound.Sound->addVolume(volume);
            return;
        }
    }


    //
    // Make sure there's room for this sound
    //

    auto const maxPlayingSoundsForThisType = GetMaxPlayingSoundsForType(soundType);

    if (thisTypeCurrentlyPlayingSounds.size() >= maxPlayingSoundsForThisType)
    {
        ScavengeStoppedSounds(thisTypeCurrentlyPlayingSounds);

        if (thisTypeCurrentlyPlayingSounds.size() >= maxPlayingSoundsForThisType)
        {
            // Need to stop the (expendable) sound that's been playing for the longest
            ScavengeOldestSound(thisTypeCurrentlyPlayingSounds);
        }
    }

    assert(thisTypeCurrentlyPlayingSounds.size() < maxPlayingSoundsForThisType);



    //
    // Create and play sound
    //

    std::unique_ptr<GameSound> sound = std::make_unique<GameSound>(
        *soundBuffer,
        volume,
        mMasterEffectsVolume,
        mMasterEffectsMuted);

    sound->play();

    thisTypeCurrentlyPlayingSounds.emplace_back(
        soundType,
        std::move(sound),
        now,
        isInterruptible);
}

void SoundController::ScavengeStoppedSounds(std::vector<PlayingSound> & playingSounds)
{
    for (auto it = playingSounds.begin(); it != playingSounds.end(); /*incremented in loop*/)
    {
        assert(!!it->Sound);
        if (sf::Sound::Status::Stopped == it->Sound->getStatus())
        {
            // Scavenge
            it = playingSounds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void SoundController::ScavengeOldestSound(std::vector<PlayingSound> & playingSounds)
{
    assert(!playingSounds.empty());

    //
    // Two choices, in order of priority:
    // 1) Interruptible
    // 2) Non interruptible
    //

    size_t iOldestInterruptibleSound = std::numeric_limits<size_t>::max();
    auto oldestInterruptibleSoundStartTimestamp = GameWallClock::time_point::max();
    size_t iOldestNonInterruptibleSound = std::numeric_limits<size_t>::max();
    auto oldestNonInterruptibleSoundStartTimestamp = GameWallClock::time_point::max();
    for (size_t i = 0; i < playingSounds.size(); ++i)
    {
        if (playingSounds[i].StartedTimestamp < oldestNonInterruptibleSoundStartTimestamp)
        {
            iOldestNonInterruptibleSound = i;
            oldestNonInterruptibleSoundStartTimestamp = playingSounds[i].StartedTimestamp;
        }

        if (playingSounds[i].StartedTimestamp < oldestInterruptibleSoundStartTimestamp
            && playingSounds[i].IsInterruptible)
        {
            iOldestInterruptibleSound = i;
            oldestInterruptibleSoundStartTimestamp = playingSounds[i].StartedTimestamp;
        }
    }

    size_t iSoundToStop;
    if (oldestInterruptibleSoundStartTimestamp != GameWallClock::time_point::max())
    {
        iSoundToStop = iOldestInterruptibleSound;
    }
    else
    {
        assert((oldestNonInterruptibleSoundStartTimestamp != GameWallClock::time_point::max()));
        iSoundToStop = iOldestNonInterruptibleSound;
    }

    assert(!!playingSounds[iSoundToStop].Sound);
    playingSounds[iSoundToStop].Sound->stop();
    playingSounds.erase(playingSounds.begin() + iSoundToStop);
}