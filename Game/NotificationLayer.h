/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "PerfStats.h"
#include "RenderContext.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <chrono>
#include <deque>
#include <optional>
#include <string>
#include <vector>

class NotificationLayer
{
public:

	NotificationLayer(
		bool isUltraViolentMode,
		bool isSoundMuted,
		bool isDayLightCycleOn);

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

	void AddEphemeralTextLine(
		std::string const & text,
		std::chrono::duration<float> lifetime = std::chrono::duration<float>(1.0f));

	void SetPhysicsProbePanelOpen(float open);

	void SetPhysicsProbeReading(
		vec2f const & velocity,
		float temperature);

	void SetUltraViolentModeIndicator(bool isUltraViolentMode);

	void SetSoundMuteIndicator(bool isSoundMuted);

	void SetDayLightCycleIndicator(bool isDayLightCycleOn);

	// One frame only; after RenderUpload() it's gone
	// (special case as this is really UI)
	inline void SetHeatBlaster(
		vec2f const & worldCoordinates,
		float radius,
		HeatBlasterActionType action)
	{
		mHeatBlasterFlameToRender.emplace(
			worldCoordinates,
			radius,
			action);
	}

	// One frame only; after RenderUpload() it's gone
	// (special case as this is really UI)
	inline void SetFireExtinguisherSpray(
		vec2f const & worldCoordinates,
		float radius)
	{
		mFireExtinguisherSprayToRender.emplace(
			worldCoordinates,
			radius);
	}

	void Reset();

    void Update(float now);

	void RenderUpload(Render::RenderContext & renderContext);

private:

	void UploadStatusTextLine(
		std::string & line,
		bool isEnabled,
		int & effectiveOrdinal,
		Render::RenderContext & renderContext);

private:

	//
	// Status text
	//

    bool mIsStatusTextEnabled;
    bool mIsExtendedStatusTextEnabled;
	std::array<std::string, 4> mStatusTextLines;
	bool mIsStatusTextDirty;

	//
	// Notification text
	//

	struct EphemeralTextLine
	{
		std::string Text;
		std::chrono::duration<float> Lifetime;

		enum class StateType
		{
			Initial,
			FadingIn,
			Displaying,
			FadingOut,
			Disappearing
		};

		StateType State;
		float CurrentStateStartTimestamp;
		float CurrentStateProgress;

		EphemeralTextLine(
			std::string const & text,
			std::chrono::duration<float> const lifetime)
			: Text(text)
			, Lifetime(lifetime)
			, State(StateType::Initial)
			, CurrentStateStartTimestamp(0.0f)
			, CurrentStateProgress(0.0f)
		{}

		EphemeralTextLine(EphemeralTextLine && other) = default;
		EphemeralTextLine & operator=(EphemeralTextLine && other) = default;
	};

	std::deque<EphemeralTextLine> mEphemeralTextLines; // Ordered from top to bottom

	bool mIsNotificationTextDirty;

	//
	// Texture notifications
	//

	bool mIsUltraViolentModeIndicatorOn;
	bool mIsSoundMuteIndicatorOn;
	bool mIsDayLightCycleOn;
	bool mAreTextureNotificationsDirty;

	//
	// Physics probe
	//

	float mPhysicsProbePanelOpen;
	bool mIsPhysicsProbePanelDirty;

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

	// When set, will be uploaded to display the HeatBlaster flame
	// - and then reset (one-time use, it's a special case as it's really UI)
	std::optional<HeatBlasterInfo> mHeatBlasterFlameToRender;

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

	// When set, will be uploaded to display the fire extinguisher spray
	// - and then reset (one-time use, it's a special case as it's really UI)
	std::optional<FireExtinguisherSpray> mFireExtinguisherSprayToRender;
};
