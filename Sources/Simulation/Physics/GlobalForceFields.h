/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-01-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Render/RenderContext.h>

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace Physics
{

class GlobalForceFields final
{
public:

    GlobalForceFields();

    void Update(Ship & ship)
    {
        // Anti-gravity fields
        for (auto & antiGravityField : mAntiGravityFields)
        {
            if (antiGravityField.StartEffectSimulationTime.has_value())
            {
                InternalUpdate(antiGravityField, ship);
            }
        }
    }

    void Upload(RenderContext & renderContext) const;

    // Interactions

    ElementIndex BeginPlaceAntiGravityField(
        vec2f const & startPos,
        float searchRadius);

    void UpdatePlaceAntiGravityField(
        ElementIndex antiGravityFieldId,
        vec2f const & endPos);

    void EndPlaceAntiGravityField(
        ElementIndex antiGravityFieldId,
        vec2f const & endPos,
        float searchRadius,
        float strengthMultiplier,
        float currentSimulationTime);

    void AbortPlaceAntiGravityField(ElementIndex antiGravityFieldId);

    void BoostAntiGravityFields(float strengthMultiplier);

private:

    bool mutable mIsDirtyForRendering;

    // Anti-Gravity field

    struct AntiGravityField
    {
        ElementIndex Id;
        vec2f StartPosition;
        vec2f EndPosition;
        std::optional<float> StartEffectSimulationTime; // When set, it's active
        float StrengthMultiplier; // Continuously decays to 1.0f

        AntiGravityField(
            ElementIndex id,
            vec2f const & startPosition)
            : Id(id)
            , StartPosition(startPosition)
            , EndPosition(startPosition)
            , StartEffectSimulationTime()
            , StrengthMultiplier(1.0f)
        { }
    };

    std::vector<AntiGravityField> mAntiGravityFields;

    void InternalUpdate(AntiGravityField & field, Ship & ship);

    template<typename T>
    auto FindFieldById(ElementIndex id, std::vector<T> & fields)
    {
        return std::find_if(
            fields.begin(),
            fields.end(),
            [id](T & field)
            {
                return field.Id == id;
            });
    }
};

}
