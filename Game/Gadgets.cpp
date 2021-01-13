/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Gadgets::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    // Run through all gadgets and invoke Update() on each;
    // remove those gadgets that have expired
    for (auto it = mCurrentGadgets.begin(); it != mCurrentGadgets.end(); /* incremented in loop */)
    {
        bool const isActive = (*it)->Update(
            currentWallClockTime,
            currentSimulationTime,
            stormParameters,
            gameParameters);

        if (!isActive)
        {
            //
            // Gadget has expired
            //

            // Gadget has detached itself already
            assert(!(*it)->GetAttachedSpringIndex());

            // Notify (soundless) removal
            mGameEventHandler->OnGadgetRemoved(
                (*it)->GetId(),
                (*it)->GetType(),
                std::nullopt);

            // Remove it from the container
            it = mCurrentGadgets.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Gadgets::OnPointDetached(ElementIndex pointElementIndex)
{
    auto squareNeighborhoodRadius = NeighborhoodRadius * NeighborhoodRadius;

    auto neighborhoodCenter = mShipPoints.GetPosition(pointElementIndex);

    for (auto & gadget : mCurrentGadgets)
    {
        // Check if the gadget is within the neighborhood of the disturbed point
        float squareGadgetDistance = (gadget->GetPosition() - neighborhoodCenter).squareLength();
        if (squareGadgetDistance < squareNeighborhoodRadius)
        {
            // Tel the gadget that its neighborhood has been disturbed
            gadget->OnNeighborhoodDisturbed();
        }
    }
}

void Gadgets::OnSpringDestroyed(ElementIndex springElementIndex)
{
    auto squareNeighborhoodRadius = NeighborhoodRadius * NeighborhoodRadius;

    auto neighborhoodCenter = mShipSprings.GetMidpointPosition(springElementIndex, mShipPoints);

    for (auto & gadget : mCurrentGadgets)
    {
        // Check if the gadget is attached to this spring
        auto gadgetSpring = gadget->GetAttachedSpringIndex();
        if (!!gadgetSpring && *gadgetSpring == springElementIndex)
        {
            // Detach gadget
            gadget->DetachIfAttached();
        }

        // Check if the gadget is within the neighborhood of the disturbed center
        float squareGadgetDistance = (gadget->GetPosition() - neighborhoodCenter).squareLength();
        if (squareGadgetDistance < squareNeighborhoodRadius)
        {
            // Tel the gadget that its neighborhood has been disturbed
            gadget->OnNeighborhoodDisturbed();
        }
    }
}

void Gadgets::DetonateRCBombs()
{
    for (auto & gadget : mCurrentGadgets)
    {
        if (GadgetType::RCBomb == gadget->GetType())
        {
            RCBomb * rcb = dynamic_cast<RCBomb *>(gadget.get());
            rcb->Detonate();
        }
    }
}

void Gadgets::DetonateAntiMatterBombs()
{
    for (auto & gadget : mCurrentGadgets)
    {
        if (GadgetType::AntiMatterBomb == gadget->GetType())
        {
            AntiMatterBomb * amb = dynamic_cast<AntiMatterBomb *>(gadget.get());
            amb->Detonate();
        }
    }
}

void Gadgets::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (auto & gadget : mCurrentGadgets)
    {
        gadget->Upload(shipId, renderContext);
    }
}

}