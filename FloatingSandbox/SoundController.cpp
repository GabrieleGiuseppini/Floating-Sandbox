/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundController.h"

#include <GameLib/GameException.h>
#include <GameLib/GameRandomEngine.h>
#include <GameLib/Log.h>
#include <GameLib/Material.h>

#include <cassert>
#include <limits>
#include <regex>

static constexpr float SinkingMusicVolume = 20.0f;

SoundController::SoundController(
    std::shared_ptr<ResourceLoader> resourceLoader,
    ProgressCallback const & progressCallback)
    : mResourceLoader(std::move(resourceLoader))
    , mCurrentVolume(100.0f)
    , mPlaySinkingMusic(true)
    // State
    , mBombsEmittingSlowFuseSounds()
    , mBombsEmittingFastFuseSounds()
    // One-shot sounds
    , mMSUSoundBuffers()
    , mDslUSoundBuffers()
    , mUSoundBuffers()
    , mCurrentlyPlayingSounds()
    // Continuous sounds
    , mSawAbovewaterSound()
    , mSawUnderwaterSound()
    , mDrawSound()
    , mSwirlSound()
    , mWaterRushSound()
    , mWaterSplashSound()
    , mTimerBombSlowFuseSound()
    , mTimerBombFastFuseSound()
    // Music
    , mSinkingMusic()
{    
    //
    // Initialize Music
    //

    if (!mSinkingMusic.openFromFile(mResourceLoader->GetMusicFilepath("sinking_ship").string()))
    {
        throw GameException("Cannot load \"sinking_ship\" music");
    }

    mSinkingMusic.setLoop(true);
    mSinkingMusic.setVolume(SinkingMusicVolume);


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
                mSawUnderwaterSound.Initialize(std::move(soundBuffer));
            }
            else
            {
                mSawAbovewaterSound.Initialize(std::move(soundBuffer));
            }            
        }
        else if (soundType == SoundType::Draw)
        {
            mDrawSound.Initialize(std::move(soundBuffer));
        }
        else if (soundType == SoundType::Swirl)
        {
            mSwirlSound.Initialize(std::move(soundBuffer));
        }
        else if (soundType == SoundType::WaterRush)
        {
            mWaterRushSound.Initialize(std::move(soundBuffer));
        }
        else if (soundType == SoundType::WaterSplash)
        {
            mWaterSplashSound.Initialize(std::move(soundBuffer));
        }
        else if (soundType == SoundType::TimerBombSlowFuse)
        {
            mTimerBombSlowFuseSound.Initialize(std::move(soundBuffer));
        }
        else if (soundType == SoundType::TimerBombFastFuse)
        {
            mTimerBombFastFuseSound.Initialize(std::move(soundBuffer));
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

            mMSUSoundBuffers[std::make_tuple(soundType, soundElementType, sizeType, isUnderwater)]
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

            mDslUSoundBuffers[std::make_tuple(soundType, durationType, isUnderwater)]
                .SoundBuffers.emplace_back(std::move(soundBuffer));
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

            mUSoundBuffers[std::make_tuple(soundType, isUnderwater)]
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
    for (auto const & playingSound : mCurrentlyPlayingSounds)
    {
        if (isPaused)
        {
            if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
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

void SoundController::SetMute(bool isMute)
{
    if (isMute)
        sf::Listener::setGlobalVolume(0.0f);
    else
        sf::Listener::setGlobalVolume(mCurrentVolume);
}

void SoundController::SetVolume(float volume)
{
    mCurrentVolume = volume;
    sf::Listener::setGlobalVolume(mCurrentVolume);
}

void SoundController::SetPlaySinkingMusic(bool playSinkingMusic)
{
    if (playSinkingMusic)
        mSinkingMusic.setVolume(SinkingMusicVolume);
    else
        mSinkingMusic.setVolume(0.0f);

    mPlaySinkingMusic = playSinkingMusic;
}

void SoundController::HighFrequencyUpdate()
{
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

    for (auto & playingSound : mCurrentlyPlayingSounds)
    {
        assert(!!playingSound.Sound);
        if (sf::Sound::Status::Playing == playingSound.Sound->getStatus())
        {
            playingSound.Sound->stop();
        }
    }

    mCurrentlyPlayingSounds.clear();

    mSawAbovewaterSound.Stop();
    mSawUnderwaterSound.Stop();
    mDrawSound.Stop();
    mSwirlSound.Stop();
    mWaterRushSound.Stop();
    mWaterSplashSound.Stop();
    mTimerBombSlowFuseSound.Stop();
    mTimerBombFastFuseSound.Stop();

    //
    // Stop music
    //

    mSinkingMusic.stop();

    //
    // Reset state
    //

    mBombsEmittingSlowFuseSounds.clear();
    mBombsEmittingFastFuseSounds.clear();
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::OnDestroy(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Destroy, 
        material, 
        size, 
        isUnderwater,
        50.0f);
}

void SoundController::OnSaw(std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        if (*isUnderwater)
        {
            mSawUnderwaterSound.Start();
            mSawAbovewaterSound.Stop();            
        }
        else
        {
            mSawAbovewaterSound.Start();
            mSawUnderwaterSound.Stop();            
        }
    }
    else
    {
        mSawAbovewaterSound.Stop();
        mSawUnderwaterSound.Stop();
    }
}

void SoundController::OnDraw(std::optional<bool> isUnderwater)
{    
    if (!!isUnderwater)    
    {
        // At the moment we ignore the water-ness
        mDrawSound.Start();
    }
    else
    {
        mDrawSound.Stop();
    }    
}

void SoundController::OnSwirl(std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        // At the moment we ignore the water-ness
        mSwirlSound.Start();
    }
    else
    {
        mSwirlSound.Stop();
    }
}

void SoundController::OnPinToggled(
    bool isPinned,
    bool isUnderwater)
{
    PlayUSound(
        isPinned ? SoundType::PinPoint : SoundType::UnpinPoint, 
        isUnderwater,
        100.0f);
}

void SoundController::OnStress(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Stress,
        material,
        size,
        isUnderwater,
        50.0f);
}

void SoundController::OnBreak(
    Material const * material,
    bool isUnderwater,
    unsigned int size)
{
    assert(nullptr != material);

    PlayMSUSound(
        SoundType::Break, 
        material, 
        size, 
        isUnderwater,
        50.0f);
}

void SoundController::OnSinkingBegin(unsigned int /*shipId*/)
{
    if (sf::SoundSource::Status::Playing != mSinkingMusic.getStatus())
    {
        mSinkingMusic.play();
    }
}

void SoundController::OnLightFlicker(
    DurationShortLongType duration,
    bool isUnderwater,
    unsigned int size)
{
    PlayDslUSound(
        SoundType::LightFlicker,
        duration,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
}

void SoundController::OnWaterTaken(float /*waterTaken*/)
{
    // Not responding to this event at the moment
}

void SoundController::OnWaterSplashed(float waterSplashed)
{
    // 100 * (-1 / 1.3^(0.004 * x) + 1)
    // @200: 19
    // @1000: 65
    float splashVolume = 100.f * (-1.f / std::pow(1.3f, 0.004f * std::abs(waterSplashed)) + 1.f);
    mWaterSplashSound.SetVolume(splashVolume);
    mWaterSplashSound.Start();

    float rushVolume = 50.f * (-1.f / std::pow(2.0f, 0.1f * std::abs(waterSplashed)) + 1.f);
    mWaterRushSound.SetVolume(rushVolume);
    mWaterRushSound.Start();
}

void SoundController::OnBombPlaced(
    ObjectId /*bombId*/,
    BombType /*bombType*/,
    bool isUnderwater) 
{
    PlayUSound(
        SoundType::BombAttached, 
        isUnderwater,
        100.0f);
}

void SoundController::OnBombRemoved(
    ObjectId /*bombId*/,
    BombType /*bombType*/,
    std::optional<bool> isUnderwater)
{
    if (!!isUnderwater)
    {
        PlayUSound(
            SoundType::BombDetached,
            *isUnderwater,
            100.0f);
    }
}

void SoundController::OnBombExplosion(
    bool isUnderwater,
    unsigned int size)
{
    PlayUSound(
        SoundType::Explosion, 
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
}

void SoundController::OnRCBombPing(
    bool isUnderwater,
    unsigned int size)
{
    PlayUSound(
        SoundType::RCBombPing, 
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
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
            if (mBombsEmittingSlowFuseSounds.erase(bombId) != 0)
            {
                UpdateContinuousSound(mBombsEmittingSlowFuseSounds.size(), mTimerBombSlowFuseSound);
            }

            // Add bomb to set of timer bombs emitting fast fuse sound, and
            // update fast fuse sound
            assert(0 == mBombsEmittingFastFuseSounds.count(bombId));
            mBombsEmittingFastFuseSounds.insert(bombId);
            UpdateContinuousSound(mBombsEmittingFastFuseSounds.size(), mTimerBombFastFuseSound);
        }
        else
        {
            // Start slow

            // See if this bomb is emitting a fast fuse sound; if so, remove it 
            // and update fast fuse sound
            if (mBombsEmittingFastFuseSounds.erase(bombId) != 0)
            {
                UpdateContinuousSound(mBombsEmittingFastFuseSounds.size(), mTimerBombFastFuseSound);
            }

            // Add bomb to set of timer bombs emitting slow fuse sound, and
            // update slow fuse sound
            assert(0 == mBombsEmittingSlowFuseSounds.count(bombId));
            mBombsEmittingSlowFuseSounds.insert(bombId);
            UpdateContinuousSound(mBombsEmittingSlowFuseSounds.size(), mTimerBombSlowFuseSound);
        }
    }
    else
    { 
        // Stop the sound
        if (mBombsEmittingSlowFuseSounds.erase(bombId) != 0)
        {
            UpdateContinuousSound(mBombsEmittingSlowFuseSounds.size(), mTimerBombSlowFuseSound);
        }
        else
        {
            if (mBombsEmittingFastFuseSounds.erase(bombId) != 0)
            {
                UpdateContinuousSound(mBombsEmittingFastFuseSounds.size(), mTimerBombFastFuseSound);
            }
        }
    }
}

void SoundController::OnTimerBombDefused(
    bool isUnderwater,
    unsigned int size)
{
    PlayUSound(
        SoundType::TimerBombDefused,
        isUnderwater,
        std::max(
            100.0f,
            30.0f * size));
}

///////////////////////////////////////////////////////////////////////////////////////

void SoundController::PlayMSUSound(
    SoundType soundType,
    Material const * material,
    unsigned int size,
    bool isUnderwater,
    float volume)
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
    
    auto it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, sizeType, isUnderwater));

    if (it == mMSUSoundBuffers.end())
    {
        // Find a smaller one
        for (int s = static_cast<int>(sizeType) - 1; s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), isUnderwater));
            if (it != mMSUSoundBuffers.end())
            {
                break;
            }
        }
    }

    if (it == mMSUSoundBuffers.end())
    {
        // Find this or smaller size with different underwater
        for (int s = static_cast<int>(sizeType); s >= static_cast<int>(SizeType::Min); --s)
        {
            it = mMSUSoundBuffers.find(std::make_tuple(soundType, material->Sound->ElementType, static_cast<SizeType>(s), !isUnderwater));
            if (it != mMSUSoundBuffers.end())
            {
                break;
            }
        }
    }

    if (it == mMSUSoundBuffers.end())
    {
        // No luck
        return;
    }

    
    //
    // Play sound
    //

    ChooseAndPlaySound(
        soundType,
        it->second,
        volume);
}

void SoundController::PlayDslUSound(
    SoundType soundType,
    DurationShortLongType duration,
    bool isUnderwater,
    float volume)
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

    auto it = mDslUSoundBuffers.find(std::make_tuple(soundType, duration, isUnderwater));
    if (it == mDslUSoundBuffers.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlaySound(
        soundType,
        it->second,
        volume);
}

void SoundController::PlayUSound(
    SoundType soundType,
    bool isUnderwater,
    float volume)
{
    LogDebug("USound: <",
        static_cast<int>(soundType),
        ",",
        static_cast<int>(isUnderwater),
        ">");

    //
    // Find vector
    //

    auto it = mUSoundBuffers.find(std::make_tuple(soundType, isUnderwater));
    if (it == mUSoundBuffers.end())
    {
        // Find different underwater
        it = mUSoundBuffers.find(std::make_tuple(soundType, !isUnderwater));
    }

    if (it == mUSoundBuffers.end())
    {
        // No luck
        return;
    }


    //
    // Play sound
    //

    ChooseAndPlaySound(
        soundType,
        it->second,
        volume);
}

void SoundController::ChooseAndPlaySound(
    SoundType soundType,
    MultipleSoundChoiceInfo & multipleSoundChoiceInfo,
    float volume)
{
    //
    // Choose sound buffer
    //

    sf::SoundBuffer * chosenSoundBuffer = nullptr;

    assert(!multipleSoundChoiceInfo.SoundBuffers.empty());
    if (1 == multipleSoundChoiceInfo.SoundBuffers.size())
    {
        // Nothing to choose
        chosenSoundBuffer = multipleSoundChoiceInfo.SoundBuffers[0].get();
    }
    else
    {
        assert(multipleSoundChoiceInfo.SoundBuffers.size() >= 2);

        // Choose randomly, but avoid choosing the last-chosen sound again
        size_t chosenSoundIndex = GameRandomEngine::GetInstance().ChooseNew(
            multipleSoundChoiceInfo.SoundBuffers.size(),
            multipleSoundChoiceInfo.LastPlayedSoundIndex);

        chosenSoundBuffer = multipleSoundChoiceInfo.SoundBuffers[chosenSoundIndex].get();

        multipleSoundChoiceInfo.LastPlayedSoundIndex = chosenSoundIndex;
    }

    assert(nullptr != chosenSoundBuffer);

    PlaySound(
        soundType,
        chosenSoundBuffer,        
        volume);
}

void SoundController::PlaySound(
    SoundType soundType,
    sf::SoundBuffer * soundBuffer,    
    float volume)
{
    assert(nullptr != soundBuffer);

    //
    // Make sure there isn't already a sound with this sound buffer that started
    // playing too recently;
    // if there is, adjust its volume
    //

    auto const now = std::chrono::steady_clock::now();

    for (auto const & currentlyPlayingSound : mCurrentlyPlayingSounds)
    {
        assert(!!currentlyPlayingSound.Sound);
        if (currentlyPlayingSound.Sound->getBuffer() == soundBuffer
            && std::chrono::duration_cast<std::chrono::milliseconds>(now - currentlyPlayingSound.StartedTimestamp) < MinDeltaTimeSound)
        {
            currentlyPlayingSound.Sound->setVolume(
                std::max(
                    100.0f,
                    currentlyPlayingSound.Sound->getVolume() + volume));

            return;
        }
    }


    //
    // Make sure there's room for this sound
    //

    if (mCurrentlyPlayingSounds.size() >= MaxPlayingSounds)
    {
        ScavengeStoppedSounds();

        if (mCurrentlyPlayingSounds.size() >= MaxPlayingSounds)
        { 
            // Need to stop sound that's been playing for the longest
            ScavengeOldestSound(soundType);
        }
    }

    assert(mCurrentlyPlayingSounds.size() < MaxPlayingSounds);



    //
    // Create and play sound
    //

    std::unique_ptr<sf::Sound> sound = std::make_unique<sf::Sound>();
    sound->setBuffer(*soundBuffer);

    sound->setVolume(volume);
    sound->play();    

    mCurrentlyPlayingSounds.emplace_back(
        soundType,
        std::move(sound),
        now);
}

void SoundController::ScavengeStoppedSounds()
{
    for (size_t i = 0; i < mCurrentlyPlayingSounds.size(); /*incremented in loop*/)
    {
        assert(!!mCurrentlyPlayingSounds[i].Sound);
        if (sf::Sound::Status::Stopped == mCurrentlyPlayingSounds[i].Sound->getStatus())
        {
            // Scavenge
            mCurrentlyPlayingSounds.erase(mCurrentlyPlayingSounds.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

void SoundController::ScavengeOldestSound(SoundType soundType)
{
    assert(!mCurrentlyPlayingSounds.empty());

    size_t iOldestSound = std::numeric_limits<size_t>::max();
    auto oldestSoundStartTimestamp = std::chrono::steady_clock::time_point::max();
    size_t iOldestSoundForType = std::numeric_limits<size_t>::max();
    auto oldestSoundForTypeStartTimestamp = std::chrono::steady_clock::time_point::max();    
    for (size_t i = 0; i < mCurrentlyPlayingSounds.size(); ++i)
    {
        if (mCurrentlyPlayingSounds[i].StartedTimestamp < oldestSoundStartTimestamp)
        {
            iOldestSound = i;
            oldestSoundStartTimestamp = mCurrentlyPlayingSounds[i].StartedTimestamp;
        }

        if (mCurrentlyPlayingSounds[i].StartedTimestamp < oldestSoundForTypeStartTimestamp
            && mCurrentlyPlayingSounds[i].Type == soundType)
        {
            iOldestSoundForType = i;
            oldestSoundForTypeStartTimestamp = mCurrentlyPlayingSounds[i].StartedTimestamp;
        }
    }

    size_t iStop;
    if (oldestSoundForTypeStartTimestamp != std::chrono::steady_clock::time_point::max())
    {
        iStop = iOldestSoundForType;
    }
    else 
    {
        assert((oldestSoundStartTimestamp != std::chrono::steady_clock::time_point::max()));
        iStop = iOldestSound;
    }

    assert(!!mCurrentlyPlayingSounds[iStop].Sound);
    mCurrentlyPlayingSounds[iStop].Sound->stop();
    mCurrentlyPlayingSounds.erase(mCurrentlyPlayingSounds.begin() + iStop);
}

void SoundController::UpdateContinuousSound(
    size_t count,
    SingleContinuousSound & sound)
{
    if (count == 0)
    {
        // Stop it
        sound.Stop();
    }
    else
    {
        float volume = 100.0f * (1.0f - exp(-0.3f * static_cast<float>(count)));
        sound.SetVolume(volume);
        sound.Start();        
    }
}
