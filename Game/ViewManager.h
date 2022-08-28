/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "NotificationLayer.h"
#include "RenderContext.h"

#include <GameCore/AABBSet.h>
#include <GameCore/ParameterSmoother.h>
#include <GameCore/Vectors.h>

#include <memory>
#include <optional>

class ViewManager
{
public:

    ViewManager(
        Render::RenderContext & renderContext,
        NotificationLayer & notificationLayer);

    float GetCameraSpeedAdjustment() const;
    void SetCameraSpeedAdjustment(float value);
    static float constexpr GetMinCameraSpeedAdjustment() { return 0.2f; }
    static float constexpr GetMaxCameraSpeedAdjustment() { return 10.0f; }

    bool GetDoAutoFocusOnShipLoad() const;
    void SetDoAutoFocusOnShipLoad(bool value);

    bool GetDoContinuousAutoFocus() const;
    void SetDoContinuousAutoFocus(bool value);

    void OnViewModelUpdated();
    void OnNewShip(Geometry::AABBSet const & allAABBs);
    void Pan(vec2f const & worldOffset);
    void PanToWorldX(float worldX);
    void AdjustZoom(float amount);
    void ResetView(Geometry::AABBSet const & allAABBs);
    void FocusOnShip(Geometry::AABBSet const & allAABBs);

    void Update(Geometry::AABBSet const & allAABBs);

private:

    static float CalculateZoomParameterSmootherConvergenceFactor(float cameraSpeedAdjustment);
    static float CalculateCameraWorldPositionParameterSmootherConvergenceFactor(float cameraSpeedAdjustment);
    static float CalculateParameterSmootherConvergenceFactor(
        float cameraSpeedAdjustment,
        float min,
        float mid,
        float max);

    void InternalFocusOnShip(Geometry::AABBSet const & allAABBs);

    float InternalCalculateZoom(Geometry::AABB const & aabb);

private:

    Render::RenderContext & mRenderContext;
    NotificationLayer & mNotificationLayer;

    std::unique_ptr<ParameterSmoother<float>> mZoomParameterSmoother;
    std::unique_ptr<ParameterSmoother<vec2f>> mCameraWorldPositionParameterSmoother;

    float mCameraSpeedAdjustment; // Storage

    bool mDoAutoFocusOnShipLoad; // Storage

    struct AutoFocusSessionData
    {
        float CurrentAutoFocusZoom;
        vec2f CurrentAutoFocusCameraWorldPosition;

        float UserZoomOffset;
        vec2f UserCameraWorldPositionOffset;

        AutoFocusSessionData(
            float currentAutoFocusZoom,
            vec2f const & currentAutoFocusCameraWorldPosition)
            : CurrentAutoFocusZoom(currentAutoFocusZoom)
            , CurrentAutoFocusCameraWorldPosition(currentAutoFocusCameraWorldPosition)
        {
            Reset();
        }

        void Reset()
        {            
            UserZoomOffset = 1.0f;
            UserCameraWorldPositionOffset = vec2f::zero();
        }
    };

    std::optional<AutoFocusSessionData> mAutoFocus;
};
