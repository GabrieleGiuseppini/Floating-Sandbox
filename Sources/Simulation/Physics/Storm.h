/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationEventDispatcher.h"
#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <Core/GameWallClock.h>
#include <Core/Vectors.h>

#include <memory>
#include <list>
#include <vector>

namespace Physics
{

class Storm
{

public:

	Storm(
		World & parentWorld,
		SimulationEventDispatcher & simulationEventDispatcher);

    void Update(SimulationParameters const & simulationParameters);

    void Upload(RenderContext & renderContext) const;

public:

    struct Parameters
    {
        float WindSpeed; // Km/h, absolute (on top of current direction)
        unsigned int NumberOfClouds;
        float CloudsSize; // [0.0f = initial size, 1.0 = full size]
        float CloudDarkening; // [0.0f = full darkness, 1.0 = no darkening]
        float AmbientDarkening; // [0.0f = full darkness, 1.0 = no darkening]
		float RainDensity; // [0.0f = no rain, 1.0f = full rain]
        float RainQuantity; // m/h
        float AirTemperatureDelta; // K

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
            RainQuantity = 0.0f;
            AirTemperatureDelta = 0.0f;
        }
    };

    Parameters const & GetParameters() const
    {
        return mParameters;
    }

    void TriggerStorm();

	void TriggerLightning(SimulationParameters const & simulationParameters);

    void TriggerForegroundLightningAt(vec2f const & targetWorldPosition);

private:

	void RecalculateCoefficients(
		GameWallClock::time_point now,
		SimulationParameters const & simulationParameters);

	static GameWallClock::time_point CalculateNextStormTimestamp(
		GameWallClock::time_point lastTimestamp,
		std::chrono::minutes rate);

    void TurnStormOn(GameWallClock::time_point now);
    void TurnStormOff(GameWallClock::time_point now);

	void DoTriggerBackgroundLightning(GameWallClock::time_point now);
	void DoTriggerForegroundLightning(
		GameWallClock::time_point now,
		vec2f const & targetWorldPosition);
	void UpdateLightnings(
		GameWallClock::time_point now,
		SimulationParameters const & simulationParameters);
	void UploadLightnings(RenderContext & renderContext) const;

private:

	World & mParentWorld;
	SimulationEventDispatcher & mSimulationEventHandler;

	//
	// Lightning state machine
	//

	class LightningStateMachine
	{
	public:

		enum class LightningType
		{
			Background,
			Foreground
		};

		LightningType const Type;
		float const PersonalitySeed;
		GameWallClock::time_point const StartTimestamp;

		std::optional<float> const NdcX;
		std::optional<vec2f> const TargetWorldPosition;
		float Progress;
		float RenderProgress;
		bool HasNotifiedTouchdown;

		LightningStateMachine(
			LightningType type,
			float personalitySeed,
			GameWallClock::time_point startTimestamp,
			std::optional<float> ndcX,
			std::optional<vec2f> targetWorldPosition)
			: Type(type)
			, PersonalitySeed(personalitySeed)
			, StartTimestamp(startTimestamp)
			, NdcX(ndcX)
			, TargetWorldPosition(targetWorldPosition)
			, Progress(0.0f)
			, RenderProgress(0.0f)
			, HasNotifiedTouchdown(false)
		{}
	};

    //
    // Storm state machine
    //

	// The storm output
	Parameters mParameters;

	// The timestamp at which we'll start the next storm, or not
	// set when we are in a storm
	std::optional<GameWallClock::time_point> mNextStormTimestamp;

    // The current progress of the storm, when in a storm: [0.0, 1.0]
    float mCurrentStormProgress;

	// The timestamp at which we last did a storm update
	GameWallClock::time_point mLastStormUpdateTimestamp;

	// Pre-calculated coefficients
	float mMaxWindSpeed;
	float mMaxRainDensity;
	float mMaxDarkening;

	// The CDF's for thunders
	float mThunderCdf;
	GameWallClock::time_point mNextThunderPoissonSampleTimestamp;

	// The CDF's for lightnings
	float mBackgroundLightningCdf;
	float mForegroundLightningCdf;
	GameWallClock::time_point mNextBackgroundLightningPoissonSampleTimestamp;
	GameWallClock::time_point mNextForegroundLightningPoissonSampleTimestamp;

	// The current lightnings' state machines
	std::list<LightningStateMachine> mLightnings;

	// Parameters that the calculated values are current with
	std::chrono::minutes mCurrentStormRate;
	float mCurrentStormStrengthAdjustment;
	float mCurrentLightningBlastProbability;
};

}
