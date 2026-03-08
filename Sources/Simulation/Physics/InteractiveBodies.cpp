/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-01-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "../SimulationParameters.h"

#include <Core/Conversions.h>

#include <cassert>

namespace Physics {

InteractiveBodies::InteractiveBodies(
    World & parentWorld,
    SimulationEventDispatcher & simulationEventDispatcher)
    : mParentWorld(parentWorld)
    , mSimulationEventHandler(simulationEventDispatcher)
    , mAntiGravityFields()
    , mAreAntiGravityFieldsDirtyForRendering(true) // Force first upload
    , mTornadoes()
    , mAreTornadoesDirtyForRendering(true) // Force first upload
{
}

void InteractiveBodies::Update(
    std::vector<std::unique_ptr<Ship>> const & ships,
    Npcs & npcs,
    OceanSurface & oceanSurface,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // Anti-gravity fields
    //

    for (auto & antiGravityField : mAntiGravityFields)
    {
        if (antiGravityField.StartEffectSimulationTime.has_value())
        {
            // Apply it

            for (auto & ship : ships)
            {
                assert(ship);
                ship->ApplyAntiGravityField(antiGravityField.StartPosition, antiGravityField.EndPosition, antiGravityField.StrengthMultiplier);
            }

            // TODO: NPCs?

            // Decay strength multiplier
            antiGravityField.StrengthMultiplier += (1.0f - antiGravityField.StrengthMultiplier) * 0.05f;
        }
    }

    //
    // Tornadoes
    //

    for (auto it = mTornadoes.begin(); it != mTornadoes.end(); /* incremented in loop */)
    {
        auto & tornado = *it;

        //
        // Update tornado
        //

        float constexpr ConvergenceThreshold = 0.008f;

        // Reclaculate target velocity depending on distance from target
        float const distanceToTarget = tornado.TargetX - tornado.CurrentX;
        tornado.TargetVelocityX = std::min(20.0f * std::fabsf(distanceToTarget), Conversions::KmhToMs(30.0f));
        if (distanceToTarget < 0.0f)
            tornado.TargetVelocityX *= -1.0f;

        // Converge current velocity to target velocity
        tornado.CurrentVelocityX += (tornado.TargetVelocityX - tornado.CurrentVelocityX) * 0.05f;
        if (std::fabsf(tornado.TargetVelocityX - tornado.CurrentVelocityX) < ConvergenceThreshold)
        {
            tornado.CurrentVelocityX = tornado.TargetVelocityX;
        }

        // Calculate projected position X
        float const newCurrentX = tornado.CurrentX + tornado.CurrentVelocityX * SimulationParameters::SimulationStepTimeDuration<float>;
        if (newCurrentX != tornado.CurrentX)
        {
            // Move to projected position X
            tornado.CurrentX = Clamp(newCurrentX, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth);

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
        }

        // Calculate new base Y
        float const newBaseY = oceanSurface.GetHeightAt(tornado.CurrentX);
        if (newBaseY != tornado.CurrentBaseY)
        {
            // Converge towards new base Y
            tornado.CurrentBaseY += (newBaseY - tornado.CurrentBaseY) * 0.2f;

            // Does not count as activity
        }

        // Converge other quantities to their targets

        if (tornado.CurrentVisibilityAlpha != tornado.TargetVisibilityAlpha)
        {
            if (tornado.TargetVisibilityAlpha > tornado.CurrentVisibilityAlpha)
            {
                // Becoming visible

                // We like lingering at low alpha for long, jumping up to destination only late

                tornado.CurrentVisibilityAlpha += 0.015f * std::max(tornado.CurrentVisibilityAlpha, 0.015f);
                if (tornado.CurrentVisibilityAlpha > tornado.CurrentVisibilityAlpha)
                {
                    tornado.CurrentVisibilityAlpha = tornado.TargetVisibilityAlpha;
                }

                tornado.LastActivitySimulationTimestamp = currentSimulationTime; // Start counting idle from now
            }
            else
            {
                // Becoming invisible

                // We like the long tail

                tornado.CurrentVisibilityAlpha += (tornado.TargetVisibilityAlpha - tornado.CurrentVisibilityAlpha) * 0.01f;
                if (std::fabsf(tornado.TargetVisibilityAlpha - tornado.CurrentVisibilityAlpha) < ConvergenceThreshold)
                {
                    tornado.CurrentVisibilityAlpha = tornado.TargetVisibilityAlpha;
                }
            }
        }

        if (tornado.CurrentForceMultiplier != tornado.TargetForceMultiplier)
        {
            tornado.CurrentForceMultiplier += (tornado.TargetForceMultiplier - tornado.CurrentForceMultiplier) * 0.015f;
            if (std::fabsf(tornado.TargetForceMultiplier - tornado.CurrentForceMultiplier) < ConvergenceThreshold)
            {
                tornado.CurrentForceMultiplier = tornado.TargetForceMultiplier;
            }

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
        }

        if (tornado.CurrentHeatDepth != tornado.TargetHeatDepth)
        {
            tornado.CurrentHeatDepth += (tornado.TargetHeatDepth - tornado.CurrentHeatDepth) * 0.01f;
            if (std::fabsf(tornado.TargetHeatDepth - tornado.CurrentHeatDepth) < ConvergenceThreshold)
            {
                tornado.CurrentHeatDepth = tornado.TargetHeatDepth;
            }

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
        }

        //
        // Progress rotation
        //

        // Only proportional to Force/StrengthMultiplier
        // NOT proportional to viz, so it keeps rotating while it disappears
        tornado.CurrentRotationPhase += SimulationParameters::SimulationStepTimeDuration<float> * tornado.CurrentForceMultiplier;

        //
        // Check whether it's been idle enough to start disappearing
        //

        float constexpr IdleTimeoutSimSeconds = 35.0f;
        if (currentSimulationTime > tornado.LastActivitySimulationTimestamp + IdleTimeoutSimSeconds)
        {
            // Start disappearing
            tornado.TargetVisibilityAlpha = 0.0f; // Only place where we write it
        }

        // Check whether we're done disappearing
        if (tornado.CurrentVisibilityAlpha == tornado.TargetVisibilityAlpha && tornado.TargetVisibilityAlpha == 0.0f)
        {
            assert(currentSimulationTime > tornado.LastActivitySimulationTimestamp + IdleTimeoutSimSeconds); // Dangerous, might be in-use

            // Publish event to signal end of tornado
            mSimulationEventHandler.OnTornadoUpdated(0.0f, 0.0f, 0.0f);

            // Remove it
            it = mTornadoes.erase(it);
        }
        else
        {
            //
            // Apply tornado
            //

            vec2f const position = vec2f(tornado.CurrentX, tornado.CurrentBaseY);
            FloatSize const size = CalculateTornadoEffectiveSize(tornado.CurrentVisibilityAlpha);
            float const bottomWidthFraction = CalculateTornadoBottomWidthFraction(tornado.CurrentForceMultiplier);
            float const strengthMultiplier = tornado.CurrentForceMultiplier * tornado.CurrentVisibilityAlpha;

            // Ships
            for (auto & ship : ships)
            {
                assert(ship);
                ship->ApplyTornado(
                    position,
                    size,
                    bottomWidthFraction,
                    tornado.CurrentRotationPhase,
                    strengthMultiplier,
                    tornado.CurrentHeatDepth);
            }

            // Npcs
            npcs.ApplyTornado(
                position,
                size,
                bottomWidthFraction,
                tornado.CurrentRotationPhase,
                strengthMultiplier,
                tornado.CurrentHeatDepth,
                simulationParameters);

            // Ocean surface
            float constexpr OceanSurfaceDisplacement = 5.0f;
            oceanSurface.DisplaceAt(
                tornado.CurrentX,
                OceanSurfaceDisplacement * tornado.CurrentForceMultiplier * tornado.CurrentVisibilityAlpha);

            // Publish event
            mSimulationEventHandler.OnTornadoUpdated(
                tornado.CurrentVisibilityAlpha,
                tornado.CurrentForceMultiplier,
                tornado.CurrentHeatDepth);

            ++it;
        }

        // Unconditionally, a tornado has updated - or has been removed
        mAreTornadoesDirtyForRendering = true;
    }
}

void InteractiveBodies::Upload(RenderContext & renderContext) const
{
    if (mAreAntiGravityFieldsDirtyForRendering)
    {
        renderContext.UploadAntiGravityFieldsStart();

        for (auto const & antiGravityField : mAntiGravityFields)
        {
            renderContext.UploadAntiGravityField(antiGravityField.StartPosition, antiGravityField.EndPosition);
        }

        renderContext.UploadAntiGravityFieldsEnd();

        mAreAntiGravityFieldsDirtyForRendering = false;
    }

    if (mAreTornadoesDirtyForRendering)
    {
        renderContext.UploadTornadoesStart();

        for (auto const & tornado : mTornadoes)
        {
            renderContext.UploadTornado(
                vec2f(tornado.CurrentX, tornado.CurrentBaseY),
                CalculateTornadoEffectiveSize(tornado.CurrentVisibilityAlpha),
                CalculateTornadoBottomWidthFraction(tornado.CurrentForceMultiplier),
                tornado.CurrentForceMultiplier * tornado.CurrentVisibilityAlpha,
                tornado.CurrentHeatDepth,
                tornado.CurrentVisibilityAlpha,
                tornado.CurrentRotationPhase);
        }

        renderContext.UploadTornadoesEnd();

        mAreTornadoesDirtyForRendering = false;
    }
}

ElementIndex InteractiveBodies::BeginPlaceAntiGravityField(
    vec2f const & startPos,
    float searchRadius)
{
    // Search if there is one in radius, and pick the nearest
    ElementIndex nearestAntiGravityField = NoneElementIndex;
    float nearestDistance = std::numeric_limits<float>::max();
    for (size_t i = 0; i < mAntiGravityFields.size(); ++i)
    {
        assert(mAntiGravityFields[i].StartEffectSimulationTime.has_value());

        float const startDistance = (mAntiGravityFields[i].StartPosition - startPos).length();
        if (startDistance <= searchRadius && startDistance < nearestDistance)
        {
            nearestAntiGravityField = static_cast<ElementIndex>(i);
            nearestDistance = startDistance;
        }

        float const endDistance = (mAntiGravityFields[i].EndPosition - startPos).length();
        if (endDistance <= searchRadius && endDistance < nearestDistance)
        {
            nearestAntiGravityField = static_cast<ElementIndex>(i);
            nearestDistance = endDistance;
        }
    }

    if (nearestAntiGravityField != NoneElementIndex)
    {
        // Nuke it
        mAntiGravityFields.erase(mAntiGravityFields.begin() + nearestAntiGravityField);
    }

    // Create new one
    ElementIndex newFieldId = NoneElementIndex;

    // See if there's room for another one
    if (mAntiGravityFields.size() < SimulationParameters::MaxAntiGravityForceFields)
    {
        // Calc ID
        auto maxIdSrchIt = std::max_element(
            mAntiGravityFields.cbegin(),
            mAntiGravityFields.cend(),
            [](AntiGravityField const & f1, AntiGravityField const & f2)
            {
                return f1.Id < f2.Id;
            });
        newFieldId = (maxIdSrchIt == mAntiGravityFields.cend())
            ? 0
            : maxIdSrchIt->Id + 1;
    }
    else
    {
        // Kill the oldest one and reuse its ID
        auto oldestSrchIt = std::min_element(
            mAntiGravityFields.cbegin(),
            mAntiGravityFields.cend(),
            [](AntiGravityField const & f1, AntiGravityField const & f2)
            {
                assert(f1.StartEffectSimulationTime.has_value());
                assert(f2.StartEffectSimulationTime.has_value());

                return f1.StartEffectSimulationTime < f2.StartEffectSimulationTime;
            });
        assert(oldestSrchIt != mAntiGravityFields.cend());
        newFieldId = oldestSrchIt->Id;
        mAntiGravityFields.erase(oldestSrchIt);
    }

    assert(newFieldId != NoneElementIndex);

    // Create it
    mAntiGravityFields.emplace_back(
        newFieldId,
        startPos);

    mAreAntiGravityFieldsDirtyForRendering = true;

    return newFieldId;
}

void InteractiveBodies::UpdatePlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    vec2f const & endPos)
{
    auto bodySearchIt = FindBodyById(antiGravityFieldId, mAntiGravityFields);
    assert(bodySearchIt != mAntiGravityFields.end());

    assert(!bodySearchIt->StartEffectSimulationTime.has_value());

    bodySearchIt->EndPosition = endPos;

    mAreAntiGravityFieldsDirtyForRendering = true;
}

void InteractiveBodies::EndPlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    vec2f const & endPos,
    float searchRadius,
    float strengthMultiplier,
    float currentSimulationTime)
{
    auto bodySearchIt = FindBodyById(antiGravityFieldId, mAntiGravityFields);
    assert(bodySearchIt != mAntiGravityFields.end());

    assert(!bodySearchIt->StartEffectSimulationTime.has_value());

    // Check if this was a "delete by staying very close to start position"
    if ((bodySearchIt->StartPosition - endPos).length() < searchRadius / 4.0f)
    {
        // Abort it
        mAntiGravityFields.erase(bodySearchIt);
    }
    else
    {
        // Activate it
        bodySearchIt->StartEffectSimulationTime = currentSimulationTime;
        bodySearchIt->StrengthMultiplier = strengthMultiplier;
    }

    mAreAntiGravityFieldsDirtyForRendering = true;
}

void InteractiveBodies::AbortPlaceAntiGravityField(ElementIndex antiGravityFieldId)
{
    auto bodySearchIt = FindBodyById(antiGravityFieldId, mAntiGravityFields);
    assert(bodySearchIt != mAntiGravityFields.end());

    assert(!bodySearchIt->StartEffectSimulationTime.has_value());

    // Abort it
    mAntiGravityFields.erase(bodySearchIt);

    mAreAntiGravityFieldsDirtyForRendering = true;
}

void InteractiveBodies::BoostAntiGravityFields(float strengthMultiplier)
{
    for (auto & antiGravityField : mAntiGravityFields)
    {
        if (antiGravityField.StartEffectSimulationTime.has_value())
        {
            antiGravityField.StrengthMultiplier = strengthMultiplier;
        }
    }
}

bool InteractiveBodies::RemoveAllAntyGravityFields()
{
    if (!mAntiGravityFields.empty())
    {
        mAntiGravityFields.clear();

        mAreAntiGravityFieldsDirtyForRendering = true;

        return true;
    }

    return false;
}

ElementIndex InteractiveBodies::BeginPlaceTornado(
    float posX,
    float searchRadius,
    OceanSurface const & oceanSurface,
    float currentSimulationTimestamp)
{
    posX = Clamp(posX, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth); // Safety

    std::optional<size_t> existingTornado;

    if constexpr (SimulationParameters::MaxTornadoes == 1)
    {
        // Pick the existing one, if one exists
        if (!mTornadoes.empty())
        {
            existingTornado = 0;
        }
    }
    else
    {
        // Search if there is one in radius, and pick the nearest
        float nearestDistance = std::numeric_limits<float>::max();
        for (size_t i = 0; i < mTornadoes.size(); ++i)
        {
            float const distance = std::fabs(mTornadoes[i].CurrentX - posX);
            float const effectiveTornadoRadius = CalculateTornadoEffectiveSize(mTornadoes[i].CurrentVisibilityAlpha).width / 2.0f;
            if (distance <= std::max(searchRadius, effectiveTornadoRadius) && distance < nearestDistance)
            {
                existingTornado = i;
                nearestDistance = distance;
            }
        }
    }

    ElementIndex newTornadoId;

    if (existingTornado.has_value())
    {
        // Refresh this one
        mTornadoes[*existingTornado].ResetToBegin(
            posX,
            oceanSurface.GetHeightAt(posX),
            currentSimulationTimestamp);

        newTornadoId = mTornadoes[*existingTornado].Id;
    }
    else
    {
        // Create new one

        // See if there's room for another one
        if (mTornadoes.size() < SimulationParameters::MaxTornadoes)
        {
            // Calc ID
            auto maxIdSrchIt = std::max_element(
                mTornadoes.cbegin(),
                mTornadoes.cend(),
                [](Tornado const & t1, Tornado const & t2)
                {
                    return t1.Id < t2.Id;
                });
            newTornadoId = (maxIdSrchIt == mTornadoes.cend())
                ? 0
                : maxIdSrchIt->Id + 1;
        }
        else
        {
            // Kill the oldest one and reuse its ID
            auto oldestSrchIt = std::min_element(
                mTornadoes.cbegin(),
                mTornadoes.cend(),
                [](Tornado const & t1, Tornado const & t2)
                {
                    return t1.StartSimulationTimestamp < t2.StartSimulationTimestamp;
                });
            assert(oldestSrchIt != mTornadoes.cend());
            newTornadoId = oldestSrchIt->Id;
            mTornadoes.erase(oldestSrchIt);
        }

        // Create it
        mTornadoes.emplace_back(
            newTornadoId,
            posX,
            oceanSurface.GetHeightAt(posX),
            currentSimulationTimestamp);
    }

    mAreTornadoesDirtyForRendering = true;

    return newTornadoId;
}

void InteractiveBodies::UpdateTornado(
    ElementIndex tornadoId,
    float posX,
    float strengthMultiplier,
    float heatDepth,
    float currentSimulationTimestamp)
{
    auto bodySearchIt = FindBodyById(tornadoId, mTornadoes);
    assert(bodySearchIt != mTornadoes.end());

    bodySearchIt->TargetX = posX;
    bodySearchIt->TargetForceMultiplier = strengthMultiplier;
    bodySearchIt->TargetHeatDepth = heatDepth;

    bodySearchIt->LastActivitySimulationTimestamp = currentSimulationTimestamp; // Prevent it from disappearing while selected
}

void InteractiveBodies::EndPlaceTornado(
    ElementIndex tornadoId,
    float currentSimulationTimestamp)
{
    auto bodySearchIt = FindBodyById(tornadoId, mTornadoes);
    assert(bodySearchIt != mTornadoes.end());

    // Not much to do, really

    bodySearchIt->LastActivitySimulationTimestamp = currentSimulationTimestamp;
}

//////////////////////////////////////////////////

FloatSize InteractiveBodies::CalculateTornadoEffectiveSize(float visibilityAlpha)
{
    return FloatSize(
        SimulationParameters::TornadoWidth * (0.5f + 0.5f * visibilityAlpha),
        SimulationParameters::TornadoHeight);
}

float InteractiveBodies::CalculateTornadoBottomWidthFraction(float strengthMultiplier)
{
    // Futurework?
    //return LinearStep(1.25f, 2.0f, strengthMultiplier) * 0.1f;
    (void)strengthMultiplier;
    return 0.0f;
}

}