/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Bombs::Update(GameParameters const & gameParameters)
{
    auto now = GameWallClock::GetInstance().Now();

    // Run through all bombs and invoke Update() on each;
    // remove those bombs that have expired
    for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); /* incremented in loop */)
    {
        bool isActive = (*it)->Update(now, gameParameters);
        if (!isActive)
        {
            //
            // Bomb has expired
            //

            // Bomb has detached itself already
            assert(!(*it)->GetAttachedSpringIndex());

            // Notify (soundless) removal
            mGameEventHandler->OnBombRemoved(
                (*it)->GetId(),
                (*it)->GetType(),
                std::nullopt);

            // Remove it from the container
            it = mCurrentBombs.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Bombs::OnPointDestroyed(ElementIndex pointElementIndex)
{
    auto squareNeighborhoodRadius = GameParameters::BombNeighborhoodRadius * GameParameters::BombNeighborhoodRadius;

    auto neighborhoodCenter = mShipPoints.GetPosition(pointElementIndex);

    for (auto & bomb : mCurrentBombs)
    {
        // Check if the bomb is within the neighborhood of the disturbed point
        float squareBombDistance = (bomb->GetPosition() - neighborhoodCenter).squareLength();
        if (squareBombDistance < squareNeighborhoodRadius)
        {
            // Tel the bomb that its neighborhood has been disturbed
            bomb->OnNeighborhoodDisturbed();
        }
    }
}

void Bombs::OnSpringDestroyed(ElementIndex springElementIndex)
{
    auto squareNeighborhoodRadius = GameParameters::BombNeighborhoodRadius * GameParameters::BombNeighborhoodRadius;

    auto neighborhoodCenter = mShipSprings.GetMidpointPosition(springElementIndex, mShipPoints);

    for (auto & bomb : mCurrentBombs)
    {
        // Check if the bomb is attached to this spring
        auto bombSpring = bomb->GetAttachedSpringIndex();
        if (!!bombSpring && *bombSpring == springElementIndex)
        {
            // Detach bomb
            bomb->DetachIfAttached();
        }

        // Check if the bomb is within the neighborhood of the disturbed center
        float squareBombDistance = (bomb->GetPosition() - neighborhoodCenter).squareLength();
        if (squareBombDistance < squareNeighborhoodRadius)
        {
            // Tel the bomb that its neighborhood has been disturbed
            bomb->OnNeighborhoodDisturbed();
        }
    }
}

void Bombs::DetonateRCBombs()
{
    for (auto & bomb : mCurrentBombs)
    {
        if (BombType::RCBomb == bomb->GetType())
        {
            RCBomb * rcb = dynamic_cast<RCBomb *>(bomb.get());
            rcb->Detonate();
        }
    }
}

void Bombs::Upload(
    int shipId,
    Render::RenderContext & renderContext) const
{
    for (auto & bomb : mCurrentBombs)
    {
        bomb->Upload(shipId, renderContext);
    }
}

}