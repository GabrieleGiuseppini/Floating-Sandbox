/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Sounds.h"

#include <Game/GameEventHandlers.h>
#include <Game/IGameController.h>
#include <Game/ResourceLoader.h>

#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ProgressCallback.h>
#include <GameCore/RunningAverage.h>
#include <GameCore/TupleKeys.h>
#include <GameCore/Utils.h>

#include <SFML/Audio.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class SoundController
    : public ILifecycleGameEventHandler
    , public IWavePhenomenaGameEventHandler
    , public ICombustionGameEventHandler
    , public IStructuralGameEventHandler
    , public IGenericGameEventHandler
{
public:

    SoundController(
        std::shared_ptr<ResourceLoader> resourceLoader,
        ProgressCallback const & progressCallback);

	virtual ~SoundController();

    //
    // Controlling
    //

    void SetPaused(bool isPaused);

    void SetMuted(bool isMuted);

    // Master effects

    float GetMasterEffectsVolume() const
    {
        return mMasterEffectsVolume;
    }

    void SetMasterEffectsVolume(float volume);

    bool GetMasterEffectsMuted() const
    {
        return mMasterEffectsMuted;
    }

    void SetMasterEffectsMuted(bool isMuted);

    // Master tools

    float GetMasterToolsVolume() const
    {
        return mMasterToolsVolume;
    }

    void SetMasterToolsVolume(float volume);

    bool GetMasterToolsMuted() const
    {
        return mMasterToolsMuted;
    }

    void SetMasterToolsMuted(bool isMuted);

    // Master Music

    float GetMasterMusicVolume() const
    {
        return mMasterMusicVolume;
    }

    void SetMasterMusicVolume(float volume);

    bool GetMasterMusicMuted() const
    {
        return mMasterMusicMuted;
    }

    void SetMasterMusicMuted(bool isMuted);

    // Misc

    bool GetPlayBreakSounds() const
    {
        return mPlayBreakSounds;
    }

    void SetPlayBreakSounds(bool playBreakSounds);

    bool GetPlayStressSounds() const
    {
        return mPlayStressSounds;
    }

    void SetPlayStressSounds(bool playStressSounds);

    bool GetPlayWindSound() const
    {
        return mPlayWindSound;
    }

    void SetPlayWindSound(bool playWindSound);

    bool GetPlaySinkingMusic() const
    {
        return mPlaySinkingMusic;
    }

    void SetPlaySinkingMusic(bool playSinkingMusic);

    void PlayDrawSound(bool isUnderwater);
    void StopDrawSound();

    void PlaySawSound(bool isUnderwater);
    void StopSawSound();

    void PlayHeatBlasterSound(HeatBlasterActionType action);
    void StopHeatBlasterSound();

    void PlayFireExtinguisherSound();
    void StopFireExtinguisherSound();

    void PlaySwirlSound(bool isUnderwater);
    void StopSwirlSound();

    void PlayAirBubblesSound();
    void StopAirBubblesSound();

    void PlayFloodHoseSound();
    void StopFloodHoseSound();

    void PlayTerrainAdjustSound();

    void PlayRepairStructureSound();
    void StopRepairStructureSound();

    void PlayWaveMakerSound();
    void StopWaveMakerSound();

    void PlayScrubSound();

    void PlaySnapshotSound();

    //
    // Updating
    //

    void Update();

    void LowFrequencyUpdate();

    void Reset();

public:

    //
    // Game event handlers
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterLifecycleEventHandler(this);
        gameController.RegisterWavePhenomenaEventHandler(this);
        gameController.RegisterCombustionEventHandler(this);
        gameController.RegisterStructuralEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    virtual void OnSinkingBegin(ShipId shipId) override;

    virtual void OnSinkingEnd(ShipId shipId) override;

    virtual void OnTsunamiNotification(float x) override;

    virtual void OnPointCombustionBegin() override;

    virtual void OnPointCombustionEnd() override;

    virtual void OnCombustionSmothered() override;

    virtual void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSpringRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTriangleRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnSawed(
        bool isMetal,
        unsigned int size) override;

    virtual void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override;

    virtual void OnLightFlicker(
        DurationShortLongType duration,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnWaterTaken(float waterTaken) override;

    virtual void OnWaterSplashed(float waterSplashed) override;

    virtual void OnWindSpeedUpdated(
        float const zeroSpeedMagnitude,
        float const baseSpeedMagnitude,
        float const preMaxSpeedMagnitude,
        float const maxSpeedMagnitude,
        vec2f const & windSpeed) override;

    virtual void OnBombPlaced(
        BombId bombId,
        BombType bombType,
        bool isUnderwater) override;

    virtual void OnBombRemoved(
        BombId bombId,
        BombType bombType,
        std::optional<bool> isUnderwater) override;

    virtual void OnBombExplosion(
        BombType bombType,
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnTimerBombFuse(
        BombId bombId,
        std::optional<bool> isFast) override;

    virtual void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override;

    virtual void OnAntiMatterBombContained(
        BombId bombId,
        bool isContained) override;

    virtual void OnAntiMatterBombPreImploding() override;

    virtual void OnAntiMatterBombImploding() override;

private:

    struct PlayingSound
    {
        SoundType Type;
        std::unique_ptr<GameSound> Sound;
        GameWallClock::time_point StartedTimestamp;
        bool IsInterruptible;

        PlayingSound(
            SoundType type,
            std::unique_ptr<GameSound> sound,
            GameWallClock::time_point startedTimestamp,
            bool isInterruptible)
            : Type(type)
            , Sound(std::move(sound))
            , StartedTimestamp(startedTimestamp)
            , IsInterruptible(isInterruptible)
        {
        }
    };

private:

    void PlayMSUOneShotMultipleChoiceSound(
        SoundType soundType,
        StructuralMaterial::MaterialSoundType materialSound,
        unsigned int size,
        bool isUnderwater,
        float volume,
        bool isInterruptible);

    void PlayDslUOneShotMultipleChoiceSound(
        SoundType soundType,
        DurationShortLongType duration,
        bool isUnderwater,
        float volume,
        bool isInterruptible);

    void PlayUOneShotMultipleChoiceSound(
        SoundType soundType,
        bool isUnderwater,
        float volume,
        bool isInterruptible);

    void PlayOneShotMultipleChoiceSound(
        SoundType soundType,
        float volume,
        bool isInterruptible);

    void ChooseAndPlayOneShotMultipleChoiceSound(
        SoundType soundType,
        OneShotMultipleChoiceSound & sound,
        float volume,
        bool isInterruptible);

    void PlayOneShotSound(
        SoundType soundType,
        sf::SoundBuffer * soundBuffer,
        float volume,
        bool isInterruptible);

    void ScavengeStoppedSounds(std::vector<PlayingSound> & playingSounds);

    void ScavengeOldestSound(std::vector<PlayingSound> & playingSounds);

private:

    std::shared_ptr<ResourceLoader> mResourceLoader;


    //
    // State
    //

    float mMasterEffectsVolume;
    bool mMasterEffectsMuted;
    float mMasterToolsVolume;
    bool mMasterToolsMuted;
    float mMasterMusicVolume;
    bool mMasterMusicMuted;

    bool mPlayBreakSounds;
    bool mPlayStressSounds;
    bool mPlayWindSound;
    bool mPlaySinkingMusic;

    float mLastWaterSplashed;
    float mCurrentWaterSplashedTrigger;
    float mLastWindSpeedAbsoluteMagnitude;
    RunningAverage<70> mWindVolumeRunningAverage;



    //
    // One-Shot sounds
    //

    static constexpr size_t GetMaxPlayingSoundsForType(SoundType soundType)
    {
        switch (soundType)
        {
            case SoundType::Break:
            case SoundType::Destroy:
                return 45;
            case SoundType::Stress:
                return 30;
            case SoundType::TerrainAdjust:
            case SoundType::Scrub:
            case SoundType::Snapshot:
                return 2;
            default:
                return 15;
        }
    }

    static constexpr std::chrono::milliseconds GetMinDeltaTimeSoundForType(SoundType soundType)
    {
        switch (soundType)
        {
            case SoundType::Break:
            case SoundType::Destroy:
            case SoundType::RepairSpring:
            case SoundType::RepairTriangle:
                return std::chrono::milliseconds(200);
            case SoundType::Stress:
                return std::chrono::milliseconds(600);
            case SoundType::TerrainAdjust:
            case SoundType::Snapshot:
                return std::chrono::milliseconds(700);
            default:
                return std::chrono::milliseconds(75);
        }
    }

    unordered_tuple_map<
        std::tuple<SoundType, StructuralMaterial::MaterialSoundType, SizeType, bool>,
        OneShotMultipleChoiceSound> mMSUOneShotMultipleChoiceSounds;

    unordered_tuple_map<
        std::tuple<SoundType, DurationShortLongType, bool>,
        OneShotMultipleChoiceSound> mDslUOneShotMultipleChoiceSounds;

    unordered_tuple_map<
        std::tuple<SoundType, bool>,
        OneShotMultipleChoiceSound> mUOneShotMultipleChoiceSounds;

    unordered_tuple_map<
        std::tuple<SoundType>,
        OneShotMultipleChoiceSound> mOneShotMultipleChoiceSounds;

    std::unordered_map<SoundType, std::vector<PlayingSound>> mCurrentlyPlayingOneShotSounds;

    //
    // Continuous sounds
    //

    ContinuousInertialSound mSawedMetalSound;
    ContinuousInertialSound mSawedWoodSound;

    ContinuousSingleChoiceSound mSawAbovewaterSound;
    ContinuousSingleChoiceSound mSawUnderwaterSound;
    ContinuousSingleChoiceSound mHeatBlasterCoolSound;
    ContinuousSingleChoiceSound mHeatBlasterHeatSound;
    ContinuousSingleChoiceSound mFireExtinguisherSound;
    ContinuousSingleChoiceSound mDrawSound;
    ContinuousSingleChoiceSound mSwirlSound;
    ContinuousSingleChoiceSound mAirBubblesSound;
    ContinuousSingleChoiceSound mFloodHoseSound;
    ContinuousSingleChoiceSound mRepairStructureSound;
    ContinuousSingleChoiceSound mWaveMakerSound;

    ContinuousSingleChoiceSound mWaterRushSound;
    ContinuousSingleChoiceSound mWaterSplashSound;
    ContinuousSingleChoiceSound mWindSound;
    ContinuousSingleChoiceSound mFireBurningSound;

    ContinuousSingleChoiceAggregateSound<BombId> mTimerBombSlowFuseSound;
    ContinuousSingleChoiceAggregateSound<BombId> mTimerBombFastFuseSound;
    ContinuousMultipleChoiceAggregateSound<BombId> mAntiMatterBombContainedSounds;

    //
    // Music
    //

    GameMusic mSinkingMusic;
};
