/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-06-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {


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

void Bombs::Upload(
    int shipId,
    RenderContext & renderContext) const
{
    renderContext.UploadShipElementBombsStart(
        shipId,
        mCurrentBombs.size());

    for (auto & bomb : mCurrentBombs)
    {
        bomb->Upload(shipId, renderContext);
    }

    renderContext.UploadShipElementBombsEnd(shipId);
}

}