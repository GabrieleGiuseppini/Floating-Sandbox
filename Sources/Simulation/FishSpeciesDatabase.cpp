/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-10-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "FishSpeciesDatabase.h"

#include <Core/Utils.h>

#include <set>

FishSpeciesDatabase FishSpeciesDatabase::Load(IAssetManager const & assetManager)
{
    picojson::value const root = assetManager.LoadFishSpeciesDatabase();

    if (!root.is<picojson::array>())
    {
        throw GameException("Fish species database is not a JSON array");
    }

    std::vector<FishSpecies> fishSpecies;

    std::set<std::string> uniqueFishSpeciesNames;

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
            // Make sure name is unique
            if (!uniqueFishSpeciesNames.insert(name).second)
            {
                throw GameException("Species name is not unique");
            }

            vec2f const worldSize = vec2f(
                Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "world_size_x"),
                Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "world_size_y"));

            size_t const shoalSize = Utils::GetMandatoryJsonMember<size_t>(fishSpeciesObject, "shoal_size");
            float const shoalRadius = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "shoal_radius");
            float const oceanDepth = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "ocean_depth");
            float const basalSpeed = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "basal_speed");

            float const tailX = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "tail_x");
            float const tailSpeed = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "tail_speed");
            float const tailSwingWidth = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "tail_swing_width");

            float const headOffsetX = Utils::GetMandatoryJsonMember<float>(fishSpeciesObject, "head_offset_x");

            std::vector<TextureFrameIndex> textureFrameIndices;
            {
                auto const textureIndicesArray = Utils::GetMandatoryJsonArray(fishSpeciesObject, "texture_indices");

                std::transform(
                    textureIndicesArray.cbegin(),
                    textureIndicesArray.cend(),
                    std::back_inserter(textureFrameIndices),
                    [](auto const & e) -> TextureFrameIndex
                    {
                        return static_cast<TextureFrameIndex>(Utils::GetJsonValueAs<std::int64_t>(e, "texture_indices"));
                    });
            }

            fishSpecies.emplace_back(
                name,
                worldSize,
                shoalSize,
                shoalRadius,
                oceanDepth,
                basalSpeed,
                tailX,
                tailSpeed,
                tailSwingWidth,
                headOffsetX,
                textureFrameIndices);
        }
        catch (GameException const & ex)
        {
            throw GameException(std::string("Error parsing fish species \"") + name + "\": " + ex.what());
        }

    }

    return FishSpeciesDatabase(std::move(fishSpecies));
}