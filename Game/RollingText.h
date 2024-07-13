/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RenderContext.h"

#include <chrono>
#include <deque>
#include <optional>
#include <string>

/*
 * A set of notification lines rolling from the bottom and pausing in the middle.
 */
class RollingText
{
public:

	RollingText();

	void AddLine(
		std::string const & text,
		std::chrono::duration<float> lifetime);

	void Update(float simulationTime);

	void RenderUpload(Render::NotificationRenderContext & notificationRenderContext);

	void Reset();

private:

	struct Line
	{
		std::string Text;
		unsigned int YOffset;
		std::chrono::duration<float> Lifetime;
		std::optional<float> StartCentralRestSimulationTime;

		explicit Line(
			std::string const & text,
			std::chrono::duration<float> lifetime)
			: Text(text)
			, YOffset(0) // Start from bottom
			, Lifetime(lifetime)
			, StartCentralRestSimulationTime()
		{}
	};

	std::deque<Line> mLines;

	bool mIsDirty; // When set, we have to upload changes to rendering subsystem
};
