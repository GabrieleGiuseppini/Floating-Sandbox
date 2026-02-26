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

InteractiveBodies::InteractiveBodies()
    : mAntiGravityFields()
    , mAreAntiGravityFieldsDirtyForRendering(true) // Force first upload
    , mTornadoes()
    , mAreTornadoesDirtyForRendering(true) // Force first upload
{
}

void InteractiveBodies::Update(
    std::vector<std::unique_ptr<Ship>> const & ships,
    Npcs & npcs,
    OceanSurface const & oceanSurface,
    float currentSimulationTime)
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

        float constexpr ConvergenceThreshold = 0.001f;

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
            tornado.CurrentX = newCurrentX;
            tornado.CurrentBaseY = CalculateTornadoBaseY(newCurrentX, oceanSurface);

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
            mAreTornadoesDirtyForRendering = true;
        }

        // Converge other quantities to their targets

        if (tornado.CurrentVisibilityAlpha != tornado.TargetVisibilityAlpha)
        {
            tornado.CurrentVisibilityAlpha += (tornado.TargetVisibilityAlpha - tornado.CurrentVisibilityAlpha) * 0.02f;
            if (std::fabsf(tornado.TargetVisibilityAlpha - tornado.CurrentVisibilityAlpha) < ConvergenceThreshold)
            {
                tornado.CurrentVisibilityAlpha = tornado.TargetVisibilityAlpha;
            }

            mAreTornadoesDirtyForRendering = true;
        }

        if (tornado.CurrentForceMultiplier != tornado.TargetForceMultiplier)
        {
            tornado.CurrentForceMultiplier += (tornado.TargetForceMultiplier - tornado.CurrentForceMultiplier) * 0.02f;
            if (std::fabsf(tornado.TargetForceMultiplier - tornado.CurrentForceMultiplier) < ConvergenceThreshold)
            {
                tornado.CurrentForceMultiplier = tornado.TargetForceMultiplier;
            }

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
            mAreTornadoesDirtyForRendering = true;
        }

        if (tornado.CurrentHeatDepth != tornado.TargetHeatDepth)
        {
            tornado.CurrentHeatDepth += (tornado.TargetHeatDepth - tornado.CurrentHeatDepth) * 0.02f;
            if (std::fabsf(tornado.TargetHeatDepth - tornado.CurrentHeatDepth) < ConvergenceThreshold)
            {
                tornado.CurrentHeatDepth = tornado.TargetHeatDepth;
            }

            tornado.LastActivitySimulationTimestamp = currentSimulationTime;
            mAreTornadoesDirtyForRendering = true;
        }

        //
        // Check whether it's been enough idle to start disappearing
        //

        float constexpr IdleTimeoutSimSeconds = 3.0f;
        if (currentSimulationTime > tornado.LastActivitySimulationTimestamp + IdleTimeoutSimSeconds)
        {
            // Start disappearing
            tornado.TargetVisibilityAlpha = 0.0f; // Only place where we write it
        }

        // Check whether we're done disappearing
        if (tornado.CurrentVisibilityAlpha == tornado.TargetVisibilityAlpha && tornado.TargetVisibilityAlpha == 0.0f)
        {
            assert(currentSimulationTime > tornado.LastActivitySimulationTimestamp + IdleTimeoutSimSeconds); // Dangerous, might be in-use

            // Remove it
            it = mTornadoes.erase(it);

            mAreTornadoesDirtyForRendering = true;
        }
        else
        {
            //
            // Apply tornado
            //

            // TODOHERE
            (void)npcs;

            ++it;
        }
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
            float const rotationSpeedMultiplier =
                1.0f
                * tornado.CurrentForceMultiplier
                * tornado.CurrentVisibilityAlpha;

            renderContext.UploadTornado(
                vec2f(tornado.CurrentX, tornado.CurrentBaseY),
                CalculateTornadoEffectiveSize(tornado.CurrentVisibilityAlpha),
                rotationSpeedMultiplier,
                tornado.CurrentHeatDepth,
                tornado.CurrentVisibilityAlpha);
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
    ElementIndex newTornadoId = NoneElementIndex;

    // Search if there is one in radius, and pick the nearest
    float nearestDistance = std::numeric_limits<float>::max();
    for (size_t i = 0; i < mTornadoes.size(); ++i)
    {
        float const distance = std::fabs(mTornadoes[i].CurrentX - posX);
        float const effectiveTornadoRadius = CalculateTornadoEffectiveSize(mTornadoes[i].CurrentVisibilityAlpha).width / 2.0f;
        if (distance <= std::max(searchRadius, effectiveTornadoRadius) && distance < nearestDistance)
        {
            newTornadoId = static_cast<ElementIndex>(i);
            nearestDistance = distance;
        }
    }

    if (newTornadoId != NoneElementIndex)
    {
        // Refresh this one
        mTornadoes[newTornadoId].ResetToBegin(
            posX,
            CalculateTornadoBaseY(posX, oceanSurface),
            currentSimulationTimestamp);
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

        assert(newTornadoId != NoneElementIndex);

        // Create it
        mTornadoes.emplace_back(
            newTornadoId,
            posX,
            CalculateTornadoBaseY(posX, oceanSurface),
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

float InteractiveBodies::CalculateTornadoBaseY(
    float posX,
    OceanSurface const & oceanSurface)
{
    return oceanSurface.GetHeightAt(Clamp(posX, -SimulationParameters::HalfMaxWorldWidth, SimulationParameters::HalfMaxWorldWidth));
}

FloatSize InteractiveBodies::CalculateTornadoEffectiveSize(float visibilityAlpha)
{
    return FloatSize(
        SimulationParameters::TornadoWidth * (0.2f + 0.8F * visibilityAlpha),
        SimulationParameters::TornadoHeight);
}

}