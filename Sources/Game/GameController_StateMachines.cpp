/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-07-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include <Core/GameMath.h>

#include <cassert>
#include <cmath>

////////////////////////////////////////////////////////////////////////
// Tsunami Notifications
////////////////////////////////////////////////////////////////////////

struct GameController::TsunamiNotificationStateMachine
{
    TsunamiNotificationStateMachine(
		std::shared_ptr<RenderContext> renderContext,
		NotificationLayer & notificationLayer);

    ~TsunamiNotificationStateMachine();

    /*
     * When returns false, the state machine is over.
     */
    bool Update();

private:

    std::shared_ptr<RenderContext> mRenderContext;
    NotificationLayer & mNotificationLayer;

    enum class StateType
    {
        RumblingFadeIn,
        Rumbling1,
        Rumbling2, // Warning comes out here
        RumblingFadeOut
    };

    StateType mCurrentState;
    float mCurrentStateStartTime;
};

void GameController::TsunamiNotificationStateMachineDeleter::operator()(TsunamiNotificationStateMachine * ptr) const
{
    delete ptr;
}

GameController::TsunamiNotificationStateMachine::TsunamiNotificationStateMachine(
    std::shared_ptr<RenderContext> renderContext,
    NotificationLayer & notificationLayer)
    : mRenderContext(std::move(renderContext))
	, mNotificationLayer(notificationLayer)
    , mCurrentState(StateType::RumblingFadeIn)
    , mCurrentStateStartTime(GameWallClock::GetInstance().NowAsFloat())
{
}

GameController::TsunamiNotificationStateMachine::~TsunamiNotificationStateMachine()
{
    mRenderContext->ResetPixelOffset();
}

bool GameController::TsunamiNotificationStateMachine::Update()
{
    float constexpr TremorAmplitude = 5.0f;
    float constexpr TremorAngularVelocity = 2.0f * Pi<float> * 6.0f;

    float const now = GameWallClock::GetInstance().NowAsFloat();

    switch (mCurrentState)
    {
        case StateType::RumblingFadeIn:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 1s);

            // Set tremor
            mRenderContext->SetPixelOffset(progress * TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::Rumbling1;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Rumbling1:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 4500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
				// Send warning
				mNotificationLayer.PublishNotificationText("TSUNAMI WARNING!", 5s);

				// Transition
                mCurrentState = StateType::Rumbling2;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Rumbling2:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::RumblingFadeOut;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::RumblingFadeOut:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 2s);

            // Set tremor
            mRenderContext->SetPixelOffset((1.0f - progress) * TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                // We're done
                return false;
            }

            return true;
        }
    }

    assert(false);
    return true;
}

void GameController::StartTsunamiNotificationStateMachine(float x)
{
    // Fire notification event
    mGameEventDispatcher.OnTsunamiNotification(x);

    // Start state machine
    mTsunamiNotificationStateMachine.reset(
		new TsunamiNotificationStateMachine(
			mRenderContext,
			mNotificationLayer));
}

////////////////////////////////////////////////////////////////////////
// Thanos Snap
////////////////////////////////////////////////////////////////////////

struct GameController::ThanosSnapStateMachine
{
    float const CenterX;
    bool const IsSparseMode;
    float const StartSimulationTimestamp;

    ThanosSnapStateMachine(
        float centerX,
        bool isSparseMode,
        float startSimulationTimestamp)
        : CenterX(centerX)
        , IsSparseMode(isSparseMode)
        , StartSimulationTimestamp(startSimulationTimestamp)
    {}
};

void GameController::ThanosSnapStateMachineDeleter::operator()(ThanosSnapStateMachine * ptr) const
{
    delete ptr;
}

void GameController::StartThanosSnapStateMachine(
    float x,
    bool isSparseMode,
    float currentSimulationTime)
{
    if (mThanosSnapStateMachines.empty())
    {
        //
        // First Thanos snap
        //

        // Start silence
        mGameEventDispatcher.OnSilenceStarted();

        // Silence world
        mWorld->SetSilence(1.0f);
    }
    else
    {
        // If full, make room for this latest arrival
        if (mThanosSnapStateMachines.size() == SimulationParameters::MaxThanosSnaps)
            mThanosSnapStateMachines.erase(mThanosSnapStateMachines.cbegin());
    }

    //
    // Start new state machine
    //

    mThanosSnapStateMachines.emplace_back(
        new ThanosSnapStateMachine(
            x,
            isSparseMode,
            currentSimulationTime));
}

bool GameController::UpdateThanosSnapStateMachine(
    ThanosSnapStateMachine & stateMachine,
    float currentSimulationTime)
{
    //
    // Calculate new radius
    //

    float constexpr AdvancingWaveSpeed = 25.0f; // m/s
    float constexpr SliceWidth = AdvancingWaveSpeed * SimulationParameters::SimulationStepTimeDuration<float>;

    float const radius =
        (currentSimulationTime - stateMachine.StartSimulationTimestamp)
        * AdvancingWaveSpeed;

    //
    // Apply Thanos Destructive Wave to both sides
    //

    bool hasAppliedWave = false;

    float const leftOuterEdgeX = stateMachine.CenterX - radius;
    float const leftInnerEdgeX = leftOuterEdgeX + SliceWidth / 2.0f;
    if (leftInnerEdgeX > -SimulationParameters::HalfMaxWorldWidth)
    {
        mWorld->ApplyThanosSnap(
            stateMachine.CenterX,
            radius,
            leftOuterEdgeX,
            leftInnerEdgeX,
            stateMachine.IsSparseMode,
            mSimulationParameters);

        hasAppliedWave = true;
    }

    float const rightOuterEdgeX = stateMachine.CenterX + radius;
    float const rightInnerEdgeX = rightOuterEdgeX - SliceWidth / 2.0f;
    if (rightInnerEdgeX < SimulationParameters::HalfMaxWorldWidth)
    {
        mWorld->ApplyThanosSnap(
            stateMachine.CenterX,
            radius,
            rightInnerEdgeX,
            rightOuterEdgeX,
            stateMachine.IsSparseMode,
            mSimulationParameters);

        hasAppliedWave = true;
    }

    return hasAppliedWave;
}

////////////////////////////////////////////////////////////////////////
// Daylight cycle
////////////////////////////////////////////////////////////////////////

struct GameController::DayLightCycleStateMachine
{
    DayLightCycleStateMachine(
        SimulationParameters const & gameParameters,
        IGameControllerSettings & gameControllerSettings);

    ~DayLightCycleStateMachine();

    /*
     * When returns false, the state machine is over.
     */
    void Update();

private:

    SimulationParameters const & mSimulationParameters;
    IGameControllerSettings & mGameControllerSettings;

    enum class StateType
    {
        SunRising,
        SunSetting
    };

    StateType mCurrentState;
    GameWallClock::float_time mLastChangeTimestamp;
    int mSkipCounter;
};

void GameController::DayLightCycleStateMachineDeleter::operator()(DayLightCycleStateMachine * ptr) const
{
    delete ptr;
}

GameController::DayLightCycleStateMachine::DayLightCycleStateMachine(
    SimulationParameters const & gameParameters,
    IGameControllerSettings & gameControllerSettings)
    : mSimulationParameters(gameParameters)
    , mGameControllerSettings(gameControllerSettings)
    , mCurrentState(StateType::SunSetting)
    , mLastChangeTimestamp(GameWallClock::GetInstance().NowAsFloat())
    , mSkipCounter(0)
{
}

GameController::DayLightCycleStateMachine::~DayLightCycleStateMachine()
{
}

void GameController::DayLightCycleStateMachine::Update()
{
    // We don't want to run at each and every frame
    ++mSkipCounter;
    if (mSkipCounter < 4) // Magic number
        return;

    mSkipCounter = 0;

    // We are stateless wrt time of day: we check each time where
    // we are at and compute the next step, based exclusively on the current
    // rising/setting state. This allows the user to change the current
    // time of day concurrently to this state machine.

    float newTimeOfDay = mGameControllerSettings.GetTimeOfDay();

    // Calculate fraction of half-cycle elapsed since last time
    auto const now = GameWallClock::GetInstance().NowAsFloat();
    auto const elapsedFraction = GameWallClock::Progress(
        now,
        mLastChangeTimestamp,
        mSimulationParameters.DayLightCycleDuration)
        * 2.0f;

    // Calculate new time of day
    if (StateType::SunRising == mCurrentState)
    {
        newTimeOfDay += elapsedFraction;
        if (newTimeOfDay >= 1.0f)
        {
            // Climax
            newTimeOfDay = 1.0f;
            mCurrentState = StateType::SunSetting;
        }
    }
    else
    {
        assert(StateType::SunSetting == mCurrentState);

        newTimeOfDay -= elapsedFraction;
        if (newTimeOfDay <= 0.0f)
        {
            // Anticlimax
            newTimeOfDay = 0.0f;
            mCurrentState = StateType::SunRising;
        }
    }

    // Change time-of-day
    mGameControllerSettings.SetTimeOfDay(newTimeOfDay);

    // Update last change timestamp
    mLastChangeTimestamp = now;
}

void GameController::StartDayLightCycleStateMachine()
{
    if (!mDayLightCycleStateMachine)
    {
        // Start state machine
        mDayLightCycleStateMachine.reset(
            new DayLightCycleStateMachine(
                mSimulationParameters,
                *this));

        mNotificationLayer.SetDayLightCycleIndicator(true);
    }
}

void GameController::StopDayLightCycleStateMachine()
{
    if (!!mDayLightCycleStateMachine)
    {
        mDayLightCycleStateMachine.reset();

        mNotificationLayer.SetDayLightCycleIndicator(false);
    }
}

////////////////////////////////////////////////////////////////////////
// All state machines
////////////////////////////////////////////////////////////////////////

void GameController::UpdateAllStateMachines(float currentSimulationTime)
{
    // Tsunami notifications
    if (!!mTsunamiNotificationStateMachine)
    {
        if (!mTsunamiNotificationStateMachine->Update())
            mTsunamiNotificationStateMachine.reset();
    }

    // Thanos' snap
    for (auto it = mThanosSnapStateMachines.begin(); it != mThanosSnapStateMachines.end(); /* incremented in loop */)
    {
        if (!UpdateThanosSnapStateMachine(*(it->get()), currentSimulationTime))
        {
            it = mThanosSnapStateMachines.erase(it);

            // If this was the last one, lift silence
            if (mThanosSnapStateMachines.empty())
            {
                // Lift silence on world
                mWorld->SetSilence(0.0f);

                // Lift silence
                mGameEventDispatcher.OnSilenceLifted();
            }
        }
        else
            ++it;
    }

    // Daylight cycle
    if (!!mDayLightCycleStateMachine)
    {
        mDayLightCycleStateMachine->Update();
    }
}

void GameController::ResetAllStateMachines()
{
    mTsunamiNotificationStateMachine.reset();

    mThanosSnapStateMachines.clear();

    // Nothing to do for daylight cycle state machine
}