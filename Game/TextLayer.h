/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "PerfStats.h"
#include "TextRenderContext.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class TextLayer
{
public:

	TextLayer(std::shared_ptr<Render::TextRenderContext> textRenderContext);

	bool IsStatusTextEnabled() const { return mIsStatusTextEnabled; }
	void SetStatusTextEnabled(bool isEnabled);

	bool IsExtendedStatusTextEnabled() const { return mIsExtendedStatusTextEnabled; }
	void SetExtendedStatusTextEnabled(bool isEnabled);

    void SetStatusTexts(
        float immediateFps,
        float averageFps,
        PerfStats const & lastDeltaPerfStats,
        PerfStats const & totalPerfStats,
        uint64_t lastDeltaFrameCount,
        uint64_t totalFrameCount,
        std::chrono::duration<float> elapsedGameSeconds,
        bool isPaused,
        float zoom,
        vec2f const & camera,
        Render::RenderStatistics const & renderStatistics);

	void AddEphemeralTextLine(
		std::string const & text,
		std::chrono::duration<float> lifetime = std::chrono::duration<float>(1.0f));

    void Update(float now);

private:

	struct StatusTextLine;

	void UpdateStatusTextLine(
		StatusTextLine & line,
		bool isEnabled,
		bool arePositionsDirty,
		int & ordinal);

private:

	std::shared_ptr<Render::TextRenderContext> mTextRenderContext;

	//
	// Status text
	//

    bool mIsStatusTextEnabled;
    bool mIsExtendedStatusTextEnabled;

	struct StatusTextLine
	{
		RenderedTextHandle Handle;
		std::string Text;
		bool IsTextDirty;

		StatusTextLine()
			: StatusTextLine("")
		{}

		StatusTextLine(std::string text)
			: Handle(NoneRenderedTextHandle)
			, Text(text)
			, IsTextDirty(true) // Start dirty
		{}
	};

	std::array<StatusTextLine, 4> mStatusTextLines;

	bool mAreStatusTextLinePositionsDirty;

	//
	// Ephemeral text
	//

	struct EphemeralTextLine
	{
		RenderedTextHandle Handle;
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

		EphemeralTextLine(
			std::string const & text,
			std::chrono::duration<float> const lifetime)
			: Handle(NoneRenderedTextHandle)
			, Text(text)
			, Lifetime(lifetime)
			, State(StateType::Initial)
			, CurrentStateStartTimestamp(0.0f)
		{}

		EphemeralTextLine(EphemeralTextLine && other) = default;
		EphemeralTextLine & operator=(EphemeralTextLine && other) = default;
	};

	std::deque<EphemeralTextLine> mEphemeralTextLines;
};
