/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameLib/IGameEventHandler.h>
#include <GameLib/ResourceLoader.h>
#include <GameLib/TupleKeys.h>
#include <GameLib/Utils.h>

#include <SFML/Audio.hpp>

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <vector>

class SoundController : public IGameEventHandler
{
public:

    SoundController(
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

	virtual ~SoundController();

    void SetPaused(bool isPaused);

    void SetMute(bool isMute);

    void SetVolume(float volume);

    void HighFrequencyUpdate();

    void LowFrequencyUpdate();

    void Reset();

public:

    //
    // Game event handlers
    //

    virtual void OnDestroy(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSaw(std::optional<bool> isUnderwater) override;

    virtual void OnDraw(std::optional<bool> isUnderwater) override;

    virtual void OnSwirl(std::optional<bool> isUnderwater) override;

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override;

    virtual void OnStress(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnBreak(
        Material const * material,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSinkingBegin(unsigned int shipId) override;

    virtual void OnLightFlicker(
        DurationShortLongType duration,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnBombPlaced(
        ObjectId bombId,
        BombType bombType,
        bool isUnderwater) override;

    virtual void OnBombRemoved(
        ObjectId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTimerBombFuse(
        ObjectId bombId,
        std::optional<bool> isFast) override;

    virtual void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override;

private:

    enum class SoundType
    {
        Break,
        Destroy,
        Draw,
        Saw,
        Swirl,
        PinPoint,
        UnpinPoint,
        Stress,
        LightFlicker,
        BombAttached,
        BombDetached,
        RCBombPing,
        TimerBombSlowFuse,
        TimerBombFastFuse,
        TimerBombDefused,
        Explosion
    };

    static SoundType StrToSoundType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);

        if (lstr == "break")
            return SoundType::Break;
        else if (lstr == "destroy")
            return SoundType::Destroy;
        else if (lstr == "draw")
            return SoundType::Draw;
        else if (lstr == "saw")
            return SoundType::Saw;
        else if (lstr == "swirl")
            return SoundType::Swirl;
        else if (lstr == "pinpoint")
            return SoundType::PinPoint;
        else if (lstr == "unpinpoint")
            return SoundType::UnpinPoint;
        else if (lstr == "stress")
            return SoundType::Stress;
        else if (lstr == "lightflicker")
            return SoundType::LightFlicker;
        else if (lstr == "bombattached")
            return SoundType::BombAttached;
        else if (lstr == "bombdetached")
            return SoundType::BombDetached;
        else if (lstr == "rcbombping")
            return SoundType::RCBombPing;
        else if (lstr == "timerbombslowfuse")
            return SoundType::TimerBombSlowFuse;
        else if (lstr == "timerbombfastfuse")
            return SoundType::TimerBombFastFuse;
        else if (lstr == "timerbombdefused")
            return SoundType::TimerBombDefused;
        else if (lstr == "explosion")
            return SoundType::Explosion;
        else
            throw GameException("Unrecognized SoundType \"" + str + "\"");
    }

    enum class SizeType : int
    {
        Min = 0,

        Small = 0,
        Medium = 1,
        Large = 2,

        Max = 2
    };

    static SizeType StrToSizeType(std::string const & str)
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

private:

    struct MultipleSoundChoiceInfo
    {
        std::vector<std::unique_ptr<sf::SoundBuffer>> SoundBuffers;
        size_t LastPlayedSoundIndex;

        MultipleSoundChoiceInfo()
            : SoundBuffers()
            , LastPlayedSoundIndex(0u)
        {
        }
    };

    struct SingleSoundInfo
    {
        std::unique_ptr<sf::SoundBuffer> SoundBuffer;

        SingleSoundInfo()
            : SoundBuffer()
        {
        }
    };

    struct PlayingSound
    {
        SoundType Type;
        std::unique_ptr<sf::Sound> Sound;
        std::chrono::steady_clock::time_point StartedTimestamp;

        PlayingSound(
            SoundType type,
            std::unique_ptr<sf::Sound> sound,
            std::chrono::steady_clock::time_point startedTimestamp)
            : Type(type)
            , Sound(std::move(sound))
            , StartedTimestamp(startedTimestamp)
        {
        }
    };

    struct SingleContinuousSound
    {
        SingleContinuousSound()
            : mSoundBuffer()
            , mSound()
            , mCurrentPauseState(false)
            , mDesiredPlayingState(false)
        {
        }

        void Initialize(std::unique_ptr<sf::SoundBuffer> soundBuffer)
        {
            assert(!mSoundBuffer && !mSound);

            mSoundBuffer = std::move(soundBuffer);
            mSound = std::make_unique<sf::Sound>();
            mSound->setBuffer(*mSoundBuffer);
            mSound->setLoop(true);
        }

        void SetVolume(float volume)
        {
            if (!!mSound)
            {
                mSound->setVolume(volume);
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

    private:
        std::unique_ptr<sf::SoundBuffer> mSoundBuffer;
        std::unique_ptr<sf::Sound> mSound;

        // True/False if we are paused/not paused
        bool mCurrentPauseState;

        // The play state we want to be after resuming from a pause:
        // - true: we want to play
        // - false: we want to stop
        bool mDesiredPlayingState;
    };

private:

    void PlayMSUSound(
        SoundType soundType,
        Material const * material,
        unsigned int size,
        bool isUnderwater,
        float volume);

    void PlayDslUSound(
        SoundType soundType,
        DurationShortLongType duration,
        bool isUnderwater,
        float volume);

    void PlayUSound(
        SoundType soundType,
        bool isUnderwater,
        float volume);

    void ChooseAndPlaySound(
        SoundType soundType,
        MultipleSoundChoiceInfo & multipleSoundChoiceInfo,
        float volume);

    void PlaySound(        
        SoundType soundType,
        sf::SoundBuffer * soundBuffer,
        float volume);

    void ScavengeStoppedSounds();

    void ScavengeOldestSound(SoundType soundType);    

    void UpdateContinuousSound(
        size_t count, 
        SingleContinuousSound & sound);

private:

    std::shared_ptr<ResourceLoader> mResourceLoader;

    float mCurrentVolume;


    //
    // State
    //

    // Tracking which bombs are emitting which fuse sounds
    std::set<ObjectId> mBombsEmittingSlowFuseSounds;
    std::set<ObjectId> mBombsEmittingFastFuseSounds;


    //
    // One-Shot sounds
    //

    static constexpr size_t MaxPlayingSounds{ 100 };
    static constexpr std::chrono::milliseconds MinDeltaTimeSound{ 100 };

    unordered_tuple_map<
        std::tuple<SoundType, Material::SoundProperties::SoundElementType, SizeType, bool>,
        MultipleSoundChoiceInfo> mMSUSoundBuffers;

    unordered_tuple_map<
        std::tuple<SoundType, DurationShortLongType, bool>,
        SingleSoundInfo> mDslUSoundBuffers;

    unordered_tuple_map<
        std::tuple<SoundType, bool>,
        MultipleSoundChoiceInfo> mUSoundBuffers;

    std::vector<PlayingSound> mCurrentlyPlayingSounds;

    //
    // Continuous sounds
    //

    SingleContinuousSound mSawAbovewaterSound;
    SingleContinuousSound mSawUnderwaterSound;
    SingleContinuousSound mDrawSound;
    SingleContinuousSound mSwirlSound;
    SingleContinuousSound mTimerBombSlowFuseSound;
    SingleContinuousSound mTimerBombFastFuseSound;


    //
    // Music
    //

    sf::Music mSinkingMusic;
};
