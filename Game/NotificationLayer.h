/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "PerfStats.h"
#include "RenderContext.h"
#include "RollingText.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <vector>

class NotificationLayer final : private IGenericGameEventHandler
{
public:

	NotificationLayer(
		bool isUltraViolentMode,
		bool isSoundMuted,
		bool isDayLightCycleOn,
		bool isAutoFocusOn,
		bool isShiftOn,
		UnitsSystem displayUnitsSystem,
		GameEventDispatcher & gameEventHandler);

	bool IsStatusTextEnabled() const { return mIsStatusTextEnabled; }
	void SetStatusTextEnabled(bool isEnabled);

	bool IsExtendedStatusTextEnabled() const { return mIsExtendedStatusTextEnabled; }
	void SetExtendedStatusTextEnabled(bool isEnabled);

    void SetStatusTexts(
        float immediateFps,
        float averageFps,
        PerfStats const & lastDeltaPerfStats,
        PerfStats const & totalPerfStats,
        std::chrono::duration<float> elapsedGameSeconds,
        bool isPaused,
        float zoom,
        vec2f const & camera,
        Render::RenderStatistics renderStats);

	void PublishNotificationText(
		std::string const & text,
		std::chrono::duration<float> lifetime = std::chrono::duration<float>(1.0f));

	void SetUltraViolentModeIndicator(bool isUltraViolentMode);

	void SetSoundMuteIndicator(bool isSoundMuted);

	void SetDayLightCycleIndicator(bool isDayLightCycleOn);

	void SetAutoFocusIndicator(bool isAutoFocusOn);

	void SetShiftIndicator(bool isShiftOn);

	void SetPhysicsProbePanelState(float targetOpen);

	void SetDisplayUnitsSystem(UnitsSystem value);

	// One frame only; after Update() it's gone
	inline void SetHeatBlaster(
		vec2f const & worldCoordinates,
		float radius,
		HeatBlasterActionType action)
	{
		mHeatBlasterFlameToRender1.emplace(
			worldCoordinates,
			radius,
			action);
	}

	// One frame only; after Update() it's gone
	inline void SetFireExtinguisherSpray(
		vec2f const & worldCoordinates,
		float radius)
	{
		mFireExtinguisherSprayToRender1.emplace(
			worldCoordinates,
			radius);
	}

	// One frame only; after Update() it's gone
	// (special case as this is really UI)
	inline void SetBlastToolHalo(
		vec2f const & worldCoordinates,
		float radius,
		float renderProgress,
		float personalitySeed)
	{
		mBlastToolHaloToRender1.emplace(
			worldCoordinates,
			radius,
			renderProgress,
			personalitySeed);
	}

	// One frame only; after Update() it's gone
	inline void SetPressureInjectionHalo(
		vec2f const & worldCoordinates,
		float flowMultiplier)
	{
		mPressureInjectionHaloToRender1.emplace(
			worldCoordinates,
			flowMultiplier);
	}

	// One frame only; after Update() it's gone
	inline void SetWindSphere(
		vec2f const & sourcePos,
		float preFrontRadius,
		float preFrontIntensityMultiplier,
		float mainFrontRadius,
		float mainFrontIntensityMultiplier)
	{
		mWindSphereToRender1.emplace(
			sourcePos,
			preFrontRadius,
			preFrontIntensityMultiplier,
			mainFrontRadius,
			mainFrontIntensityMultiplier);
	}

	// One frame only; after Update() it's gone
	inline void SetLaserCannon(
		DisplayLogicalCoordinates const & center,
		std::optional<float> strength)
	{
		mLaserCannonToRender1.emplace(
			center,
			strength);
	}

	// One frame only; after Update() it's gone
	inline void SetGripCircle(
		vec2f const & worldCenterPosition,
		float worldRadius)
	{
		mGripCircleToRender1.emplace(
			worldCenterPosition,
			worldRadius);
	}

	// One frame only; after Update() it's gone
	inline void ShowInteractiveToolDashedLine(
		DisplayLogicalCoordinates const & start,
		DisplayLogicalCoordinates const & end)
	{
		mInteractiveToolDashedLineToRender1.emplace_back(
			start,
			end);
	}

	void Reset();

    void Update(
		float now,
		float currentSimulationTime);

	void RenderUpload(Render::RenderContext & renderContext);

private:

	//
	// IGenericGameEventHandler
	//

	void OnPhysicsProbeReading(
		vec2f const & velocity,
		float temperature,
		float depth,
		float pressure) override;

private:

	void UploadStatusTextLine(
		std::string & line,
		bool isEnabled,
		int & effectiveOrdinal,
		Render::NotificationRenderContext & notificationRenderContext);

	void RegeneratePhysicsProbeReadingStrings();

private:

	GameEventDispatcher & mGameEventHandler;

	//
	// Status text
	//

    bool mIsStatusTextEnabled;
    bool mIsExtendedStatusTextEnabled;
	std::array<std::string, 4> mStatusTextLines;
	bool mIsStatusTextDirty;

	//
	// Notifications
	//

	RollingText mRollingText;

	bool mIsUltraViolentModeIndicatorOn;
	bool mIsSoundMuteIndicatorOn;
	bool mIsDayLightCycleOn;
	bool mIsAutoFocusOn;
	bool mIsShiftOn;
	bool mAreTextureNotificationsDirty;

	//
	// Physics probe
	//

	struct PhysicsProbePanelState
	{
		static std::chrono::duration<float> constexpr OpenDelayDuration = std::chrono::duration<float>(0.5f);
		static std::chrono::duration<float> constexpr TransitionDuration = std::chrono::duration<float>(2.1f); // After open delay

		float CurrentOpen;
		float TargetOpen;
		float CurrentStateStartTime;

		PhysicsProbePanelState()
		{
			Reset();
		}

		void Reset()
		{
			CurrentOpen = 0.0f;
			TargetOpen = 0.0f;
			CurrentStateStartTime = 0.0f;
		}
	};

	PhysicsProbePanelState mPhysicsProbePanelState;
	bool mIsPhysicsProbePanelDirty;

	struct PhysicsProbeReading
	{
		float Speed;
		float Temperature;
		float Depth;
		float Pressure;
	};

	PhysicsProbeReading mPhysicsProbeReading; // Storage for raw reading values

	struct PhysicsProbeReadingStrings
	{
		std::string Speed;
		std::string Temperature;
		std::string Depth;
		std::string Pressure;

		PhysicsProbeReadingStrings(
			std::string && speed,
			std::string && temperature,
			std::string && depth,
			std::string && pressure)
			: Speed(std::move(speed))
			, Temperature(std::move(temperature))
			, Depth(std::move(depth))
			, Pressure(std::move(pressure))
		{}
	};

	std::optional<PhysicsProbeReadingStrings> mPhysicsProbeReadingStrings;
	bool mArePhysicsProbeReadingStringsDirty;

	//
	// Units system
	//

	UnitsSystem mDisplayUnitsSystem;
	// No need to track dirtyness

	//
	// Interactions
	//

	struct HeatBlasterInfo
	{
		vec2f WorldCoordinates;
		float Radius;
		HeatBlasterActionType Action;

		HeatBlasterInfo(
			vec2f const & worldCoordinates,
			float radius,
			HeatBlasterActionType action)
			: WorldCoordinates(worldCoordinates)
			, Radius(radius)
			, Action(action)
		{}
	};

	std::optional<HeatBlasterInfo> mHeatBlasterFlameToRender1;
	std::optional<HeatBlasterInfo> mHeatBlasterFlameToRender2;

	struct FireExtinguisherSpray
	{
		vec2f WorldCoordinates;
		float Radius;

		FireExtinguisherSpray(
			vec2f const & worldCoordinates,
			float radius)
			: WorldCoordinates(worldCoordinates)
			, Radius(radius)
		{}
	};

	std::optional<FireExtinguisherSpray> mFireExtinguisherSprayToRender1;
	std::optional<FireExtinguisherSpray> mFireExtinguisherSprayToRender2;

	struct BlastToolHalo
	{
		vec2f WorldCoordinates;
		float Radius;
		float RenderProgress;
		float PersonalitySeed;

		BlastToolHalo(
			vec2f const & worldCoordinates,
			float radius,
			float renderProgress,
			float personalitySeed)
			: WorldCoordinates(worldCoordinates)
			, Radius(radius)
			, RenderProgress(renderProgress)
			, PersonalitySeed(personalitySeed)
		{}
	};

	std::optional<BlastToolHalo> mBlastToolHaloToRender1;
	std::optional<BlastToolHalo> mBlastToolHaloToRender2;

	struct PressureInjectionHalo
	{
		vec2f WorldCoordinates;
		float FlowMultiplier;

		PressureInjectionHalo(
			vec2f const & worldCoordinates,
			float flowMultiplier)
			: WorldCoordinates(worldCoordinates)
			, FlowMultiplier(flowMultiplier)
		{}
	};

	std::optional<PressureInjectionHalo> mPressureInjectionHaloToRender1;
	std::optional<PressureInjectionHalo> mPressureInjectionHaloToRender2;

	struct WindSphere
	{
		vec2f SourcePos;
		float PreFrontRadius;
		float PreFrontIntensityMultiplier;
		float MainFrontRadius;
		float MainFrontIntensityMultiplier;

		WindSphere(
			vec2f const & sourcePos,
			float preFrontRadius,
			float preFrontIntensityMultiplier,
			float mainFrontRadius,
			float mainFrontIntensityMultiplier)
			: SourcePos(sourcePos)
			, PreFrontRadius(preFrontRadius)
			, PreFrontIntensityMultiplier(preFrontIntensityMultiplier)
			, MainFrontRadius(mainFrontRadius)
			, MainFrontIntensityMultiplier(mainFrontIntensityMultiplier)
		{}
	};

	std::optional<WindSphere> mWindSphereToRender1;
	std::optional<WindSphere> mWindSphereToRender2;

	struct LaserCannon
	{
		DisplayLogicalCoordinates Center;
		std::optional<float> Strength;

		LaserCannon(
			DisplayLogicalCoordinates const & center,
			std::optional<float> strength)
			: Center(center)
			, Strength(strength)
		{}
	};

	std::optional<LaserCannon> mLaserCannonToRender1;
	std::optional<LaserCannon> mLaserCannonToRender2;

	struct GripCircle
	{
		vec2f WorldCenterPosition;
		float WorldRadius;

		GripCircle(
			vec2f const & worldCenterPosition,
			float worldRadius)
			: WorldCenterPosition(worldCenterPosition)
			, WorldRadius(worldRadius)
		{}
	};

	std::optional<GripCircle> mGripCircleToRender1;
	std::optional<GripCircle> mGripCircleToRender2;

	struct LineGuide
	{
		DisplayLogicalCoordinates Start;
		DisplayLogicalCoordinates End;

		LineGuide(
			DisplayLogicalCoordinates const & start,
			DisplayLogicalCoordinates const & end)
			: Start(start)
			, End(end)
		{}
	};

	std::vector<LineGuide> mInteractiveToolDashedLineToRender1;
	std::vector<LineGuide> mInteractiveToolDashedLineToRender2;
};
