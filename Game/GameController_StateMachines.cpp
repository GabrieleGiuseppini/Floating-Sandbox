/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-07-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GameController.h"

#include <cassert>

////////////////////////////////////////////////////////////////////////
// Tsunami Notifications
////////////////////////////////////////////////////////////////////////

struct GameController::TsunamiNotificationStateMachine
{
    TsunamiNotificationStateMachine(
		std::shared_ptr<Render::RenderContext> renderContext,
		std::shared_ptr<TextLayer> textLayer);

    ~TsunamiNotificationStateMachine();

    /*
     * When returns false, the state machine is over.
     */
    bool Update();

private:

    std::shared_ptr<Render::RenderContext> mRenderContext;
	std::shared_ptr<TextLayer> mTextLayer;

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
    std::shared_ptr<Render::RenderContext> renderContext,
	std::shared_ptr<TextLayer> textLayer)
    : mRenderContext(std::move(renderContext))
	, mTextLayer(std::move(textLayer))
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
				mTextLayer->AddEphemeralTextLine("TSUNAMI WARNING!", 5s);

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
    mGameEventDispatcher->OnTsunamiNotification(x);

    // Start state machine
    mTsunamiNotificationStateMachine.reset(
		new TsunamiNotificationStateMachine(
			mRenderContext,
			mTextLayer));
}

////////////////////////////////////////////////////////////////////////
// Thanos Snap
////////////////////////////////////////////////////////////////////////

struct GameController::ThanosSnapStateMachine
{
    float const CenterX;
    float const StartSimulationTimestamp;

    ThanosSnapStateMachine(
        float centerX,
        float startSimulationTimestamp)
        : CenterX(centerX)
        , StartSimulationTimestamp(startSimulationTimestamp)
    {}
};

void GameController::ThanosSnapStateMachineDeleter::operator()(ThanosSnapStateMachine * ptr) const
{
    delete ptr;
}

void GameController::StartThanosSnapStateMachine(
    float x,
    float currentSimulationTime)
{
    if (mThanosSnapStateMachines.empty())
    {
        //
        // First Thanos snap
        //

        // Start silence
        mGameEventDispatcher->OnSilenceStarted();

        // Silence world
        mWorld->SetSilence(1.0f);
    }
    else
    {
        // If full, make room for this latest arrival
        if (mThanosSnapStateMachines.size() == GameParameters::MaxThanosSnaps)
            mThanosSnapStateMachines.erase(mThanosSnapStateMachines.cbegin());
    }

    //
    // Start new state machine
    //

    mThanosSnapStateMachines.emplace_back(
        new ThanosSnapStateMachine(
            x,
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
    float constexpr SliceWidth = AdvancingWaveSpeed * GameParameters::SimulationStepTimeDuration<float>;

    float const radius =
        (currentSimulationTime - stateMachine.StartSimulationTimestamp)
        * AdvancingWaveSpeed;


    //
    // Apply Thanos Destructive Wave to both sides
    //

    bool hasAppliedWave = false;

    float const leftOuterEdgeX = stateMachine.CenterX - radius;
    float const leftInnerEdgeX = leftOuterEdgeX + SliceWidth / 2.0f;
    if (leftInnerEdgeX > -GameParameters::HalfMaxWorldWidth)
    {
        mWorld->ApplyThanosSnap(
            stateMachine.CenterX,
            radius,
            leftOuterEdgeX,
            leftInnerEdgeX,
            currentSimulationTime,
            mGameParameters);

        hasAppliedWave = true;
    }

    float const rightOuterEdgeX = stateMachine.CenterX + radius;
    float const rightInnerEdgeX = rightOuterEdgeX - SliceWidth / 2.0f;
    if (rightInnerEdgeX < GameParameters::HalfMaxWorldWidth)
    {
        mWorld->ApplyThanosSnap(
            stateMachine.CenterX,
            radius,
            rightInnerEdgeX,
            rightOuterEdgeX,
            currentSimulationTime,
            mGameParameters);

        hasAppliedWave = true;
    }

    return hasAppliedWave;
}

////////////////////////////////////////////////////////////////////////
// All state machines
////////////////////////////////////////////////////////////////////////

void GameController::UpdateStateMachines(float currentSimulationTime)
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
                mGameEventDispatcher->OnSilenceLifted();
            }
        }
        else
            ++it;
    }
}

void GameController::ResetStateMachines()
{
    mTsunamiNotificationStateMachine.reset();

    mThanosSnapStateMachines.clear();
}