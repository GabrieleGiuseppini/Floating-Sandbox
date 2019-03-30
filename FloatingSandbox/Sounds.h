/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>

#include <SFML/Audio.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

enum class SoundType
{
    Break,
    Destroy,
    Draw,
    Saw,
    Sawed,
    Swirl,
    PinPoint,
    UnpinPoint,
    AirBubbles,
    FloodHose,
    Stress,
    LightFlicker,
    WaterRush,
    WaterSplash,
    Wave,
    Wind,
    WindGust,
    BombAttached,
    BombDetached,
    BombExplosion,
    RCBombPing,
    TimerBombSlowFuse,
    TimerBombFastFuse,
    TimerBombDefused,
    AntiMatterBombContained,
    AntiMatterBombPreImplosion,
    AntiMatterBombImplosion,
    AntiMatterBombExplosion,
    Snapshot,
    TerrainAdjust,
    Scrub
};

SoundType StrToSoundType(std::string const & str);

enum class SizeType : int
{
    Min = 0,

    Small = 0,
    Medium = 1,
    Large = 2,

    Max = 2
};

SizeType StrToSizeType(std::string const & str);


/*
 * Our wrapper for sf::Sound.
 *
 * Provides volume control based on a master volume and a local volume.
 */
class GameSound : public sf::Sound
{
public:

    GameSound(
        sf::SoundBuffer const & soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted)
        : sf::Sound(soundBuffer)
        , mVolume(volume)
        , mMasterVolume(masterVolume)
        , mIsMuted(isMuted)
    {
        InternalSetVolume();
    }

    void setVolume(float volume)
    {
        if (volume != mVolume)
        {
            mVolume = volume;
            InternalSetVolume();
        }
    }

    void addVolume(float volume)
    {
        mVolume += volume;
        InternalSetVolume();
    }

    void setMasterVolume(float masterVolume)
    {
        mMasterVolume = masterVolume;
        InternalSetVolume();
    }

    void setMuted(bool isMuted)
    {
        mIsMuted = isMuted;
        InternalSetVolume();
    }

    void setVolumes(
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mVolume = volume;
        mMasterVolume = masterVolume;
        mIsMuted = isMuted;
        InternalSetVolume();
    }

private:

    void InternalSetVolume()
    {
        if (!mIsMuted)
        {
            // 100*(1 - e^(-0.01*x))
            float localVolume = 1.0f - exp(-0.01f * mVolume);
            sf::Sound::setVolume(100.0f * localVolume * (mMasterVolume / 100.0f));
        }
        else
        {
            sf::Sound::setVolume(0.0f);
        }
    }

    float mVolume;
    float mMasterVolume;
    bool mIsMuted;
};

/*
 * Our wrapper for sf::Music.
 *
 * Provides volume control based on a master volume and a local volume.
 */
class GameMusic : public sf::Music
{
public:

    GameMusic(
        float volume,
        float masterVolume,
        bool isMuted)
        : mVolume(volume)
        , mMasterVolume(masterVolume)
        , mIsMuted(isMuted)
    {
        InternalSetVolume();
    }

    void setVolume(float volume)
    {
        mVolume = volume;
        InternalSetVolume();
    }

    void setMasterVolume(float masterVolume)
    {
        mMasterVolume = masterVolume;
        InternalSetVolume();
    }

    void setMuted(bool isMuted)
    {
        mIsMuted = isMuted;
        InternalSetVolume();
    }

    void setVolumes(
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mVolume = volume;
        mMasterVolume = masterVolume;
        mIsMuted = isMuted;
        InternalSetVolume();
    }

private:

    void InternalSetVolume()
    {
        if (!mIsMuted)
            sf::Music::setVolume(100.0f * (mVolume / 100.0f) * (mMasterVolume / 100.0f));
        else
            sf::Music::setVolume(0.0f);
    }

    float mVolume;
    float mMasterVolume;
    bool mIsMuted;
};


/*
 * A sound that plays continuously, until stopped.
 *
 * Remembers playing state across pauses, and is capable of adjusting its volume
 * based on "number of triggers".
 */
struct ContinuousSound
{
    ContinuousSound()
        : mSoundBuffer()
        , mSound()
        , mCurrentPauseState(false)
        , mDesiredPlayingState(false)
    {
    }

    explicit ContinuousSound(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted)
        : ContinuousSound()
    {
        Initialize(
            std::move(soundBuffer),
            volume,
            masterVolume,
            isMuted);
    }

    void Initialize(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted)
    {
        assert(!mSoundBuffer && !mSound);

        mSoundBuffer = std::move(soundBuffer);
        mSound = std::make_unique<GameSound>(
            *mSoundBuffer,
            volume,
            masterVolume,
            isMuted);
        mSound->setLoop(true);
    }

    void SetVolume(float volume)
    {
        if (!!mSound)
        {
            mSound->setVolume(volume);
        }
    }

    void SetMasterVolume(float masterVolume)
    {
        if (!!mSound)
        {
            mSound->setMasterVolume(masterVolume);
        }
    }

    void SetMuted(bool isMuted)
    {
        if (!!mSound)
        {
            mSound->setMuted(isMuted);
        }
    }

    void Start()
    {
        if (!!mSound)
        {
            if (!mCurrentPauseState
                && sf::Sound::Status::Playing != mSound->getStatus())
            {
                mSound->play();
            }
        }

        // Remember we want to play when we resume
        mDesiredPlayingState = true;
    }

    void SetPaused(bool isPaused)
    {
        if (!!mSound)
        {
            if (isPaused)
            {
                // Pausing
                if (sf::Sound::Status::Playing == mSound->getStatus())
                    mSound->pause();
            }
            else
            {
                // Resuming - look at the desired playing state
                if (mDesiredPlayingState)
                    mSound->play();
            }
        }

        mCurrentPauseState = isPaused;
    }

    void Stop()
    {
        if (!!mSound)
        {
            // We stop regardless of the pause state, even if we're paused
            if (sf::Sound::Status::Stopped != mSound->getStatus())
                mSound->stop();
        }

        // Remember we want to stay stopped when we resume
        mDesiredPlayingState = false;
    }

    void AggregateUpdate(size_t count)
    {
        if (count == 0)
        {
            // Stop it
            Stop();
        }
        else
        {
            float volume = 100.0f * (1.0f - exp(-0.3f * static_cast<float>(count)));
            SetVolume(volume);
            Start();
        }
    }

private:
    std::unique_ptr<sf::SoundBuffer> mSoundBuffer;
    std::unique_ptr<GameSound> mSound;

    // True/False if we are paused/not paused
    bool mCurrentPauseState;

    // The play state we want to be after resuming from a pause:
    // - true: we want to play
    // - false: we want to stop
    bool mDesiredPlayingState;
};

/*
 * A simple continuously-playing sound.
 *
 * The sound may only be shut up after at least a certain time has elapsed since the sound was last heard.
 */
struct ContinuousInertialSound
{
    ContinuousInertialSound(std::chrono::milliseconds inertiaDuration)
        : mContinuousSound()
        , mInertiaDuration(inertiaDuration)
        , mHearableLastTime()
    {
    }

    explicit ContinuousInertialSound(
        std::chrono::milliseconds inertiaDuration,
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float masterVolume,
        bool isMuted)
        : mContinuousSound()
        , mInertiaDuration(inertiaDuration)
        , mHearableLastTime()
    {
        Initialize(
            std::move(soundBuffer),
            masterVolume,
            isMuted);
    }

    void Initialize(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float masterVolume,
        bool isMuted)
    {
        mContinuousSound.Initialize(
            std::move(soundBuffer),
            0.0f,
            masterVolume,
            isMuted);

        mHearableLastTime.reset();
    }

    void Reset()
    {
        Stop();
    }

    void SetVolume(float volume)
    {
        auto now = GameWallClock::GetInstance().Now();

        if (volume > 0.0f)
        {
            mContinuousSound.SetVolume(volume);

            // Remember the last time at which we heard this sound
            mHearableLastTime = now;

            return;
        }
        else if (!mHearableLastTime
            || now - *mHearableLastTime < mInertiaDuration)
        {
            // Been playing for too little - Nothing to do
            return;
        }
        else
        {
            // We can mute it now
            mContinuousSound.SetVolume(0.0f);
            mHearableLastTime.reset();
        }
    }

    void SetMasterVolume(float masterVolume)
    {
        mContinuousSound.SetMasterVolume(masterVolume);
    }

    void SetMuted(bool isMuted)
    {
        mContinuousSound.SetMuted(isMuted);
    }

    void Start()
    {
        mContinuousSound.SetVolume(0.0f);
        mContinuousSound.Start();

        mHearableLastTime.reset();
    }

    void SetPaused(bool isPaused)
    {
        mContinuousSound.SetPaused(isPaused);
    }

    void Stop()
    {
        mContinuousSound.Stop();
        mContinuousSound.SetVolume(0.0f);

        mHearableLastTime.reset();
    }

private:

    ContinuousSound mContinuousSound;

    std::chrono::milliseconds const mInertiaDuration;
    std::optional<GameWallClock::time_point> mHearableLastTime;
};

struct OneShotMultipleChoiceSound
{
    std::vector<std::unique_ptr<sf::SoundBuffer>> SoundBuffers;
    size_t LastPlayedSoundIndex;

    OneShotMultipleChoiceSound()
        : SoundBuffers()
        , LastPlayedSoundIndex(0u)
    {
    }
};

struct OneShotSingleChoiceSound
{
    std::unique_ptr<sf::SoundBuffer> SoundBuffer;

    OneShotSingleChoiceSound()
        : SoundBuffer()
    {
    }
};

struct ContinuousMultipleChoiceSound
{
public:

    ContinuousMultipleChoiceSound()
        : mSoundAlternatives()
        , mSoundAlternativePlayCounts()
        , mAlternativesByObject()
        , mLastChosenAlternative(std::numeric_limits<size_t>::max())
    {}

    void AddAlternative(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mSoundAlternatives.emplace_back(
            std::move(soundBuffer),
            volume,
            masterVolume,
            isMuted);

        mSoundAlternativePlayCounts.emplace_back(0);
    }

    void Reset()
    {
        Stop();

        std::fill(
            mSoundAlternativePlayCounts.begin(),
            mSoundAlternativePlayCounts.end(),
            0);

        mAlternativesByObject.clear();
    }

    void StartSoundAlternativeForObject(ObjectId objectId)
    {
        // Choose alternative
        mLastChosenAlternative = GameRandomEngine::GetInstance().ChooseNew(
            mSoundAlternatives.size(),
            mLastChosenAlternative);

        // Remember how many objects are playing this alternative
        ++mSoundAlternativePlayCounts[mLastChosenAlternative];

        // Remember object<->alternative mapping
        assert(mAlternativesByObject.count(objectId) == 0);
        mAlternativesByObject[objectId] = mLastChosenAlternative;

        // Update continuous sound
        mSoundAlternatives[mLastChosenAlternative].AggregateUpdate(mSoundAlternativePlayCounts[mLastChosenAlternative]);
    }

    void StopSoundAlternativeForObject(ObjectId objectId)
    {
        // Get alternative we had for this object
        assert(mAlternativesByObject.count(objectId) == 1);
        size_t alternative = mAlternativesByObject[objectId];

        // Update number ofobjects that are playing this alternative
        --mSoundAlternativePlayCounts[alternative];

        // Remove object<->alternative mapping
        mAlternativesByObject.erase(objectId);

        // Update continuous sound
        mSoundAlternatives[alternative].AggregateUpdate(mSoundAlternativePlayCounts[alternative]);
    }

    void SetVolume(float volume)
    {
        std::for_each(
            mSoundAlternatives.begin(),
            mSoundAlternatives.end(),
            [&volume](auto & s)
            {
                s.SetVolume(volume);
            });
    }

    void SetMasterVolume(float masterVolume)
    {
        std::for_each(
            mSoundAlternatives.begin(),
            mSoundAlternatives.end(),
            [&masterVolume](auto & s)
            {
                s.SetMasterVolume(masterVolume);
            });
    }

    void SetMuted(bool isMuted)
    {
        std::for_each(
            mSoundAlternatives.begin(),
            mSoundAlternatives.end(),
            [&isMuted](auto & s)
            {
                s.SetMuted(isMuted);
            });
    }

    void SetPaused(bool isPaused)
    {
        std::for_each(
            mSoundAlternatives.begin(),
            mSoundAlternatives.end(),
            [&isPaused](auto & s)
            {
                s.SetPaused(isPaused);
            });
    }

    void Stop()
    {
        std::for_each(
            mSoundAlternatives.begin(),
            mSoundAlternatives.end(),
            [](auto & s)
            {
                s.Stop();
            });
    }

private:

    std::vector<ContinuousSound> mSoundAlternatives;
    std::vector<size_t> mSoundAlternativePlayCounts;
    std::unordered_map<ObjectId, size_t> mAlternativesByObject;
    size_t mLastChosenAlternative;
};

struct ContinuousSingleChoiceSound
{
    ContinuousSingleChoiceSound()
        : mSound()
        , mObjectsPlayingSound()
    {}

    void Initialize(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mSound.Initialize(
            std::move(soundBuffer),
            volume,
            masterVolume,
            isMuted);
    }

    void Reset()
    {
        mSound.Stop();
        mObjectsPlayingSound.clear();
    }

    void StartSoundForObject(ObjectId objectId)
    {
        // Remember that this object is playing this sound
        assert(mObjectsPlayingSound.count(objectId) == 0);
        mObjectsPlayingSound.insert(objectId);

        // Update continuous sound
        mSound.AggregateUpdate(mObjectsPlayingSound.size());
    }

    bool StopSoundForObject(ObjectId objectId)
    {
        // Remove the object tracking, if any
        bool objectWasPlayingSound = (mObjectsPlayingSound.erase(objectId) != 0);

        if (objectWasPlayingSound)
        {
            // Update continuous sound
            mSound.AggregateUpdate(mObjectsPlayingSound.size());
        }

        return objectWasPlayingSound;
    }

    void SetVolume(float volume)
    {
        mSound.SetVolume(volume);
    }

    void SetMasterVolume(float volume)
    {
        mSound.SetMasterVolume(volume);
    }

    void SetMuted(bool muted)
    {
        mSound.SetMuted(muted);
    }

    void Start()
    {
        mSound.Start();
    }

    void SetPaused(bool isPaused)
    {
        mSound.SetPaused(isPaused);
    }

    void Stop()
    {
        mSound.Stop();
    }

private:

    ContinuousSound mSound;
    std::set<ObjectId> mObjectsPlayingSound;
};

