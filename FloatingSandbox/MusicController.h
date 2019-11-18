/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-11-17
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Music.h"

#include <Game/GameEventHandlers.h>
#include <Game/IGameController.h>
#include <Game/ResourceLoader.h>

#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/ProgressCallback.h>

#include <SFML/Audio.hpp>

#include <cassert>
#include <chrono>
#include <memory>
#include <limits>
#include <set>
#include <string>
#include <vector>

class MusicController
    : public ILifecycleGameEventHandler
    , public IGenericGameEventHandler
{
public:

    MusicController(
        ResourceLoader & resourceLoader,
        ProgressCallback const & progressCallback);

	virtual ~MusicController();

    //
    // Controlling
    //

    void SetPaused(bool isPaused);

    void SetMuted(bool isMuted);

    bool GetMuted() const
    {
        return mIsMuted;
    }

    // Background Music

    float GetBackgroundMusicVolume() const
    {
        return mBackgroundMusicVolume;
    }

    void SetBackgroundMusicVolume(float volume);

    bool GetPlayBackgroundMusic() const
    {
        return mPlayBackgroundMusic;
    }

    void SetPlayBackgroundMusic(bool playBackgroundMusic);

    // Game Music

    float GetGameMusicVolume() const
    {
        return mGameMusicVolume;
    }

    void SetGameMusicVolume(float volume);

    bool GetPlaySinkingMusic() const
    {
        return mPlaySinkingMusic;
    }

    void SetPlaySinkingMusic(bool playSinkingMusic);

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
        gameController.RegisterGenericEventHandler(this);
    }

    virtual void OnSinkingBegin(ShipId shipId) override;

    virtual void OnSinkingEnd(ShipId shipId) override;

    virtual void OnSilenceStarted() override;

    virtual void OnSilenceLifted() override;

private:

    void OnGameMusicStopped();

private:

    //
    // State
    //

    bool mIsMuted;

    float mBackgroundMusicVolume;
    bool mPlayBackgroundMusic;

    float mGameMusicVolume;
    bool mPlaySinkingMusic;


    //
    // Music
    //

    BackgroundMusic mBackgroundMusic;
    GameMusic mSinkingMusic;
};
