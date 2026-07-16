/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/Utils.h>

#include <picojson.h>

struct ShipLoadOptions
{
	bool FlipHorizontally;
	bool FlipVertically;
	bool Rotate90CW;

	ShipLoadOptions()
		: FlipHorizontally(false)
		, FlipVertically(false)
		, Rotate90CW(false)
	{}

	ShipLoadOptions(
		bool flipHorizontally,
		bool flipVertically,
		bool rotate90CW)
		: FlipHorizontally(flipHorizontally)
		, FlipVertically(flipVertically)
		, Rotate90CW(rotate90CW)
	{}

	picojson::object Serialize() const;
	static ShipLoadOptions Deserialize(picojson::value const & optionsRoot);
};
