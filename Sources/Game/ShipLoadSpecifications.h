/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Simulation/ShipLoadOptions.h>

#include <filesystem>

struct ShipLoadSpecifications
{
	std::filesystem::path DefinitionFilepath;
	ShipLoadOptions LoadOptions;

	explicit ShipLoadSpecifications(std::filesystem::path const & definitionFilepath)
		: DefinitionFilepath(definitionFilepath)
		, LoadOptions()
	{}

	ShipLoadSpecifications(
		std::filesystem::path const & definitionFilepath,
		ShipLoadOptions const & options)
		: DefinitionFilepath(definitionFilepath)
		, LoadOptions(options)
	{}

	static ShipLoadSpecifications FromJson(picojson::object const & specsRoot)
	{
		return ShipLoadSpecifications(
			Utils::GetMandatoryJsonMember<std::string>(specsRoot, "definition_filepath"),
			ShipLoadOptions::FromJson(Utils::GetMandatoryJsonObject(specsRoot, "options")));
	}

	picojson::object ToJson() const
	{
		picojson::object specsRoot;

		specsRoot.emplace("definition_filepath", picojson::value(DefinitionFilepath.string()));
		specsRoot.emplace("options", LoadOptions.ToJson());

		return specsRoot;
	}
};
