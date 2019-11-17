/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-11-17
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/GameException.h>
#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>

#include <SFML/Audio.hpp>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <memory>
#include <limits>
#include <optional>
#include <string>
#include <vector>

/*
 * The logical state of music, net of fade in/out, etc.
 */
enum class LogicalMusicStatus
{
    Stopped,
    Playing
};

/*
 * Our base wrapper for sf::Music.
 *
 * Provides volume control based on a master volume and a local volume,
 * and facilities to fade-in and fade-out.
 */
class BaseGameMusic
{
public:

    BaseGameMusic(
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn,
        std::chrono::milliseconds timeToFadeOut)
        : mMusic()
        , mVolume(volume)
        , mMasterVolume(masterVolume)
        , mFadeLevel(1.0f)
        , mIsMuted(isMuted)
        , mLogicalStatus(LogicalMusicStatus::Stopped)
        , mTimeToFadeIn(timeToFadeIn)
        , mTimeToFadeOut(timeToFadeOut)
        , mFadeInStartTimestamp()
        , mFadeOutStartTimestamp()
    {
        InternalSetVolume();
    }

    void SetVolume(float volume)
    {
        mVolume = volume;
        InternalSetVolume();
    }

    void SetMasterVolume(float masterVolume)
    {
        mMasterVolume = masterVolume;
        InternalSetVolume();
    }

    void SetMuted(bool isMuted)
    {
        mIsMuted = isMuted;
        InternalSetVolume();
    }

    void SetVolumes(
        float volume,
        float masterVolume,
        bool isMuted)
    {
        mVolume = volume;
        mMasterVolume = masterVolume;
        mIsMuted = isMuted;
        InternalSetVolume();
    }

    LogicalMusicStatus GetLogicalStatus() const
    {
        return mLogicalStatus;
    }

    /*
     * NOP if already playing.
     */
    void Play()
    {
        if (mLogicalStatus != LogicalMusicStatus::Playing)
        {
            // Reset fade
            mFadeLevel = 1.0f;
            InternalSetVolume();

            // Play
            InternalStart();

            // Reset state
            mFadeInStartTimestamp.reset();
            mFadeOutStartTimestamp.reset();

            mLogicalStatus = LogicalMusicStatus::Playing;
        }
    }

    /*
     * NOP if already playing.
     */
    void FadeToPlay()
    {
        if (mLogicalStatus != LogicalMusicStatus::Playing)
        {
            // Reset state
            mFadeInStartTimestamp = GameWallClock::GetInstance().Now();
            mFadeOutStartTimestamp.reset();

            mLogicalStatus = LogicalMusicStatus::Playing;
        }
    }

    /*
     * NOP if already stopped.
     */
    void Stop()
    {
        if (mLogicalStatus != LogicalMusicStatus::Stopped)
        {
            // Stop
            InternalStop();

            // Reset state
            mFadeInStartTimestamp.reset();
            mFadeOutStartTimestamp.reset();

            mLogicalStatus = LogicalMusicStatus::Stopped;
        }
    }

    /*
     * NOP if already stopped.
     */
    void FadeToStop()
    {
        if (mLogicalStatus != LogicalMusicStatus::Stopped)
        {
            mFadeInStartTimestamp.reset();
            mFadeOutStartTimestamp = GameWallClock::GetInstance().Now();

            mLogicalStatus = LogicalMusicStatus::Stopped;
        }
    }

    /*
     * NOP if already paused.
     */
    void Pause()
    {
        mMusic.pause();
    }

    /*
     * NOP if already playing.
     */
    void Resume()
    {
        if (sf::Sound::Status::Paused == mMusic.getStatus())
        {
            mMusic.play();
        }
    }

    void Reset()
    {
        Stop();
    }

    void Update()
    {
        if (!!mFadeInStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeInStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeIn)
            {
                // Reset state
                mFadeLevel = 1.0f;
                mFadeInStartTimestamp.reset();
            }
            else
            {
                // Raise volume
                mFadeLevel = static_cast<float>(elapsedMillis.count()) / static_cast<float>(mTimeToFadeOut.count());
            }

            InternalSetVolume();

            // Make sure we're playing
            if (sf::Sound::Status::Stopped == mMusic.getStatus())
            {
                InternalStart();
            }
        }
        else if (!!mFadeOutStartTimestamp)
        {
            auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(
                GameWallClock::GetInstance().Elapsed(*mFadeOutStartTimestamp));

            // Check if we're done
            if (elapsedMillis >= mTimeToFadeOut)
            {
                if (sf::Sound::Status::Stopped != mMusic.getStatus())
                {
                    InternalStop();
                }

                // Reset state
                mFadeInStartTimestamp.reset();
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

protected:

    virtual std::optional<std::filesystem::path> GetMusicFileToPlay() const = 0;

    sf::Music mMusic;

private:

    void InternalSetVolume()
    {
        if (!mIsMuted)
            mMusic.setVolume(100.0f * (mVolume / 100.0f) * (mMasterVolume / 100.0f) * mFadeLevel);
        else
            mMusic.setVolume(0.0f);
    }

    void InternalStart()
    {
        // Load file
        auto const musicFilePath = GetMusicFileToPlay();
        if (!!musicFilePath)
        {
            if (!mMusic.openFromFile(musicFilePath->string()))
            {
                throw GameException("Cannot load \"" + musicFilePath->string() + "\" music");
            }

            // Play
            mMusic.play();
        }
    }

    void InternalStop()
    {
        mMusic.stop();
    }

private:

    float mVolume;
    float mMasterVolume;
    float mFadeLevel;
    bool mIsMuted;

    LogicalMusicStatus mLogicalStatus;

    std::chrono::milliseconds const mTimeToFadeIn;
    std::chrono::milliseconds const mTimeToFadeOut;
    std::optional<GameWallClock::time_point> mFadeInStartTimestamp;
    std::optional<GameWallClock::time_point> mFadeOutStartTimestamp;
};

/*
 * Background music wraps a playlist made of multiple music files, which are
 * played continuously one after each other, until the music is stopped.
 */
class BackgroundMusic : public BaseGameMusic
{
public:

    BackgroundMusic(
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : BaseGameMusic(
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut)
        , mPlaylist()
        , mCurrentPlaylistItem(0)
        , mDesiredPlayStatus(false)
    {
        mMusic.setLoop(false);
    }

    void AddToPlaylist(std::filesystem::path const & filepath)
    {
        mPlaylist.push_back(filepath);
    }

    void Play()
    {
        // Rewind playlist from beginning
        mCurrentPlaylistItem = 0;

        BaseGameMusic::Play();

        mDesiredPlayStatus = true;
    }

    void FadeToPlay()
    {
        // Resume playing from current playlist entry

        BaseGameMusic::FadeToPlay();

        mDesiredPlayStatus = true;
    }

    void Stop()
    {
        BaseGameMusic::Stop();

        mDesiredPlayStatus = false;
    }

    void FadeToStop()
    {
        BaseGameMusic::FadeToStop();

        mDesiredPlayStatus = false;
    }

    void Update()
    {
        // Check whether we need to start the next entry in the playlist, after
        // the current entry has finished playing
        if (mDesiredPlayStatus == true
            && sf::SoundSource::Status::Stopped == mMusic.getStatus()
            && !mPlaylist.empty())
        {
            LogMessage("TODOTEST: BackgroundMusic::Update: starting new music");

            // Play new entry
            BaseGameMusic::Play();

            // Advance playlist entry
            ++mCurrentPlaylistItem;
            if (mCurrentPlaylistItem > mPlaylist.size())
                mCurrentPlaylistItem = 0;
        }

        BaseGameMusic::Update();
    }

protected:

    std::optional<std::filesystem::path> GetMusicFileToPlay() const override
    {
        if (!mPlaylist.empty())
        {
            assert(mCurrentPlaylistItem < mPlaylist.size());

            return mPlaylist[mCurrentPlaylistItem];
        }
        else
        {
            return std::nullopt;
        }
    }

private:

    std::vector<std::filesystem::path> mPlaylist;
    size_t mCurrentPlaylistItem; // The index of the playlist entry that we're playing now

    bool mDesiredPlayStatus;
};

/*
 * Game music has multiple alternatives, one of which is chosen randomly
 * when the music is started, and plays that alternative continuously,
 * until stopped.
 */
class GameMusic : public BaseGameMusic
{
public:

    GameMusic(
        float volume,
        float masterVolume,
        bool isMuted,
        std::chrono::milliseconds timeToFadeIn = std::chrono::milliseconds::zero(),
        std::chrono::milliseconds timeToFadeOut = std::chrono::milliseconds::zero())
        : BaseGameMusic(
            volume,
            masterVolume,
            isMuted,
            timeToFadeIn,
            timeToFadeOut)
        , mAlternatives()
        , mRareAlternatives()
    {
        mMusic.setLoop(true);
    }

    void AddAlternative(
		std::filesystem::path const & filepath,
		bool isRare)
    {
		if (!isRare)
			mAlternatives.push_back(filepath);
		else
			mRareAlternatives.push_back(filepath);
    }

protected:

    std::optional<std::filesystem::path> GetMusicFileToPlay() const override
    {
        // Choose alternative
		if (mRareAlternatives.empty()
			|| GameRandomEngine::GetInstance().GenerateUniformBoolean(0.975f))
		{
            //
            // Choose normal alternative
            //

			auto alternativeToPlay = GameRandomEngine::GetInstance().Choose(mAlternatives.size());
            return mAlternatives[alternativeToPlay];
		}
        else
        {
            //
            // Choose rare alternative
            //

            auto alternativeToPlay = GameRandomEngine::GetInstance().Choose(mRareAlternatives.size());
            return mRareAlternatives[alternativeToPlay];
        }
    }

private:

    std::vector<std::filesystem::path> mAlternatives;
	std::vector<std::filesystem::path> mRareAlternatives;
};
