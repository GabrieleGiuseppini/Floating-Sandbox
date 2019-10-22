/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "TextRenderContext.h"

#include <GameCore/GameTypes.h>

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

    void SetStatusTextEnabled(bool isEnabled)
	{
		mIsStatusTextEnabled = isEnabled;
	}

	void SetExtendedStatusTextEnabled(bool isEnabled)
	{
		mIsExtendedStatusTextEnabled = isEnabled;
	}

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

	std::shared_ptr<Render::TextRenderContext> mTextRenderContext;


	//
	// Status text state
	//

    bool mIsStatusTextEnabled;
    bool mIsExtendedStatusTextEnabled;
    std::vector<std::string> mTextLines;
    RenderedTextHandle mTextHandle;
    bool mIsTextDirty;
};

