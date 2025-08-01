/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-09-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"

class StockColors final
{
public:

	// NpcSelection, tool radius notifications (Android)
	static rgbColor constexpr Red1 = rgbColor(224, 18, 18);
    // Repair radius notification (Android)
    static rgbColor constexpr Yellow1 = rgbColor(224, 232, 9);

	static rgbColor constexpr White = rgbColor(255, 255, 255);
	static rgbColor constexpr CornflowerBlue = rgbColor(135, 207, 250); // Cornflower blue
};
