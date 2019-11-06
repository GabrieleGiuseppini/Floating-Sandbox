/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "RenderContext.h"

#include <GameCore/GameWallClock.h>
#include <GameCore/Vectors.h>

#include <memory>
#include <vector>

namespace Physics
{

class Storm
{

public:

	Storm(
		World & parentWorld,
		std::shared_ptr<GameEventDispatcher> gameEventDispatcher);

    void Update(GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

public:

    struct Parameters
    {
        float WindSpeed; // Km/h, absolute (on top of current direction)
        unsigned int NumberOfClouds;
        float CloudsSize; // [0.0f = initial size, 1.0 = full size]
        float CloudDarkening; // [0.0f = full darkness, 1.0 = no darkening]
        float AmbientDarkening; // [0.0f = full darkness, 1.0 = no darkening]
		float RainDensity; // [0.0f = no rain, 1.0f = full rain]

        Parameters()
        {
            Reset();
        }

        void Reset()
        {
            WindSpeed = 0.0f;
            NumberOfClouds = 0;
            CloudsSize = 0.0f;
            CloudDarkening = 1.0f;
            AmbientDarkening = 1.0f;
			RainDensity = 0.0f;
        }
    };

    Parameters const & GetParameters() const
    {
        return mParameters;
    }

    void TriggerStorm();

	void TriggerLightning();

private:

    void TurnStormOn(GameWallClock::time_point now);
    void TurnStormOff();

	void DoTriggerBackgroundLightning(GameWallClock::time_point now);
	void DoTriggerForegroundLightning(
		GameWallClock::time_point now,
		vec2f const & targetWorldPosition);

private:

	World & mParentWorld;
	std::shared_ptr<GameEventDispatcher> mGameEventHandler;

	//
	// Lightning state machine
	//

	class BaseLightningStateMachine
	{
	public:

		virtual ~BaseLightningStateMachine()
		{}

		bool Update(
			GameWallClock::time_point now,
			GameParameters const & /*gameParameters*/)
		{
			static constexpr float LightningDuration = 4.0f; // TODOTEST

			// Calculate progress of lightning: 0.0f = beginning, 1.0f = end
			mProgress = std::min(1.0f,				
				std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - mStartTimestamp).count()
				/ LightningDuration);

			return (mProgress == 1.0f);
		}

		virtual void Upload(Render::RenderContext & renderContext) const = 0;

	protected:

		BaseLightningStateMachine(
			GameWallClock::time_point startTimestamp,
			float personalitySeed)
			: mProgress(0.0f)
			, mPersonalitySeed(personalitySeed)
			, mStartTimestamp(startTimestamp)
		{}

		float mProgress;
		float const mPersonalitySeed;

	private:

		GameWallClock::time_point const mStartTimestamp;
	};

	class BackgroundLightningStateMachine final : public BaseLightningStateMachine
	{
	public:

		BackgroundLightningStateMachine(
			GameWallClock::time_point startTimestamp,
			float ndcX,
			float personalitySeed)
			: BaseLightningStateMachine(startTimestamp, personalitySeed)
			, mNdcX(ndcX)
		{}

		void Upload(Render::RenderContext & renderContext) const override
		{
			renderContext.UploadBackgroundLightning(mNdcX, mProgress, mPersonalitySeed);
		}

	private:

		float const mNdcX;
	};

	class ForegroundLightningStateMachine final : public BaseLightningStateMachine
	{
	public:

		ForegroundLightningStateMachine(
			GameWallClock::time_point startTimestamp,
			vec2f targetWorldPosition,
			float personalitySeed)
			: BaseLightningStateMachine(startTimestamp, personalitySeed)
			, mTargetWorldPosition(targetWorldPosition)
		{}

		void Upload(Render::RenderContext & renderContext) const override
		{
			renderContext.UploadForegroundLightning(mTargetWorldPosition, mProgress, mPersonalitySeed);
		}

	private:

		vec2f const mTargetWorldPosition;
	};

    //
    // Storm state machine
    //

	// The storm output
	Parameters mParameters;

    // Flag indicating whether we are in a storm or waiting for one
    bool mIsInStorm;

    // The current progress of the storm, when in a storm: [0.0, 1.0]
    float mCurrentStormProgress;

	// The timestamp at which we last did a storm update
	GameWallClock::time_point mLastStormUpdateTimestamp;

	// The CDF for thunders
	float const mThunderCdf;

	// The CDF for lightnings
	float const mLightningCdf;

	// The next timestamp at which to sample the Poisson distribution for deciding various things
	GameWallClock::time_point mNextThunderPoissonSampleTimestamp;
	GameWallClock::time_point mNextBackgroundLightningPoissonSampleTimestamp;
	GameWallClock::time_point mNextForegroundLightningPoissonSampleTimestamp;

	// The current lightnings' state machines
	std::vector<std::unique_ptr<BaseLightningStateMachine>> mLightnings;
};

}
