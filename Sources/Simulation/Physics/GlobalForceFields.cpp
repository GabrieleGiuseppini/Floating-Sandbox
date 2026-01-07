/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-01-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include "../SimulationParameters.h"

#include <cassert>

namespace Physics {

GlobalForceFields::GlobalForceFields()
    : mIsDirtyForRendering(true) // Force first upload
    , mAntiGravityFields()
{
}

void GlobalForceFields::Upload(RenderContext & renderContext) const
{
    if (mIsDirtyForRendering)
    {
        renderContext.UploadAntiGravityFieldsStart();

        for (auto & antiGravityField : mAntiGravityFields)
        {
            renderContext.UploadAntiGravityField(antiGravityField.StartPosition, antiGravityField.EndPosition);
        }

        renderContext.UploadAntiGravityFieldsEnd();

        mIsDirtyForRendering = false;
    }
}

ElementIndex GlobalForceFields::BeginPlaceAntiGravityField(
    vec2f const & startPos,
    float searchRadius)
{
    // Search if there is one in radius
    for (auto antiGravityFieldSrchIt = mAntiGravityFields.begin(); antiGravityFieldSrchIt != mAntiGravityFields.end(); ++antiGravityFieldSrchIt)
    {
        assert(antiGravityFieldSrchIt->StartEffectSimulationTime.has_value());

        if ((antiGravityFieldSrchIt->StartPosition - startPos).length() <= searchRadius
            || (antiGravityFieldSrchIt->EndPosition - startPos).length() <= searchRadius)
        {
            // Nuke it
            mAntiGravityFields.erase(antiGravityFieldSrchIt);
            break;
        }
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

    mIsDirtyForRendering = true;

    return newFieldId;
}

void GlobalForceFields::UpdatePlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    vec2f const & endPos)
{
    auto fieldSearchIt = FindFieldById(antiGravityFieldId, mAntiGravityFields);
    assert(fieldSearchIt != mAntiGravityFields.end());

    assert(!fieldSearchIt->StartEffectSimulationTime.has_value());

    fieldSearchIt->EndPosition = endPos;

    mIsDirtyForRendering = true;
}

void GlobalForceFields::EndPlaceAntiGravityField(
    ElementIndex antiGravityFieldId,
    vec2f const & endPos,
    float searchRadius,
    float strengthMultiplier,
    float currentSimulationTime)
{
    auto fieldSearchIt = FindFieldById(antiGravityFieldId, mAntiGravityFields);
    assert(fieldSearchIt != mAntiGravityFields.end());

    assert(!fieldSearchIt->StartEffectSimulationTime.has_value());

    // Check if this was a "delete by staying very close to start position"
    if ((fieldSearchIt->StartPosition - endPos).length() < searchRadius / 4.0f)
    {
        // Abort it
        mAntiGravityFields.erase(fieldSearchIt);
    }
    else
    {
        // Activate it
        fieldSearchIt->StartEffectSimulationTime = currentSimulationTime;
        fieldSearchIt->StrengthMultiplier = strengthMultiplier;
    }

    mIsDirtyForRendering = true;
}

void GlobalForceFields::AbortPlaceAntiGravityField(ElementIndex antiGravityFieldId)
{
    auto fieldSearchIt = FindFieldById(antiGravityFieldId, mAntiGravityFields);
    assert(fieldSearchIt != mAntiGravityFields.end());

    assert(!fieldSearchIt->StartEffectSimulationTime.has_value());

    // Abort it
    mAntiGravityFields.erase(fieldSearchIt);

    mIsDirtyForRendering = true;
}

void GlobalForceFields::BoostAntiGravityFields(float strengthMultiplier)
{
    for (auto & antiGravityField : mAntiGravityFields)
    {
        if (antiGravityField.StartEffectSimulationTime.has_value())
        {
            antiGravityField.StrengthMultiplier = strengthMultiplier;
        }
    }
}

/////////////////////////////////////////////////////////////

void GlobalForceFields::InternalUpdate(
    AntiGravityField & field,
    Ship & ship)
{
    ship.ApplyAntiGravityField(field.StartPosition, field.EndPosition, field.StrengthMultiplier);

    // Decay strength multiplier
    field.StrengthMultiplier += (1.0f - field.StrengthMultiplier) * 0.05f;
}

}