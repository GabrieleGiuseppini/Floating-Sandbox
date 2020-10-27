/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-10-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "FishSpeciesDatabase.h"

#include <GameCore/Utils.h>

FishSpeciesDatabase FishSpeciesDatabase::Load(std::filesystem::path fishSpeciesDatabaseFilePath)
{
    picojson::value const root = Utils::ParseJSONFile(fishSpeciesDatabaseFilePath);

    if (!root.is<picojson::array>())
    {
        throw GameException("Fish species database is not a JSON array");
    }

    std::vector<FishSpecies> fishSpecies;

    picojson::array const & speciesArray = root.get<picojson::array>();
    for (auto const & fishSpeciesElem : speciesArray)
    {
        if (!fishSpeciesElem.is<picojson::object>())
        {
            throw GameException("Found a non-object in fish species array");
        }

        picojson::object const & fishSpeciesObject = fishSpeciesElem.get<picojson::object>();

        std::string const name = Utils::GetMandatoryJsonMember<std::string>(fishSpeciesObject, "name");

        try
        {
            size_t const shoalSize = Utils::GetMandatoryJsonMember<size_t>(fishSpeciesObject, "shoal_size");
            float const basalDepth = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "basal_depth");
            float const basalSpeed = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "basal_speed");
            float const tailX = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "tail_x");

            auto const textureIndex = static_cast<TextureFrameIndex>(Utils::GetMandatoryJsonMember<int>(fishSpeciesObject, "texture_index"));

            fishSpecies.emplace_back(
                name,
                shoalSize,
                basalDepth,
                basalSpeed,
                tailX,
                textureIndex);
        }
        catch (GameException const & ex)
        {
            throw GameException(std::string("Error parsing fish species \"") + name + "\": " + ex.what());
        }

    }

    return FishSpeciesDatabase(std::move(fishSpecies));
}