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

class GameController::TsunamiNotificationStateMachine
{
public:

    TsunamiNotificationStateMachine(std::shared_ptr<Render::RenderContext> renderContext);

    ~TsunamiNotificationStateMachine();

    /*
     * When returns false, the state machine is over.
     */
    bool Update();

private:

    std::shared_ptr<Render::RenderContext> mRenderContext;
    RenderedTextHandle mTextHandle;

    enum class StateType
    {
        RumblingFadeIn,
        Rumbling1,
        WarningFadeIn,
        Warning,
        WarningFadeOut,
        Rumbling2,
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
    std::shared_ptr<Render::RenderContext> renderContext)
    : mRenderContext(std::move(renderContext))
    , mCurrentState(StateType::RumblingFadeIn)
    , mCurrentStateStartTime(GameWallClock::GetInstance().NowAsFloat())
{
    // Create text
    mTextHandle = mRenderContext->AddText(
        { "TSUNAMI WARNING!" },
        TextPositionType::TopRight,
        0.0f,
        FontType::GameText);
}

GameController::TsunamiNotificationStateMachine::~TsunamiNotificationStateMachine()
{
    mRenderContext->ResetPixelOffset();

    assert(NoneRenderedTextHandle != mTextHandle);
    mRenderContext->ClearText(mTextHandle);
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
                mCurrentState = StateType::WarningFadeIn;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::WarningFadeIn:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // Fade-in text
            mRenderContext->UpdateText(
                mTextHandle,
                std::min(progress, 1.0f));

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::Warning;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::Warning:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 5s);

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
                mCurrentState = StateType::WarningFadeOut;
                mCurrentStateStartTime = now;
            }

            return true;
        }

        case StateType::WarningFadeOut:
        {
            auto const progress = GameWallClock::Progress(now, mCurrentStateStartTime, 500ms);

            // Fade-out text
            mRenderContext->UpdateText(
                mTextHandle,
                1.0f - std::min(progress, 1.0f));

            // Set tremor
            mRenderContext->SetPixelOffset(TremorAmplitude * sin(TremorAngularVelocity * now), 0.0f);

            // See if time to transition
            if (progress >= 1.0f)
            {
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
    mTsunamiNotificationStateMachine.reset(new TsunamiNotificationStateMachine(mRenderContext));
}

////////////////////////////////////////////////////////////////////////
// Thanos Snap
////////////////////////////////////////////////////////////////////////

class GameController::ThanosSnapStateMachine
{};

void GameController::ThanosSnapStateMachineDeleter::operator()(ThanosSnapStateMachine * ptr) const
{
    delete ptr;
}

void GameController::StartThanosSnapStateMachine(float x)
{
    // TODO
}

////////////////////////////////////////////////////////////////////////
// All state machines
////////////////////////////////////////////////////////////////////////

void GameController::UpdateStateMachines()
{
    if (!!mTsunamiNotificationStateMachine)
    {
        if (!mTsunamiNotificationStateMachine->Update())
            mTsunamiNotificationStateMachine.reset();
    }
}

void GameController::ResetStateMachines()
{
    mTsunamiNotificationStateMachine.reset();

    mThanosSnapStateMachines.clear();
}