/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundController.h"

#include <GameLib/GameException.h>
#include <GameLib/Log.h>
#include <GameLib/Material.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <regex>

constexpr float SawVolume = 50.0f;
constexpr float SawedVolume = 80.0f;
constexpr std::chrono::milliseconds SawedInertiaDuration = std::chrono::milliseconds(200);
constexpr float SinkingMusicVolume = 20.0f;
constexpr float WaveSplashTriggerSize = 0.5f;

SoundController::SoundController(
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
    : mResourceLoader(std::move(resourceLoader))
    // State
    , mMasterEffectsVolume(100.0f)
    , mMasterMusicVolume(100.0f)
    , mMasterEffectsMuted(false)
    , mMasterMusicMuted(false)
    , mPlayBreakSounds(true)
    , mPlayStressSounds(true)
    , mPlaySinkingMusic(true)
    , mLastWaterSplashed(0.0f)
    , mCurrentWaterSplashedTrigger(WaveSplashTriggerSize)
    // One-shot sounds
    , mMSUOneShotMultipleChoiceSounds()
    , mDslUOneShotMultipleChoiceSounds()
    , mUOneShotMultipleChoiceSounds()
    , mOneShotMultipleChoiceSounds()
    , mCurrentlyPlayingOneShotSounds()
    // Continuous sounds
    , mSawedMetalSound(SawedInertiaDuration)
    , mSawedWoodSound(SawedInertiaDuration)
    , mSawAbovewaterSound()
    , mSawUnderwaterSound()
    , mDrawSound()
    , mSwirlSound()
    , mWaterRushSound()
    , mWaterSplashSound()
    , mTimerBombSlowFuseSound()
    , mTimerBombFastFuseSound()
    , mAntiMatterBombContainedSounds()
    // Music
    , mSinkingMusic(SinkingMusicVolume, mMasterMusicVolume, mMasterMusicMuted)
{    
    //
    // Initialize Music
    //

    if (!mSinkingMusic.openFromFile(mResourceLoader->GetMusicFilepath("sinking_ship").string()))
    {
        throw GameException("Cannot load \"sinking_ship\" music");
    }

    mSinkingMusic.setLoop(true);


    //
    // Initialize Sounds
    //

    auto soundNames = mResourceLoader->GetSoundNames();
    for (size_t i = 0; i < soundNames.size(); ++i)
    {
        std::string const & soundName = soundNames[i];

        // Notify progress
        progressCallback(static_cast<float>(i + 1) / static_cast<float>(soundNames.size()), "Loading sounds...");
        

        //
        // Load sound buffer
        //

        std::unique_ptr<sf::SoundBuffer> soundBuffer = std::make_unique<sf::SoundBuffer>();
        if (!soundBuffer->loadFromFile(mResourceLoader->GetSoundFilepath(soundName).string()))
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
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }
            else
            {
                mSawAbovewaterSound.Initialize(
                    std::move(soundBuffer),
                    SawVolume,
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }            
        }
        else if (soundType == SoundType::Draw)
        {
            mDrawSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
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
            Material::SoundProperties::SoundElementType soundElementType = Material::SoundProperties::StrToSoundElementType(mMatch[2].str());

            if (Material::SoundProperties::SoundElementType::Metal == soundElementType)
            {
                mSawedMetalSound.Initialize(
                    std::move(soundBuffer),
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }
            else
            {
                mSawedWoodSound.Initialize(
                    std::move(soundBuffer),
                    mMasterEffectsVolume,
                    mMasterEffectsMuted);
            }
        }
        else if (soundType == SoundType::Swirl)
        {
            mSwirlSound.Initialize(
                std::move(soundBuffer),
                100.0f,
                mMasterEffectsVolume,
                mMasterEffectsMuted);
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
        else if (soundType == SoundType::Break || soundType == SoundType::Destroy || soundType == SoundType::Stress)
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

            // 1. Parse SoundElementType
            Material::SoundProperties::SoundElementType soundElementType = Material::SoundProperties::StrToSoundElementType(msuMatch[2].str());

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

            mMSUOneShotMultipleChoiceSounds[std::make_tuple(soundType, soundElementType, sizeType, isUnderwater)]
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
                || soundType == SoundType::AntiMatterBombPreImplosion
                || soundType == SoundType::AntiMatterBombImplosion)
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
    for (auto const & playingSound : mCurrentlyPlayingOneShotSounds)
    {
        if (isPaused)
        {
            if (sf::Sound::Status::Playing == playingSound.Sound->getStatus()
                && playingSound.Type != SoundType::Destroy) // TODOHERE
                playingSound.Sound->pause();
        }
        else
        {
            if (sf::Sound::Status::Paused == playingSound.Sound->getStatus())
                playingSound.Sound->play();
        }
    }

    // We don't pause the continuous tool sounds

    mWaterRushSound.SetPaused(isPaused);
    mWaterSplashSound.SetPaused(isPaused);
    mTimerBombSlowFuseSound.SetPaused(isPaused);
    mTimerBombFastFuseSound.SetPaused(isPaused);
    mAntiMatterBombContainedSounds.SetPaused(isPaused);

    if (isPaused)
    {
        if (sf::Sound::Status::Playing == mSinkingMusic.getStatus())
            mSinkingMusic.pause();
    }
    else
    {
        if (sf::Sound::Status::Paused == mSinkingMusic.getStatus())
            mSinkingMusic.play();
    }
}

void SoundController::SetMuted(bool isMuted)
{
    sf::Listener::setGlobalVolume(isMuted ? 0.0f : 100.0f);
}

void SoundController::SetMasterEffectsVolume(float volume)
{
    mMasterEffectsVolume = volume;

    for (auto const & playingSound : mCurrentlyPlayingOneShotSounds)
    {
        playingSound.Sound->setMasterVolume(mMasterEffectsVolume);
    }

    mSawedMetalSound.SetMasterVolume(mMasterEffectsVolume);
    mSawedWoodSound.SetMasterVolume(mMasterEffectsVolume);
    mSawAbovewaterSound.SetMasterVolume(mMasterEffectsVolume);
    mSawUnderwaterSound.SetMasterVolume(mMasterEffectsVolume);
    mDrawSound.SetMasterVolume(mMasterEffectsVolume);
    mSwirlSound.SetMasterVolume(mMasterEffectsVolume);

    mWaterRushSound.SetMasterVolume(mMasterEffectsVolume);
    mWaterSplashSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombSlowFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mTimerBombFastFuseSound.SetMasterVolume(mMasterEffectsVolume);
    mAntiMatterBombContainedSounds.SetMasterVolume(mMasterEffectsVolume);
}

void SoundController::SetMasterMusicVolume(float volume)
{
    mMasterMusicVolume = volume;

    mSinkingMusic.setMasterVolume(volume);
}

void SoundController::SetMasterEffectsMuted(bool isMuted)
{
    mMasterEffectsMuted = isMuted;

    for (auto const & playingSound : mCurrentlyPlayingOneShotSounds)
    {
        playingSound.Sound->setMuted(mMasterEffectsMuted);
    }

    mSawedMetalSound.SetMuted(mMasterEffectsMuted);
    mSawedWoodSound.SetMuted(mMasterEffectsMuted);
    mSawAbovewaterSound.SetMuted(mMasterEffectsMuted);
    mSawUnderwaterSound.SetMuted(mMasterEffectsMuted);
    mDrawSound.SetMuted(mMasterEffectsMuted);;
    mSwirlSound.SetMuted(mMasterEffectsMuted);;

    mWaterRushSound.SetMuted(mMasterEffectsMuted);;
    mWaterSplashSound.SetMuted(mMasterEffectsMuted);;
    mTimerBombSlowFuseSound.SetMuted(mMasterEffectsMuted);;
    mTimerBombFastFuseSound.SetMuted(mMasterEffectsMuted);;
    mAntiMatterBombContainedSounds.SetMuted(mMasterEffectsMuted);;
}

void SoundController::SetMasterMusicMuted(bool isMuted)
{
    mMasterMusicMuted = isMuted;

    mSinkingMusic.setMuted(mMasterMusicMuted);
}

void SoundController::SetPlayBreakSounds(bool playBreakSounds)
{
    mPlayBreakSounds = playBreakSounds;

    if (!mPlayBreakSounds)
    {
        for (auto const & playingSound : mCurrentlyPlayingOneShotSounds)
        {
            if (SoundType::Break == playingSound.Type)
            {
                playingSound.Sound->stop();
            }
        }
    }
}

void SoundController::SetPlayStressSounds(bool playStressSounds)
{
    mPlayStressSounds = playStressSounds;

    if (!mPlayStressSounds)
    {
        for (auto const & playingSound : mCurrentlyPlayingOneShotSounds)
        {
            if (SoundType::Stress == playingSound.Type)
            {
                playingSound.Sound->stop();
            }
        }
    }
}

void SoundController::SetPlaySinkingMusic(bool playSinkingMusic)
{
    mPlaySinkingMusic = playSinkingMusic;

    if (!mPlaySinkingMusic)
    {
        mSinkingMusic.stop();
    }
}

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

void SoundController::PlaySwirlSound(bool /*isUnderwater*/)
{
    // At the moment we ignore the water-ness
    mSwirlSound.Start();
}

void SoundController::StopSwirlSound()
{
    mSwirlSound.Stop();
}

void SoundController::Update()
{
    // Silence the sawed sounds - this will be a nop in case 
    // they've just been started or will be started really soon
    mSawedMetalSound.SetVolume(0.0f);
    mSawedWoodSound.SetVolume(0.0f);
}

void SoundController::LowFrequencyUpdate()
{
    //
    // Scavenge stopped sounds
    //

    ScavengeStoppedSounds();
}

void SoundController::Reset()
{
    //
    // Stop and clear all sounds
    //

    for (auto & playingSound : mCurrentlyPlayingOneShotSounds)
    {
        assert(!!playingSound.Sound);
        if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
        {
            playingSound.Sound->stop();
        }
    }

    mCurrentlyPlayingOneShotSounds.clear();

    mSawedMetalSound.Reset();
    mSawedWoodSound.Reset();
    mSawAbovewaterSound.Reset();
    mSawUnderwaterSound.Reset();
    mDrawSound.Reset();
    mSwirlSound.Reset();

    mWaterRushSound.Reset();
    mWaterSplashSound.Reset();
    mTimerBombSlowFuseSound.Reset();
    mTimerBombFastFuseSound.Reset();
    mAntiMatterBombContainedSounds.Reset();

    //
    // Stop music
    //

    mSinkingMusic.stop();

    //
    // Reset state
    //

    mLastWaterSplashed = 0.0f;
    mCurrentWaterSplashedTrigger = WaveSplashTriggerSize;
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::OnDestroy(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUOneShotMultipleChoiceSound(
        SoundType::Destroy, 
        material, 
        size, 
        isUnderwater,
        70.0f,
        true);
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

void SoundController::OnStress(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    if (mPlayStressSounds)
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Stress,
            material,
            size,
            isUnderwater,
            10.0f,
            true);
    }
}

void SoundController::OnBreak(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    if (mPlayBreakSounds)
    {
        PlayMSUOneShotMultipleChoiceSound(
            SoundType::Break,
            material,
            size,
            isUnderwater,
            10.0f,
            true);
    }
}

void SoundController::OnSinkingBegin(unsigned int /*shipId*/)
{
    if (mPlaySinkingMusic)
    {
        if (sf::SoundSource::Status::Playing != mSinkingMusic.getStatus())
        {
            mSinkingMusic.play();
        }
    }
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

void SoundController::OnWaterTaken(float waterTaken)
{
    // 50 * (-1 / 2.4^(0.3 * x) + 1)
    float rushVolume = 50.f * (-1.f / std::pow(2.4f, 0.3f * std::abs(waterTaken)) + 1.f);
    mWaterRushSound.SetVolume(rushVolume);
    mWaterRushSound.Start();
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
            // 100 * (-1 / 1.8^(0.08 * x) + 1)
            //   3: 13.0
            float waveVolume = 20.f * (-1.f / std::pow(1.8f, 0.08f * std::abs(waterSplashed)) + 1.f);

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

    float splashVolume = 15.f * (-1.f / std::pow(1.3f, 0.01f * std::abs(waterSplashed)) + 1.f);
    mWaterSplashSound.SetVolume(splashVolume);
    mWaterSplashSound.Start();
}

void SoundController::OnBombPlaced(
    ObjectId /*bombId*/,
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
    ObjectId /*bombId*/,
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
    ObjectId bombId,
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
    ObjectId bombId,
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

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::PlayMSUOneShotMultipleChoiceSound(
    SoundType soundType,
    Material const * material,
    unsigned int size,
    bool isUnderwater,
    float volume,
    bool isInterruptible)
{
    assert(nullptr != material);

    if (!material->Sound)
        return;

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
        static_cast<int>(material->Sound->ElementType), 
        ",", 
        static_cast<int>(sizeType), 
        ",", 
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //
    
    auto it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, material->Sound->ElementType, sizeType, isUnderwater));
    if (it == mMSUOneShotMultipleChoiceSounds.end())
    {
        // Find a smaller one
        for (int s = static_cast<int>(sizeType) - 1; s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), isUnderwater));
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
            it = mMSUOneShotMultipleChoiceSounds.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), !isUnderwater));
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

    auto const now = std::chrono::steady_clock::now();

    for (auto const & currentlyPlayingSound : mCurrentlyPlayingOneShotSounds)
    {
        assert(!!currentlyPlayingSound.Sound);
        if (currentlyPlayingSound.Sound->getBuffer() == soundBuffer
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - currentlyPlayingSound.StartedTimestamp) < MinDeltaTimeSound)
        {
            currentlyPlayingSound.Sound->addVolume(volume);

            return;
        }
    }


    //
    // Make sure there's room for this sound
    //

    if (mCurrentlyPlayingOneShotSounds.size() >= MaxPlayingSounds)
    {
        ScavengeStoppedSounds();

        if (mCurrentlyPlayingOneShotSounds.size() >= MaxPlayingSounds)
        { 
            // Need to stop the (expendable) sound that's been playing for the longest
            ScavengeOldestSound(soundType);
        }
    }

    assert(mCurrentlyPlayingOneShotSounds.size() < MaxPlayingSounds);



    //
    // Create and play sound
    //

    std::unique_ptr<GameSound> sound = std::make_unique<GameSound>(
        *soundBuffer,
        volume,
        mMasterEffectsVolume,
        mMasterEffectsMuted);
    
    sound->play();    

    mCurrentlyPlayingOneShotSounds.emplace_back(
        soundType,
        std::move(sound),
        now,
        isInterruptible);
}

void SoundController::ScavengeStoppedSounds()
{
    for (size_t i = 0; i < mCurrentlyPlayingOneShotSounds.size(); /*incremented in loop*/)
    {
        assert(!!mCurrentlyPlayingOneShotSounds[i].Sound);
        if (sf::Sound::Status::Stopped == mCurrentlyPlayingOneShotSounds[i].Sound->getStatus())
        {
            // Scavenge
            mCurrentlyPlayingOneShotSounds.erase(mCurrentlyPlayingOneShotSounds.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

void SoundController::ScavengeOldestSound(SoundType soundType)
{
    assert(!mCurrentlyPlayingOneShotSounds.empty());

    //
    // Three choices, in order of priority:
    // 1) Same type
    // 2) Another type, interruptible
    // 3) Another type, non interruptible

    size_t iOldestSound = std::numeric_limits<size_t>::max();
    auto oldestSoundStartTimestamp = std::chrono::steady_clock::time_point::max();
    size_t iOldestInterruptibleSound = std::numeric_limits<size_t>::max();
    auto oldestInterruptibleSoundStartTimestamp = std::chrono::steady_clock::time_point::max();
    size_t iOldestSoundForType = std::numeric_limits<size_t>::max();
    auto oldestSoundForTypeStartTimestamp = std::chrono::steady_clock::time_point::max();    
    for (size_t i = 0; i < mCurrentlyPlayingOneShotSounds.size(); ++i)
    {
        if (mCurrentlyPlayingOneShotSounds[i].StartedTimestamp < oldestSoundStartTimestamp)
        {
            iOldestSound = i;
            oldestSoundStartTimestamp = mCurrentlyPlayingOneShotSounds[i].StartedTimestamp;
        }

        if (mCurrentlyPlayingOneShotSounds[i].StartedTimestamp < oldestInterruptibleSoundStartTimestamp
            && mCurrentlyPlayingOneShotSounds[i].IsInterruptible)
        {
            iOldestInterruptibleSound = i;
            oldestInterruptibleSoundStartTimestamp = mCurrentlyPlayingOneShotSounds[i].StartedTimestamp;
        }

        if (mCurrentlyPlayingOneShotSounds[i].StartedTimestamp < oldestSoundForTypeStartTimestamp
            && mCurrentlyPlayingOneShotSounds[i].Type == soundType)
        {
            iOldestSoundForType = i;
            oldestSoundForTypeStartTimestamp = mCurrentlyPlayingOneShotSounds[i].StartedTimestamp;
        }
    }

    size_t iStop;
    if (oldestSoundForTypeStartTimestamp != std::chrono::steady_clock::time_point::max())
    {
        iStop = iOldestSoundForType;
    }
    else if (oldestInterruptibleSoundStartTimestamp != std::chrono::steady_clock::time_point::max())
    {
        iStop = iOldestInterruptibleSound;
    }
    else
    {
        assert((oldestSoundStartTimestamp != std::chrono::steady_clock::time_point::max()));
        iStop = iOldestSound;
    }

    assert(!!mCurrentlyPlayingOneShotSounds[iStop].Sound);
    mCurrentlyPlayingOneShotSounds[iStop].Sound->stop();
    mCurrentlyPlayingOneShotSounds.erase(mCurrentlyPlayingOneShotSounds.begin() + iStop);
}
