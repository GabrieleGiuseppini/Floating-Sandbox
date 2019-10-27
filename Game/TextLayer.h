/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextRenderContext.h"

#include <GameCore/GameTypes.h>

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

class TextLayer
{
public:

	TextLayer(
		std::shared_ptr<Render::TextRenderContext> textRenderContext,
        bool isStatusTextEnabled,
        bool isExtendedStatusTextEnabled);

	void SetStatusTextEnabled(bool isEnabled);

	void SetExtendedStatusTextEnabled(bool isEnabled);

    void SetStatusTexts(
        float immediateFps,
        float averageFps,
        std::chrono::duration<float> elapsedGameSeconds,
        bool isPaused,
        float zoom,
        vec2f const & camera,
        float totalUpdateToRenderDurationRatio,
        float lastUpdateToRenderDurationRatio,
        Render::RenderStatistics const & renderStatistics);

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
	// Status text state
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
	
	std::array<StatusTextLine, 3> mStatusTextLines;

	bool mAreStatusTextLinePositionsDirty;
};

