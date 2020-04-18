/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Bombs::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    // Run through all bombs and invoke Update() on each;
    // remove those bombs that have expired
    for (auto it = mCurrentBombs.begin(); it != mCurrentBombs.end(); /* incremented in loop */)
    {
        bool const isActive = (*it)->Update(
            currentWallClockTime,
            currentSimulationTime,
            stormParameters,
            gameParameters);

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

void Bombs::OnPointDetached(ElementIndex pointElementIndex)
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

void Bombs::DetonateAntiMatterBombs()
{
    for (auto & bomb : mCurrentBombs)
    {
        if (BombType::AntiMatterBomb == bomb->GetType())
        {
            AntiMatterBomb * amb = dynamic_cast<AntiMatterBomb *>(bomb.get());
            amb->Detonate();
        }
    }
}

void Bombs::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (auto & bomb : mCurrentBombs)
    {
        bomb->Upload(shipId, renderContext);
    }
}

}