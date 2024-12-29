/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-11-17
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "MusicController.h"

#include <GameCore/GameException.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <limits>
#include <regex>

using namespace std::chrono_literals;

MusicController::MusicController(
    ResourceLocator const &  resourceLocator,
    ProgressCallback const & progressCallback)
    : // State
      mIsMuted(false)
    , mBackgroundMusicVolume(50.0f)
    , mPlayBackgroundMusic(true)
    , mGameMusicVolume(100.0f)
    , mPlaySinkingMusic(true)
    // Music
    , mBackgroundMusic(
        100.0f,
        mBackgroundMusicVolume,
        mIsMuted,
        std::chrono::seconds(2),
        std::chrono::seconds(2))
    , mSinkingMusic(
        80.0f,
        mGameMusicVolume,
        mIsMuted,
        std::chrono::seconds(2),
        std::chrono::seconds(4))
{
    //
    // Initialize Music
    //

    auto musicNames = resourceLocator.GetMusicNames();

    std::sort(musicNames.begin(), musicNames.end()); // Sort music deterministically

    for (size_t i = 0; i < musicNames.size(); ++i)
    {
        std::string const & musicName = musicNames[i];

        // Notify progress
        progressCallback(
            static_cast<float>(i + 1) / static_cast<float>(musicNames.size()),
            ProgressMessageType::LoadingMusic);

        //
        // Parse filename
        //

        static std::regex const MusicNameRegex(R"(([^_]+)(?:_([^_]+))?(?:_\d+))");

        std::smatch musicNameMatch;
        if (!std::regex_match(musicName, musicNameMatch, MusicNameRegex))
        {
            throw GameException("Music filename \"" + musicName + "\" is not recognized");
        }

        assert(musicNameMatch.size() == 1 + 2);

        if (musicNameMatch[1].str() == "background")
        {
            //
            // Background music
            //

            mBackgroundMusic.AddToPlaylist(resourceLocator.GetMusicFilePath(musicName));
        }
        else if (musicNameMatch[1].str() == "sinkingship")
        {
            //
            // Game music
            //

            // Parse frequency
            bool isRare = (musicNameMatch[2].str() == "rare");

            mSinkingMusic.AddAlternative(resourceLocator.GetMusicFilePath(musicName), isRare);
        }
    }

    SetPlayBackgroundMusic(mPlayBackgroundMusic);
}

MusicController::~MusicController()
{
    mBackgroundMusic.Stop();
    mSinkingMusic.Stop();
}

void MusicController::SetPaused(bool isPaused)
{
    if (isPaused)
    {
        mBackgroundMusic.Pause();
        mSinkingMusic.Pause();
    }
    else
    {
        mBackgroundMusic.Resume();
        mSinkingMusic.Resume();
    }
}

void MusicController::SetMuted(bool isMuted)
{
    mIsMuted = isMuted;

    mBackgroundMusic.SetMuted(mIsMuted);
    mSinkingMusic.SetMuted(mIsMuted);
}

void MusicController::SetBackgroundMusicVolume(float volume)
{
    mBackgroundMusicVolume = volume;

    mBackgroundMusic.SetMasterVolume(volume);
}

void MusicController::SetPlayBackgroundMusic(bool playBackgroundMusic)
{
    mPlayBackgroundMusic = playBackgroundMusic;

    // See whether we should start or stop the music
    if (playBackgroundMusic)
    {
        mBackgroundMusic.Play();
    }
    else
    {
        mBackgroundMusic.Stop();
    }
}

void MusicController::SetGameMusicVolume(float volume)
{
    mGameMusicVolume = volume;

    mSinkingMusic.SetMasterVolume(volume);
}

void MusicController::SetPlaySinkingMusic(bool playSinkingMusic)
{
    mPlaySinkingMusic = playSinkingMusic;

    if (!mPlaySinkingMusic)
    {
        mSinkingMusic.Stop();

        OnGameMusicStopped();
    }
}

void MusicController::UpdateSimulation()
{
    mBackgroundMusic.UpdateSimulation();
    mSinkingMusic.UpdateSimulation();
}

void MusicController::LowFrequencyUpdateSimulation()
{
}

void MusicController::Reset()
{
    mBackgroundMusic.Reset();
    mSinkingMusic.Reset();

    if (mPlayBackgroundMusic)
    {
        mBackgroundMusic.AdvanceNextPlaylistItem();
        mBackgroundMusic.Play();
    }
    else
    {
        mBackgroundMusic.Stop();
    }
}

///////////////////////////////////////////////////////////////////////////////////////

void MusicController::OnSinkingBegin(ShipId /*shipId*/)
{
    if (mPlaySinkingMusic)
    {
        mBackgroundMusic.FadeToStop();
        mSinkingMusic.FadeToPlay();
    }
}

void MusicController::OnSinkingEnd(ShipId /*shipId*/)
{
    mSinkingMusic.FadeToStop();

    OnGameMusicStopped();
}

void MusicController::OnSilenceStarted()
{
    mBackgroundMusic.FadeToStop();
    mSinkingMusic.FadeToStop();
}

void MusicController::OnSilenceLifted()
{
    if (mPlayBackgroundMusic)
    {
        mBackgroundMusic.FadeToPlay();
    }

    // If we were sinking, we won't resume the music
}

void MusicController::OnGameMusicStopped()
{
    if (mPlayBackgroundMusic)
    {
        mBackgroundMusic.FadeToPlay();
    }
}