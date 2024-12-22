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
#include <GameCore/TupleKeys.h>

#include <SFML/Audio.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

enum class SoundType : uint32_t
{
    Break = 0,
    Destroy,
    Impact,
    LightningHit,
    RepairSpring,
    RepairTriangle,
    Draw,
    Saw,
    Sawed,
    LaserCut,
    HeatBlasterCool,
    HeatBlasterHeat,
    ElectricSpark,
    FireExtinguisher,
    Swirl,
    PinPoint,
    UnpinPoint,
    AirBubbles,
    FloodHose,
    Stress,
    LightFlicker,
    InteractiveSwitchOn,
    InteractiveSwitchOff,
    ElectricalPanelOpen,
    ElectricalPanelClose,
    ElectricalPanelDock,
    ElectricalPanelUndock,
    GlassTick,
    EngineDiesel1,
    EngineJet1,
    EngineOutboard1,
    EngineSteam1,
    EngineSteam2,
    EngineTelegraph,
    EngineThrottleIdle,
    WaterPump,
    WatertightDoorClosed,
    WatertightDoorOpened,
    ShipBell1,
    ShipBell2,
    ShipQueenMaryHorn,
    ShipFourFunnelLinerWhistle,
    ShipTripodHorn,
    ShipPipeWhistle,
    ShipLakeFreighterHorn,
    ShipShieldhallSteamSiren,
    ShipQueenElizabeth2Horn,
    ShipSSRexWhistle,
    ShipKlaxon1,
    ShipNuclearAlarm1,
    ShipEvacuationAlarm1,
    ShipEvacuationAlarm2,
    WaterRush,
    WaterSplash,
    WaterDisplacementSplash,
    WaterDisplacementWave,
    AirBubblesSurface,
    Wave,
    Wind,
    WindGust,
    WindGustShort,
    Rain,
    Thunder,
    Lightning,
    FireBurning,
    FireSizzling,
    CombustionExplosion,
    WaterReactionTriggered,
    WaterReactionExplosion,
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
    PhysicsProbeAttached,
    PhysicsProbeDetached,
    PhysicsProbePanelOpen,
    PhysicsProbePanelClose,
    Pliers,
    Snapshot,
    TerrainAdjust,
    Scrub,
    Rot,
    RepairStructure,
    ThanosSnap,
    WaveMaker,
    FishScream,
    FishShaker,
    BlastToolSlow1,
    BlastToolSlow2,
    BlastToolFast,
    PressureInjection,
    LaserRayNormal,
    LaserRayAmplified,
    LampExplosion,
    LampImplosion,

    // UI
    Error
};

SoundType StrToSoundType(std::string const & str);

struct SoundFile
{
    sf::SoundBuffer SoundBuffer;
    std::string Filename;

    static std::unique_ptr<SoundFile> Load(std::filesystem::path const & soundFilePath);

    std::unique_ptr<SoundFile> Clone() const
    {
        return std::unique_ptr<SoundFile>(
            new SoundFile(
                sf::SoundBuffer(SoundBuffer),
                Filename));
    }

private:

    SoundFile(
        sf::SoundBuffer && soundBuffer,
        std::string const & filename)
        : SoundBuffer(std::move(soundBuffer))
        , Filename(filename)
    {}

    SoundFile(SoundFile const & other)
    {
        SoundBuffer = other.SoundBuffer;
        Filename = other.Filename;
    }
};

enum class SizeType : int
{
    Min = 0,

    Small = 0,
    Medium = 1,
    Large = 2,

    Max = 2
};

SizeType StrToSizeType(std::string const & str);

enum class SoundStartMode
{
    Immediate,
    WithFadeIn
};

enum class SoundStopMode
{
    Immediate,
    WithFadeOut
};

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
        SoundFile const & soundFile,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : sf::Sound(soundFile.SoundBuffer)
        , mSoundFile(soundFile)
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
        mVolume += (100.0f - mVolume) / 100.0f * volume;
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

    void setPitch(float pitch)
    {
        sf::Sound::setPitch(pitch);
    }

    void play() override
    {
        //LogMessage("Sound::play(", mSoundFile.Filename, ")");

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
        if (!mFadeInStartTimestamp)
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
        if (!mFadeOutStartTimestamp)
        {
            // Start fade-out now, adjusting end timestamp to match current fade level,
            // so that if we are interrupting a fade-in half-way, the volume doesn't explode up
            mFadeOutStartTimestamp =
                GameWallClock::GetInstance().Now()
                - std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>((1.0f - mFadeLevel) * mTimeToFadeOut.count()));

            // Stop fade-in, if any
            mFadeInStartTimestamp.reset();
        }
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
            sf::Sound::setVolume(mVolume * (mMasterVolume / 100.0f) * mFadeLevel);
        }
        else
        {
            sf::Sound::setVolume(0.0f);
        }
    }

private:

    SoundFile const & mSoundFile;

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
 * A sound that plays continuously, until stopped.
 *
 * Remembers playing state across pauses, supports fade-in and fade-out, and
 * is capable of adjusting its volume based on "number of triggers".
 */
struct ContinuousSound
{
    ContinuousSound()
        : mSoundFile()
        , mSound()
    {
    }

    ContinuousSound(
        std::unique_ptr<SoundFile> soundFile,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : ContinuousSound()
    {
        Initialize(
            std::move(soundFile),
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut);
    }

    void Initialize(
        std::unique_ptr<SoundFile> soundFile,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero(),
        float aggregateRate = 0.3f)
    {
        assert(!mSoundFile && !mSound);

        mSoundFile = std::move(soundFile);
        mSound = std::make_unique<GameSound>(
            *mSoundFile,
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut);
        mSound->setLoop(true);

        mAggregateRate = aggregateRate;
    }

    void SetVolume(float volume)
    {
        if (!!mSound)
        {
            mSound->setVolume(volume);
            if (volume > 0.0f)
            {
                Start();
            }
            else
            {
                // Conserve resources
                Stop();
            }
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
            float volume = 100.0f * (1.0f - exp(-mAggregateRate * static_cast<float>(count)));
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

    void SetPitch(float pitch)
    {
        if (!!mSound)
        {
            mSound->setPitch(pitch);
        }
    }

    void Start(SoundStartMode startMode = SoundStartMode::Immediate)
    {
        if (!!mSound)
        {
            if (SoundStartMode::WithFadeIn == startMode)
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

    void Stop(SoundStopMode stopMode = SoundStopMode::Immediate)
    {
        if (!!mSound)
        {
            // We stop regardless of the pause state, even if we're paused
            if (sf::Sound::Status::Stopped != mSound->getStatus())
            {
                if (SoundStopMode::WithFadeOut == stopMode)
                    mSound->fadeToStop();
                else
                    mSound->stop();
            }
        }
    }

    void UpdateSimulation()
    {
        mSound->update();
    }

private:
    std::unique_ptr<SoundFile> mSoundFile;
    std::unique_ptr<GameSound> mSound;

    float mAggregateRate;
};

/*
 * A simple continuously-playing sound.
 *
 * The sound may only be shut up after at least a certain time has elapsed since the sound was last heard.
 */
struct ContinuousInertialSound
{
    explicit ContinuousInertialSound(std::chrono::milliseconds inertiaDuration)
        : mContinuousSound()
        , mInertiaDuration(inertiaDuration)
        , mHearableLastTime()
    {
    }

    ContinuousInertialSound(
        std::chrono::milliseconds inertiaDuration,
        std::unique_ptr<SoundFile> soundFile,
        float masterVolume,
        bool isMuted)
        : mContinuousSound()
        , mInertiaDuration(inertiaDuration)
        , mHearableLastTime()
    {
        Initialize(
            std::move(soundFile),
            masterVolume,
            isMuted);
    }

    void Initialize(
        std::unique_ptr<SoundFile> soundFile,
        float masterVolume,
        bool isMuted)
    {
        mContinuousSound.Initialize(
            std::move(soundFile),
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

struct ContinuousPulsedSound
{
    ContinuousPulsedSound(
        float riseCoefficient,
        float decayCoefficient)
        : mRiseCoefficient(riseCoefficient)
        , mDecayCoefficient(decayCoefficient)
        , mContinuousSound()
        , mCurrentVolumeLevel(0.0f)
        , mTargetVolumeLevel(0.0f)
    {
    }

    void Initialize(
        std::unique_ptr<SoundFile> soundFile,
        float masterVolume,
        bool isMuted)
    {
        mContinuousSound.Initialize(
            std::move(soundFile),
            0.0f,
            masterVolume,
            isMuted);

        Reset();
    }

    void Reset()
    {
        Stop();
    }

    void SetMasterVolume(float masterVolume)
    {
        mContinuousSound.SetMasterVolume(masterVolume);
    }

    void SetMuted(bool isMuted)
    {
        mContinuousSound.SetMuted(isMuted);
    }

    void SetPaused(bool isPaused)
    {
        mContinuousSound.SetPaused(isPaused); // Does not pause state machine
    }

    void Stop()
    {
        mContinuousSound.Stop();

        mCurrentVolumeLevel = 0.0f;
        mTargetVolumeLevel = 0.0f;
    }

    void Pulse(float newLevel)
    {
        mTargetVolumeLevel = newLevel;
    }

    void UpdateSimulation()
    {
        if (mTargetVolumeLevel > mCurrentVolumeLevel)
        {
            //
            // Raise
            //

            mCurrentVolumeLevel += (mTargetVolumeLevel - mCurrentVolumeLevel) * mRiseCoefficient;
            mContinuousSound.SetVolume(mCurrentVolumeLevel);

            // See if needs to be started
            if (mCurrentVolumeLevel > 0.0f)
                mContinuousSound.Start();
        }
        else if (mCurrentVolumeLevel > mTargetVolumeLevel)
        {
            //
            // Decay
            //

            mCurrentVolumeLevel += (mTargetVolumeLevel - mCurrentVolumeLevel) * mDecayCoefficient;
            mContinuousSound.SetVolume(mCurrentVolumeLevel);

            // See if needs to be stopped
            if (mCurrentVolumeLevel < 0.01f)
            {
                mCurrentVolumeLevel = 0.0f;
                mContinuousSound.Stop();
            }
        }

        // Zero out target level for next iteration
        mTargetVolumeLevel = 0.0f;
    }

private:

    float const mRiseCoefficient;
    float const mDecayCoefficient;

    ContinuousSound mContinuousSound;

    // State
    float mCurrentVolumeLevel;
    float mTargetVolumeLevel;
};

/*
 * A collection of multiple instances of the same sound, which plays continuously until stopped.
 *
 * Remembers playing state across pauses, and supports fade-in and fade-out.
 */
/*
template<typename TInstanceId>
struct MultiInstanceContinuousSounds
{
    MultiInstanceContinuousSounds(
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : mVolume(volume)
        , mMasterVolume(masterVolume)
        , mIsMuted(isMuted)
        , mTimeToFadeIn(timeToFadeIn)
        , mTimeToFadeOut(timeToFadeOut)
        , mIsPaused(false)
        //
        , mSoundTypeToSoundBuffer()
        , mInstanceIdToSoundType()
        , mSounds()
    {
    }

    void AddSoundType(SoundType soundType, std::unique_ptr<sf::SoundBuffer> soundBuffer)
    {
        assert(0 == mSoundTypeToSoundBuffer.count(soundType));

        mSoundTypeToSoundBuffer.emplace(soundType, std::move(soundBuffer));
    }

    //
    // Must be called first if we want to use the Start(.) overload that does not take a sound type.
    //
    void AddSoundTypeForInstanceId(TInstanceId instanceId, SoundType soundType)
    {
        assert(0 == mInstanceIdToSoundType.count(instanceId));

        mInstanceIdToSoundType.emplace(instanceId, soundType);
    }

    SoundType GetSoundTypeForInstanceId(TInstanceId instanceId) const
    {
        auto const it = mInstanceIdToSoundType.find(instanceId);
        assert(it != mInstanceIdToSoundType.cend());
        return it->second;
    }

    void SetMasterVolume(float masterVolume)
    {
        mMasterVolume = masterVolume;

        for (auto & p : mSounds)
        {
            p.second->setMasterVolume(masterVolume);
        }
    }

    void SetMuted(bool isMuted)
    {
        mIsMuted = isMuted;

        for (auto & p : mSounds)
        {
            p.second->setMuted(isMuted);
        }
    }

    void SetPitch(
        TInstanceId instanceId,
        float pitch)
    {
        auto it = mSounds.find(instanceId);
        if (it != mSounds.end())
        {
            it->second->setPitch(pitch);
        }
    }

    void Start(
        TInstanceId instanceId,
        SoundType soundType,
        SoundStartMode startMode = SoundStartMode::Immediate)
    {
        auto it = mSounds.find(instanceId);
        if (it == mSounds.end())
        {
            //
            // Create sound altogether
            //

            auto const srchIt = mSoundTypeToSoundBuffer.find(soundType);
            if (srchIt == mSoundTypeToSoundBuffer.cend())
            {
                throw GameException("No sound buffer associated with sound type \"" + std::to_string(int(soundType)) + "\"");
            }

            auto newSound = std::make_unique<GameSound>(
                *(srchIt->second),
                mVolume,
                mMasterVolume,
                mIsMuted,
                mTimeToFadeIn,
                mTimeToFadeOut);

            newSound->setLoop(true);

            if (mIsPaused)
                newSound->pause();

            it = mSounds.emplace(
                instanceId,
                std::move(newSound)).first;
        }

        GameSound * sound = it->second.get();

        if (SoundStartMode::WithFadeIn == startMode)
        {
            sound->fadeToPlay();
        }
        else if (sf::Sound::Status::Playing != sound->getStatus())
        {
            sound->play();
        }
    }

    //
    // Must have been told already which sound type is associated with this instance ID.
    //
    void Start(
        TInstanceId instanceId,
        SoundStartMode startMode = SoundStartMode::Immediate)
    {
        auto const srchIt = mInstanceIdToSoundType.find(instanceId);
        if (srchIt != mInstanceIdToSoundType.cend())
        {
            Start(
                instanceId,
                srchIt->second,
                startMode);
        }
    }

    void SetPaused(bool isPaused)
    {
        mIsPaused = isPaused;

        for (auto & p : mSounds)
        {
            if (isPaused)
            {
                // Pausing
                p.second->pause();
            }
            else
            {
                // Resuming
                p.second->resume();
            }
        }
    }

    void Stop(
        TInstanceId instanceId,
        SoundStopMode stopMode = SoundStopMode::Immediate)
    {
        auto it = mSounds.find(instanceId);
        if (it != mSounds.end())
        {
            // We stop regardless of the pause state, even if we're paused
            if (sf::Sound::Status::Stopped != it->second->getStatus())
            {
                if (SoundStopMode::WithFadeOut == stopMode)
                    it->second->fadeToStop();
                else
                    it->second->stop();
            }
        }
    }

    void UpdateSimulation()
    {
        for (auto & p : mSounds)
        {
            p.second->update();
        }
    }

    void Reset()
    {
        mInstanceIdToSoundType.clear();
        mSounds.clear();
    }

private:

    float mVolume;
    float mMasterVolume;
    bool mIsMuted;
    std::chrono::milliseconds const mTimeToFadeIn;
    std::chrono::milliseconds const mTimeToFadeOut;
    bool mIsPaused;

    // SoundType<->SoundBuffer
    std::unordered_map<SoundType, std::unique_ptr<sf::SoundBuffer>> mSoundTypeToSoundBuffer;

    // InstanceId<->SoundType (optional)
    std::unordered_map<TInstanceId, SoundType> mInstanceIdToSoundType;

    // InstanceId<->Sound
    std::unordered_map<TInstanceId, std::unique_ptr<GameSound>> mSounds;
};
*/

struct OneShotMultipleChoiceSound
{
    std::vector<std::unique_ptr<SoundFile>> Choices;
    size_t LastPlayedSoundIndex;

    OneShotMultipleChoiceSound()
        : Choices()
        , LastPlayedSoundIndex(0u)
    {}
};

struct OneShotSingleChoiceSound
{
    std::unique_ptr<SoundFile> Choice;

    OneShotSingleChoiceSound()
        : Choice()
    {}

    void Initialize(std::unique_ptr<SoundFile> choice)
    {
        Choice = std::move(choice);
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
        std::unique_ptr<SoundFile> soundFile,
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mSoundAlternatives.emplace_back(
            std::move(soundFile),
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
        , mAggregateCount(0)
    {}

    void Initialize(
        std::unique_ptr<SoundFile> soundFile,
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero(),
        float aggregateRate = 0.3f)
    {
        mSound.Initialize(
            std::move(soundFile),
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut,
            aggregateRate);

        mAggregateCount = 0;
    }

    void Reset()
    {
        mSound.Stop();

        mAggregateCount = 0;
    }

    void SetVolume(float volume)
    {
        mSound.SetVolume(volume);
    }

    void SetMasterVolume(float volume)
    {
        mSound.SetMasterVolume(volume);
    }

    void AddAggregateVolume()
    {
        ++mAggregateCount;
        mSound.UpdateAggregateVolume(mAggregateCount);
    }

    void SubAggregateVolume()
    {
        --mAggregateCount;
        mSound.UpdateAggregateVolume(mAggregateCount);
    }

    void SetMuted(bool muted)
    {
        mSound.SetMuted(muted);
    }

    void SetPitch(float pitch)
    {
        mSound.SetPitch(pitch);
    }

    void Start()
    {
        mSound.Start();
    }

    void FadeIn()
    {
        mSound.Start(SoundStartMode::WithFadeIn);
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
        mSound.Stop(SoundStopMode::WithFadeOut);
    }

    void UpdateSimulation()
    {
        mSound.UpdateSimulation();
    }

protected:

    ContinuousSound mSound;

    size_t mAggregateCount;
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

template<typename TInstanceId>
class MultiInstanceLoopedSounds
{
public:

    MultiInstanceLoopedSounds(
        float masterVolume,
        bool isMuted)
        : mIsPaused(false)
        , mMasterVolume(masterVolume)
        , mIsMuted(isMuted)
    {
    }

    void AddAlternativeForSoundType(
        SoundType soundType,
        bool isUnderwater,
        std::filesystem::path soundFilePath)
    {
        AddAlternativeForSoundType(
            soundType,
            isUnderwater,
            soundFilePath,
            0.0f,
            0.0f);
    }

    void AddAlternativeForSoundType(
        SoundType soundType,
        bool isUnderwater,
        std::filesystem::path soundFilePath,
        float loopStartSample,
        float loopEndSample)
    {
        mSoundFileInfos.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(soundType, isUnderwater),
            std::forward_as_tuple(std::make_unique<SoundFileInfo>(soundFilePath, loopStartSample, loopEndSample)));
    }

    /*
     * Must be called first if we want to use the Start(.) overload that does not take a sound type.
     */
    void AddSoundTypeForInstanceId(TInstanceId instanceId, SoundType soundType)
    {
        assert(0 == mInstanceIdToSoundType.count(instanceId));

        mInstanceIdToSoundType.emplace(instanceId, soundType);
    }

    SoundType GetSoundTypeForInstanceId(TInstanceId instanceId) const
    {
        auto const it = mInstanceIdToSoundType.find(instanceId);
        assert(it != mInstanceIdToSoundType.cend());
        return it->second;
    }

    void SetMasterVolume(float masterVolume)
    {
        mMasterVolume = masterVolume;
        UpdateAllVolumes();
    }

    void SetMuted(bool isMuted)
    {
        mIsMuted = isMuted;
        UpdateAllVolumes();
    }

    void SetPitch(
        TInstanceId instanceId,
        float pitch)
    {
        auto it = mPlayingSounds.find(instanceId);
        if (it != mPlayingSounds.end())
        {
            it->second.Sound->setPitch(pitch);
        }
    }

    bool IsPlaying(TInstanceId instanceId)
    {
        auto const instanceSoundIt = mPlayingSounds.find(instanceId);
        return (instanceSoundIt != mPlayingSounds.end()
            && instanceSoundIt->second.IsPlaying);
    }

    void Start(
        TInstanceId instanceId,
        SoundType soundType,
        bool isUnderwater,
        float volume)
    {
        ScavengeSounds();

        // Find sound file info
        auto soundFileInfoIt = mSoundFileInfos.find(std::make_tuple(soundType, isUnderwater));
        if (soundFileInfoIt == mSoundFileInfos.end())
        {
            soundFileInfoIt = mSoundFileInfos.find(std::make_tuple(soundType, false));
            if (soundFileInfoIt == mSoundFileInfos.end())
                return;
        }

        assert(!!soundFileInfoIt->second);

        // See whether we're already playing this instance
        if (auto instanceSoundIt = mPlayingSounds.find(instanceId);
            instanceSoundIt != mPlayingSounds.end())
        {
            // Nuke existing sound
            instanceSoundIt->second.Sound->stop();
            mPlayingSounds.erase(instanceSoundIt);
        }

        // Create new sound
        auto sound = std::make_unique<sf::Music>();
        sound->openFromFile(soundFileInfoIt->second->FilePath.string());

        // Calculate volume
        // 0 -> 1
        // 1 -> 0.6
        // 5 -> 0.1
        // 10 -> 0.01
        // y = 0.01076043 + 0.986362*e^(-0.5049688*x)
        size_t & playingCount = mCurrentlyPlayingSoundCountsPerSoundType[soundType];
        volume *= Clamp(0.01076043f + 0.986362f * std::exp(-0.5049688f * static_cast<float>(playingCount)), 0.0f, 1.0f);

        // Setup sound
        InternalSetVolumeForSound(*sound, volume);
        sound->setLoop(true);
        if (soundFileInfoIt->second->LoopStartSample != 0.0f || soundFileInfoIt->second->LoopEndSample != 0.0f)
        {
            sound->setLoopPoints(
                sf::Music::TimeSpan(
                    sf::seconds(soundFileInfoIt->second->LoopStartSample),
                    sf::seconds(soundFileInfoIt->second->LoopEndSample - soundFileInfoIt->second->LoopStartSample)));
        }

        // Start sound
        sound->play();

        // Pause immediately if we're currently paused
        if (mIsPaused)
        {
            sound->pause();
        }

        // Store playing sound
        mPlayingSounds.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(instanceId),
            std::forward_as_tuple(std::move(sound), soundType, volume));

        // Maintain count and aggregate volume
        ++playingCount;
    }

    /*
     * Must have been told already which sound type is associated with this instance ID.
     */
    void Start(
        TInstanceId instanceId,
        bool isUnderwater,
        float volume)
    {
        // Figure out the sound type for this instance
        if (auto const srchIt = mInstanceIdToSoundType.find(instanceId);
            srchIt != mInstanceIdToSoundType.cend())
        {
            Start(
                instanceId,
                srchIt->second,
                isUnderwater,
                volume);
        }
    }

    void Stop(TInstanceId instanceId)
    {
        if (auto it = mPlayingSounds.find(instanceId);
            it != mPlayingSounds.end())
        {
            SoundType const soundType = it->second.Type;

            // Mark as not playing anymore, and let the sound run to its end
            it->second.IsPlaying = false;
            it->second.Sound->setLoop(false);

            // Maintain count and aggregate volume
            mCurrentlyPlayingSoundCountsPerSoundType[soundType]--;
        }
    }

    void SetPaused(bool isPaused)
    {
        for (auto & it : mPlayingSounds)
        {
            if (isPaused)
                it.second.Sound->pause();
            else
                it.second.Sound->play();
        }

        mIsPaused = isPaused;
    }

    void Reset()
    {
        mInstanceIdToSoundType.clear();
        mPlayingSounds.clear();
        mCurrentlyPlayingSoundCountsPerSoundType.clear();
    }

private:

    void ScavengeSounds()
    {
        for (auto it = mPlayingSounds.begin(); it != mPlayingSounds.end(); /*incremented in loop*/)
        {
            if (it->second.Sound->getStatus() == sf::SoundSource::Status::Stopped)
            {
                it = mPlayingSounds.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void UpdateAllVolumes()
    {
        for (auto it = mPlayingSounds.begin(); it != mPlayingSounds.end(); ++it)
        {
            InternalSetVolumeForSound(
                *(it->second.Sound),
                it->second.Volume);
        }
    }

    void InternalSetVolumeForSound(
        sf::Music & sound,
        float volume)
    {
        if (!mIsMuted)
        {
            // 100*(1 - e^(-0.01*x))
            float localVolume = 1.0f - exp(-0.01f * volume);
            sound.setVolume(100.0f * localVolume * (mMasterVolume / 100.0f));
        }
        else
        {
            sound.setVolume(0.0f);
        }
    }

private:

    bool mIsPaused;
    float mMasterVolume;
    bool mIsMuted;

    //
    // Association between <SoundType, Underwater-ness> and actual sound file
    //

    struct SoundFileInfo
    {
        std::filesystem::path FilePath;
        float LoopStartSample;
        float LoopEndSample;

        SoundFileInfo(
            std::filesystem::path filePath,
            float loopStartSample,
            float loopEndSample)
            : FilePath(filePath)
            , LoopStartSample(loopStartSample)
            , LoopEndSample(loopEndSample)
        {}
    };

    unordered_tuple_map<std::tuple<SoundType, bool>, std::unique_ptr<SoundFileInfo>> mSoundFileInfos;

    // InstanceId<->SoundType (optional)
    std::unordered_map<TInstanceId, SoundType> mInstanceIdToSoundType;

    //
    // Currently-playing sounds, keyed by instance ID
    //

    struct PlayingSoundInfo
    {
        std::unique_ptr<sf::Music> Sound;
        SoundType Type;
        bool IsPlaying; // Cleared when loop has been stopped via Stop(); says nothing about being paused
        float Volume; // Static volume asked for instance by caller; not necessarily volume being used for this instance now

        PlayingSoundInfo(
            std::unique_ptr<sf::Music> sound,
            SoundType type,
            float volume)
            : Sound(std::move(sound))
            , Type(type)
            , IsPlaying(true) // Start as playing
            , Volume(volume)
        {}
    };

    std::unordered_map<TInstanceId, PlayingSoundInfo> mPlayingSounds;

    //
    // Counts of currently-playing sounds, by sound type
    //

    std::unordered_map<SoundType, size_t> mCurrentlyPlayingSoundCountsPerSoundType;
};
