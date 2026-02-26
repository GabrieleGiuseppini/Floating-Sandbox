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

class InteractiveBodies final
{
public:

    InteractiveBodies();

    void Update(
        std::vector<std::unique_ptr<Ship>> const & ships,
        Npcs & npcs,
        OceanSurface const & oceanSurface,
        float currentSimulationTime);

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

    bool RemoveAllAntyGravityFields();

    ElementIndex BeginPlaceTornado(
        float posX,
        float searchRadius,
        OceanSurface const & oceanSurface,
        float currentSimulationTime);

    void UpdateTornado(
        ElementIndex tornadoId,
        float posX,
        float strengthMultiplier,
        float heatDepth,
        float currentSimulationTime);

    void EndPlaceTornado(
        ElementIndex tornadoId,
        float currentSimulationTime);

    ///

    static inline float CalculateTornadoBaseY(
        float posX,
        OceanSurface const & oceanSurface);

    static inline FloatSize CalculateTornadoEffectiveSize(float visibilityAlpha);

private:

    template<typename T>
    auto FindBodyById(ElementIndex id, std::vector<T> & bodies)
    {
        return std::find_if(
            bodies.begin(),
            bodies.end(),
            [id](T & body)
            {
                return body.Id == id;
            });
    }

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

    bool mutable mAreAntiGravityFieldsDirtyForRendering;

    // Tornado

    struct Tornado
    {
        ElementIndex Id;

        float CurrentX;
        float CurrentBaseY;
        float TargetX;

        float CurrentVelocityX;
        float TargetVelocityX;

        float CurrentVisibilityAlpha;
        float TargetVisibilityAlpha;

        float CurrentForceMultiplier;
        float TargetForceMultiplier;

        float CurrentHeatDepth;
        float TargetHeatDepth;

        float StartSimulationTimestamp;
        float LastActivitySimulationTimestamp; // To detect idle

        Tornado(
            ElementIndex id,
            float currentX,
            float currentBaseY,
            float currentSimulationTime)
            : Id(id)
            , CurrentX(currentX)
            , CurrentBaseY(currentBaseY)
            , TargetX(currentX)
            , CurrentVelocityX(0.0f)
            , TargetVelocityX(0.0f)
            , CurrentVisibilityAlpha(0.0f)
            , TargetVisibilityAlpha(1.0f) // Ready to see
            , CurrentForceMultiplier(1.0f)
            , TargetForceMultiplier(1.0f)
            , CurrentHeatDepth(0.0f)
            , TargetHeatDepth(0.0f)
            , StartSimulationTimestamp(currentSimulationTime)
            , LastActivitySimulationTimestamp(currentSimulationTime)
        { }

        void ResetToBegin(
            float newX,
            float newBaseY,
            float currentSimulationTime)
        {
            CurrentBaseY = newBaseY;
            TargetX = newX;
            TargetVisibilityAlpha = 1.0f;
            TargetForceMultiplier = 1.0f;
            TargetHeatDepth = 0.0f;
            StartSimulationTimestamp = currentSimulationTime;
            LastActivitySimulationTimestamp = currentSimulationTime;
        }
    };

    std::vector<Tornado> mTornadoes;

    bool mutable mAreTornadoesDirtyForRendering;
};

}
