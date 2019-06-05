/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>

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
    RepairSpring,
    RepairTriangle,
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
    TsunamiTriggered,
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
    Scrub,
    RepairStructure,
    WaveMaker
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
 * Provides volume control based on a master volume and a local volume,
 * and facilities to fade-in and fade-out.
 *
 * Also provides facilities to pause/resume.
 */
class GameSound : public sf::Sound
{
public:

    GameSound(
        sf::SoundBuffer const & soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : sf::Sound(soundBuffer)
        , mIsPaused(false)
        , mDesiredPlayingStateAfterPause(false)
        , mVolume(volume)
        , mMasterVolume(masterVolume)
        , mIsMuted(isMuted)
        , mFadeLevel(0.0f)
        , mTimeToFadeIn(timeToFadeIn)
        , mTimeToFadeOut(timeToFadeOut)
        , mFadeInStartTimestamp()
        , mFadeOutStartTimestamp()
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

    void play() override
    {
        // Reset fade
        mFadeLevel = 1.0f;
        InternalSetVolume();

        // Reset state
        mFadeInStartTimestamp.reset();
        mFadeOutStartTimestamp.reset();

        // Remember we want to play when we resume
        mDesiredPlayingStateAfterPause = true;

        if (!mIsPaused)
        {
            // Play
            sf::Sound::play();
        }
    }

    void fadeToPlay()
    {
        // Start fade-in now, adjusting start timestamp to match current fade level,
        // so that if we are interrupting a fade-out half-way, the volume doesn't drop
        mFadeInStartTimestamp =
            GameWallClock::GetInstance().Now()
            - std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(mFadeLevel * mTimeToFadeIn.count()));

        // Stop fade-out, if any
        mFadeOutStartTimestamp.reset();

        // Remember we want to play when we resume
        mDesiredPlayingStateAfterPause = true;
    }

    void stop() override
    {
        // Reset state
        mFadeLevel = 0.0f;
        mFadeInStartTimestamp.reset();
        mFadeOutStartTimestamp.reset();

        // Remember we want to stay stopped afte resume
        mDesiredPlayingStateAfterPause = false;

        // Stop
        sf::Sound::stop();
    }

    void pause() override
    {
        if (!mIsPaused)
        {
            mIsPaused = true;

            // Pause
            sf::Sound::pause();
        }
    }

    void resume()
    {
        if (mIsPaused)
        {
            mIsPaused = false;

            // Look at the desired playing state
            if (mDesiredPlayingStateAfterPause
                && sf::Sound::Status::Paused == this->getStatus())
                sf::Sound::play(); // Resume
        }
    }

    void fadeToStop()
    {
        // Start fade-out now, adjusting end timestamp to match current fade level,
        // so that if we are interrupting a fade-in half-way, the volume doesn't explode up
        mFadeOutStartTimestamp =
            GameWallClock::GetInstance().Now()
            - std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>((1.0f - mFadeLevel) * mTimeToFadeOut.count()));

        // Stop fade-in, if any
        mFadeInStartTimestamp.reset();
    }

    void update()
    {
        if (!!mFadeInStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeInStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeIn)
            {
                mFadeLevel = 1.0f;
                mFadeInStartTimestamp.reset();
            }
            else
            {
                // Raise volume towards max
                mFadeLevel = static_cast<float>(elapsedMillis.count()) / static_cast<float>(mTimeToFadeIn.count());
            }

            InternalSetVolume();

            if (!mIsPaused)
            {
                if (sf::Sound::Status::Playing != this->getStatus())
                {
                    sf::Sound::play();
                }
            }
        }
        else if (!!mFadeOutStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeOutStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeOut)
            {
                mFadeLevel = 0.0f;
                mFadeOutStartTimestamp.reset();

                // Remember we want to stay stopped when we're done
                mDesiredPlayingStateAfterPause = false;

                // Stop
                sf::Sound::stop();
            }
            else
            {
                // Lower volume towards zero
                mFadeLevel = 1.0f - static_cast<float>(elapsedMillis.count()) / static_cast<float>(mTimeToFadeOut.count());
                InternalSetVolume();
            }
        }
    }

private:

    void InternalSetVolume()
    {
        if (!mIsMuted)
        {
            // 100*(1 - e^(-0.01*x))
            float localVolume = 1.0f - exp(-0.01f * mVolume);
            sf::Sound::setVolume(100.0f * localVolume * (mMasterVolume / 100.0f) * mFadeLevel);
        }
        else
        {
            sf::Sound::setVolume(0.0f);
        }
    }

private:

    bool mIsPaused;

    // The play state we want to be in after resuming from a pause:
    // - true: we want to play
    // - false: we want to stop
    bool mDesiredPlayingStateAfterPause;

    float mVolume;
    float mMasterVolume;
    bool mIsMuted;
    float mFadeLevel;

    std::chrono::milliseconds const mTimeToFadeIn;
    std::chrono::milliseconds const mTimeToFadeOut;
    std::optional<GameWallClock::time_point> mFadeInStartTimestamp;
    std::optional<GameWallClock::time_point> mFadeOutStartTimestamp;
};

/*
 * Our wrapper for sf::Music.
 *
 * Provides volume control based on a master volume and a local volume,
 * and facilities to fade-in and fade-out.
 */
class GameMusic : public sf::Music
{
public:

    GameMusic(
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : mVolume(volume)
        , mMasterVolume(masterVolume)
        , mFadeLevel(1.0f)
        , mIsMuted(isMuted)
        , mTimeToFadeIn(timeToFadeIn)
        , mTimeToFadeOut(timeToFadeOut)
        , mFadeInStartTimestamp()
        , mFadeOutStartTimestamp()
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

    void play() override
    {
        // Reset fade
        mFadeLevel = 1.0f;
        InternalSetVolume();

        // Play
        sf::Music::play();

        // Reset state
        mFadeInStartTimestamp.reset();
        mFadeOutStartTimestamp.reset();
    }

    void fadeToPlay()
    {
        mFadeInStartTimestamp = GameWallClock::GetInstance().Now();
    }

    void stop() override
    {
        // Stop
        sf::Music::stop();

        // Reset state
        mFadeInStartTimestamp.reset();
        mFadeOutStartTimestamp.reset();
    }

    void fadeToStop()
    {
        mFadeInStartTimestamp.reset();
        mFadeOutStartTimestamp = GameWallClock::GetInstance().Now();
    }

    void update()
    {
        if (!!mFadeInStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeInStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeIn)
            {
                mFadeLevel = 1.0f;
                mFadeInStartTimestamp.reset();
            }
            else
            {
                // Raise volume
                mFadeLevel = static_cast<float>(elapsedMillis.count()) / static_cast<float>(mTimeToFadeOut.count());
            }

            InternalSetVolume();

            if (sf::Sound::Status::Playing != this->getStatus())
            {
                this->play();
            }
        }
        else if (!!mFadeOutStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeOutStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeOut)
            {
                this->stop();
                mFadeOutStartTimestamp.reset();
            }
            else
            {
                // Lower volume
                mFadeLevel = (1.0f - static_cast<float>(elapsedMillis.count()) / static_cast<float>(mTimeToFadeOut.count()));
                InternalSetVolume();
            }
        }
    }

private:

    void InternalSetVolume()
    {
        if (!mIsMuted)
            sf::Music::setVolume(100.0f * (mVolume / 100.0f) * (mMasterVolume / 100.0f) * mFadeLevel);
        else
            sf::Music::setVolume(0.0f);
    }

    float mVolume;
    float mMasterVolume;
    float mFadeLevel;
    bool mIsMuted;

    std::chrono::milliseconds const mTimeToFadeIn;
    std::chrono::milliseconds const mTimeToFadeOut;
    std::optional<GameWallClock::time_point> mFadeInStartTimestamp;
    std::optional<GameWallClock::time_point> mFadeOutStartTimestamp;
};


/*
 * A sound that plays continuously, until stopped.
 *
 * Remembers playing state across pauses, supports fade-in and fade-out, and
 * is capable of adjusting its volume based on "number of triggers".
 */
struct ContinuousSound
{
    ContinuousSound()
        : mSoundBuffer()
        , mSound()
    {
    }

    ContinuousSound(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : ContinuousSound()
    {
        Initialize(
            std::move(soundBuffer),
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut);
    }

    void Initialize(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
    {
        assert(!mSoundBuffer && !mSound);

        mSoundBuffer = std::move(soundBuffer);
        mSound = std::make_unique<GameSound>(
            *mSoundBuffer,
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut);
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

    void UpdateAggregateVolume(size_t count)
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

    void SetMuted(bool isMuted)
    {
        if (!!mSound)
        {
            mSound->setMuted(isMuted);
        }
    }

    enum class StartMode
    {
        Immediate,
        WithFadeIn
    };

    void Start(StartMode startMode = StartMode::Immediate)
    {
        if (!!mSound)
        {
            if (StartMode::WithFadeIn == startMode)
            {
                mSound->fadeToPlay();
            }
            else
            {
                if (sf::Sound::Status::Playing != mSound->getStatus())
                {
                    mSound->play();
                }
            }
        }
    }

    void SetPaused(bool isPaused)
    {
        if (!!mSound)
        {
            if (isPaused)
            {
                // Pausing
                mSound->pause();
            }
            else
            {
                // Resuming
                mSound->resume();
            }
        }
    }

    enum class StopMode
    {
        Immediate,
        WithFadeOut
    };

    void Stop(StopMode stopMode = StopMode::Immediate)
    {
        if (!!mSound)
        {
            // We stop regardless of the pause state, even if we're paused
            if (sf::Sound::Status::Stopped != mSound->getStatus())
            {
                if (StopMode::WithFadeOut == stopMode)
                    mSound->fadeToStop();
                else
                    mSound->stop();
            }
        }
    }

    void Update()
    {
        mSound->update();
    }

private:
    std::unique_ptr<sf::SoundBuffer> mSoundBuffer;
    std::unique_ptr<GameSound> mSound;
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

protected:

    std::vector<ContinuousSound> mSoundAlternatives;
    std::vector<size_t> mSoundAlternativePlayCounts;
    size_t mLastChosenAlternative;
};

template<typename TObjectId>
struct ContinuousMultipleChoiceAggregateSound : public ContinuousMultipleChoiceSound
{
public:

    ContinuousMultipleChoiceAggregateSound()
        : ContinuousMultipleChoiceSound()
        , mAlternativesByObject()
    {}

    void Reset()
    {
        ContinuousMultipleChoiceSound::Reset();

        mAlternativesByObject.clear();
    }

    void StartSoundAlternativeForObject(TObjectId objectId)
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
        mSoundAlternatives[mLastChosenAlternative].UpdateAggregateVolume(mSoundAlternativePlayCounts[mLastChosenAlternative]);
    }

    void StopSoundAlternativeForObject(TObjectId objectId)
    {
        // Get alternative we had for this object
        assert(mAlternativesByObject.count(objectId) == 1);
        size_t alternative = mAlternativesByObject[objectId];

        // Update number ofobjects that are playing this alternative
        --mSoundAlternativePlayCounts[alternative];

        // Remove object<->alternative mapping
        mAlternativesByObject.erase(objectId);

        // Update continuous sound
        mSoundAlternatives[alternative].UpdateAggregateVolume(mSoundAlternativePlayCounts[alternative]);
    }

private:

    std::unordered_map<TObjectId, size_t> mAlternativesByObject;
};

struct ContinuousSingleChoiceSound
{
    ContinuousSingleChoiceSound()
        : mSound()
    {}

    void Initialize(
        std::unique_ptr<sf::SoundBuffer> soundBuffer,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
    {
        mSound.Initialize(
            std::move(soundBuffer),
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut);
    }

    void Reset()
    {
        mSound.Stop();
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

    void FadeIn()
    {
        mSound.Start(ContinuousSound::StartMode::WithFadeIn);
    }

    void SetPaused(bool isPaused)
    {
        mSound.SetPaused(isPaused);
    }

    void Stop()
    {
        mSound.Stop();
    }

    void FadeOut()
    {
        mSound.Stop(ContinuousSound::StopMode::WithFadeOut);
    }

    void Update()
    {
        mSound.Update();
    }

protected:

    ContinuousSound mSound;
};

template<typename TObjectId>
struct ContinuousSingleChoiceAggregateSound : public ContinuousSingleChoiceSound
{
    ContinuousSingleChoiceAggregateSound()
        : ContinuousSingleChoiceSound()
        , mObjectsPlayingSound()
    {}

    void Reset()
    {
        ContinuousSingleChoiceSound::Reset();
        mObjectsPlayingSound.clear();
    }

    void StartSoundForObject(TObjectId objectId)
    {
        // Remember that this object is playing this sound
        assert(mObjectsPlayingSound.count(objectId) == 0);
        mObjectsPlayingSound.insert(objectId);

        // Update continuous sound
        mSound.UpdateAggregateVolume(mObjectsPlayingSound.size());
    }

    bool StopSoundForObject(TObjectId objectId)
    {
        // Remove the object tracking, if any
        bool objectWasPlayingSound = (mObjectsPlayingSound.erase(objectId) != 0);

        if (objectWasPlayingSound)
        {
            // Update continuous sound
            mSound.UpdateAggregateVolume(mObjectsPlayingSound.size());
        }

        return objectWasPlayingSound;
    }

private:

    std::set<TObjectId> mObjectsPlayingSound;
};
