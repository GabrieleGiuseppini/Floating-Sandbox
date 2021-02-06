/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Sounds.h"

#include <Game/IGameController.h>
#include <Game/IGameEventHandlers.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameRandomEngine.h>
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
    : public IWavePhenomenaGameEventHandler
    , public ICombustionGameEventHandler
    , public IStructuralGameEventHandler
	, public IAtmosphereGameEventHandler
    , public IElectricalElementGameEventHandler
    , public IGenericGameEventHandler
{
public:

    SoundController(
        ResourceLocator & resourceLocator,
        ProgressCallback const & progressCallback);

	virtual ~SoundController();

    //
    // Controlling
    //

    void SetPaused(bool isPaused);

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

    bool GetPlayAirBubbleSurfaceSound() const
    {
        return mPlayAirBubbleSurfaceSound;
    }

    void SetPlayAirBubbleSurfaceSound(bool playAirBubbleSurfaceSound);

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

    void PlayThanosSnapSound();

    void PlayWaveMakerSound();
    void StopWaveMakerSound();

    void PlayScrubSound();

    void PlayRotSound();

    void PlayPliersSound(bool isUnderwater);

    void PlayFishScareSound();
    void StopFishScareSound();

    void PlayFishFoodSound();
    void StopFishFoodSound();

    void PlaySnapshotSound();

    void PlayElectricalPanelOpenSound(bool isClose);

    void PlayElectricalPanelDockSound(bool isUndock);

    void PlayTickSound();

    void PlayErrorSound();

    //
    // Updating
    //

    void UpdateSimulation();

    void LowFrequencyUpdateSimulation();

    void Reset();

public:

    //
    // Game event handlers
    //

    void RegisterEventHandler(IGameController & gameController)
    {
        gameController.RegisterWavePhenomenaEventHandler(this);
        gameController.RegisterCombustionEventHandler(this);
        gameController.RegisterStructuralEventHandler(this);
		gameController.RegisterAtmosphereEventHandler(this);
        gameController.RegisterElectricalElementEventHandler(this);
        gameController.RegisterGenericEventHandler(this);
    }

    void OnTsunamiNotification(float x) override;

    void OnPointCombustionBegin() override;

    void OnPointCombustionEnd() override;

    void OnCombustionSmothered() override;

    void OnCombustionExplosion(
        bool isUnderwater,
        unsigned int size) override;

    void OnStress(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    void OnBreak(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    void OnDestroy(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

	void OnLightningHit(StructuralMaterial const & structuralMaterial) override;

    void OnSpringRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    void OnTriangleRepaired(
        StructuralMaterial const & structuralMaterial,
        bool isUnderwater,
        unsigned int size) override;

    void OnSawed(
        bool isMetal,
        unsigned int size) override;

    void OnPinToggled(
        bool isPinned,
        bool isUnderwater) override;

    void OnWaterTaken(float waterTaken) override;

    void OnWaterSplashed(float waterSplashed) override;

    void OnAirBubbleSurfaced(unsigned int size) override;

    void OnWindSpeedUpdated(
        float const zeroSpeedMagnitude,
        float const baseSpeedMagnitude,
        float const baseAndStormSpeedMagnitude,
        float const preMaxSpeedMagnitude,
        float const maxSpeedMagnitude,
        vec2f const & windSpeed) override;

	void OnRainUpdated(float density) override;

	void OnThunder() override;

	void OnLightning() override;

    void OnLightFlicker(
        DurationShortLongType duration,
        bool isUnderwater,
        unsigned int size) override;

    void OnEngineMonitorCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & electricalMaterial,
        float thrustMagnitude,
        float rpm,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    void OnWaterPumpCreated(
        ElectricalElementId electricalElementId,
        ElectricalElementInstanceIndex instanceIndex,
        ElectricalMaterial const & electricalMaterial,
        float normalizedForce,
        std::optional<ElectricalPanelElementMetadata> const & panelElementMetadata) override;

    void OnSwitchToggled(
        ElectricalElementId electricalElementId,
        ElectricalState newState) override;

    void OnEngineControllerUpdated(
        ElectricalElementId electricalElementId,
        int telegraphValue) override;

    void OnEngineMonitorUpdated(
        ElectricalElementId electricalElementId,
        float thrustMagnitude,
        float rpm) override;

    void OnShipSoundUpdated(
        ElectricalElementId electricalElementId,
        ElectricalMaterial const & electricalMaterial,
        bool isPlaying,
        bool isUnderwater) override;

    void OnWaterPumpUpdated(
        ElectricalElementId electricalElementId,
        float normalizedForce) override;

    void OnGadgetPlaced(
        GadgetId gadgetId,
        GadgetType gadgetType,
        bool isUnderwater) override;

    void OnGadgetRemoved(
        GadgetId gadgetId,
        GadgetType gadgetType,
        std::optional<bool> isUnderwater) override;

    void OnBombExplosion(
        GadgetType gadgetType,
        bool isUnderwater,
        unsigned int size) override;

    void OnRCBombPing(
        bool isUnderwater,
        unsigned int size) override;

    void OnTimerBombFuse(
        GadgetId gadgetId,
        std::optional<bool> isFast) override;

    void OnTimerBombDefused(
        bool isUnderwater,
        unsigned int size) override;

    void OnAntiMatterBombContained(
        GadgetId gadgetId,
        bool isContained) override;

    void OnAntiMatterBombPreImploding() override;

    void OnAntiMatterBombImploding() override;

    void OnWatertightDoorOpened(
        bool isUnderwater,
        unsigned int size) override;

    void OnWatertightDoorClosed(
        bool isUnderwater,
        unsigned int size) override;

    void OnPhysicsProbePanelOpened() override;

    void OnPhysicsProbePanelClosed() override;

private:

    struct PlayingSound
    {
        SoundType Type;
        std::unique_ptr<GameSound> Sound;
        std::chrono::steady_clock::time_point StartedTimestamp;
        bool IsInterruptible;

        PlayingSound(
            SoundType type,
            std::unique_ptr<GameSound> sound,
            std::chrono::steady_clock::time_point startedTimestamp,
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

	void PlayMOneShotMultipleChoiceSound(
		SoundType soundType,
		StructuralMaterial::MaterialSoundType materialSound,
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

    //
    // State
    //

    float mMasterEffectsVolume;
    bool mMasterEffectsMuted;
    float mMasterToolsVolume;
    bool mMasterToolsMuted;

    bool mPlayBreakSounds;
    bool mPlayStressSounds;
    bool mPlayWindSound;
    bool mPlayAirBubbleSurfaceSound;

    float mLastWaterSplashed;
    float mCurrentWaterSplashedTrigger;
    float mLastWindSpeedAbsoluteMagnitude;
    RunningAverage<70> mWindVolumeRunningAverage;
    float mShipEnginesCount;

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
            case SoundType::ThanosSnap:
            case SoundType::Scrub:
            case SoundType::Rot:
            case SoundType::Snapshot:
            case SoundType::Error:
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
            case SoundType::GlassTick:
            case SoundType::InteractiveSwitchOn:
            case SoundType::InteractiveSwitchOff:
            case SoundType::ElectricalPanelOpen:
            case SoundType::ElectricalPanelClose:
            case SoundType::ElectricalPanelDock:
            case SoundType::ElectricalPanelUndock:
            case SoundType::EngineTelegraph:
                return std::chrono::milliseconds(0);
            default:
                return std::chrono::milliseconds(75);
        }
    }

    unordered_tuple_map<
        std::tuple<SoundType, StructuralMaterial::MaterialSoundType, SizeType, bool>,
        OneShotMultipleChoiceSound> mMSUOneShotMultipleChoiceSounds;

	unordered_tuple_map<
		std::tuple<SoundType, StructuralMaterial::MaterialSoundType>,
		OneShotMultipleChoiceSound> mMOneShotMultipleChoiceSounds;

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
    ContinuousSingleChoiceSound mFishScareSound;
    ContinuousSingleChoiceSound mFishFoodSound;

    ContinuousSingleChoiceSound mWaterRushSound;
    ContinuousSingleChoiceSound mWaterSplashSound;
    ContinuousPulsedSound mAirBubblesSurfacingSound;
    ContinuousSingleChoiceSound mWindSound;
	ContinuousSingleChoiceSound mRainSound;
    ContinuousSingleChoiceSound mFireBurningSound;

    ContinuousSingleChoiceAggregateSound<GadgetId> mTimerBombSlowFuseSound;
    ContinuousSingleChoiceAggregateSound<GadgetId> mTimerBombFastFuseSound;
    ContinuousMultipleChoiceAggregateSound<GadgetId> mAntiMatterBombContainedSounds;

    MultiInstanceLoopedSounds<ElectricalElementId> mLoopedSounds;
};
