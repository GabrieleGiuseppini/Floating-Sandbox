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

	picojson::object Serialize() const
	{
		picojson::object optionsRoot;

		optionsRoot.emplace("flip_horizontally", picojson::value(FlipHorizontally));
		optionsRoot.emplace("flip_vertically", picojson::value(FlipVertically));
		optionsRoot.emplace("rotate_90cw", picojson::value(Rotate90CW));

		return optionsRoot;
	}

	static ShipLoadOptions Deserialize(picojson::value const & optionsRoot)
	{
        auto const & optionsRootAsObject = Utils::GetJsonValueAsObject(optionsRoot, "ShipMetadata");

		return ShipLoadOptions(
			Utils::GetMandatoryJsonMember<bool>(optionsRootAsObject, "flip_horizontally"),
			Utils::GetMandatoryJsonMember<bool>(optionsRootAsObject, "flip_vertically"),
			Utils::GetMandatoryJsonMember<bool>(optionsRootAsObject, "rotate_90cw"));
	}
};
