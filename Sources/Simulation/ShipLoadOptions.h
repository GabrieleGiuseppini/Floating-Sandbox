/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/Utils.h>

#include <picojson.h>

#include <filesystem>

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

	static ShipLoadOptions FromJson(picojson::object const & optionsRoot)
	{
		return ShipLoadOptions(
			Utils::GetMandatoryJsonMember<bool>(optionsRoot, "flip_horizontally"),
			Utils::GetMandatoryJsonMember<bool>(optionsRoot, "flip_vertically"),
			Utils::GetMandatoryJsonMember<bool>(optionsRoot, "rotate_90cw"));
	}

	picojson::object ToJson() const
	{
		picojson::object optionsRoot;

		optionsRoot.emplace("flip_horizontally", picojson::value(FlipHorizontally));
		optionsRoot.emplace("flip_vertically", picojson::value(FlipVertically));
		optionsRoot.emplace("rotate_90cw", picojson::value(Rotate90CW));

		return optionsRoot;
	}
};
