/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Baker.h"

#include <iostream>

void Baker::BakeRegularAtlas(
    std::filesystem::path const & jsonSpecificationFilePath,
    std::filesystem::path const & outputDirectoryPath,
    std::string const & atlasFilenamesStem)
{
    if (!std::filesystem::exists(jsonSpecificationFilePath))
    {
        throw std::runtime_error("JSON specification file '" + jsonSpecificationFilePath.string() + "' does not exist");
    }

    if (!std::filesystem::exists(outputDirectoryPath))
    {
        throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
    }

    // Create output filenames
    auto outputAtlasMetadataFilePath = outputDirectoryPath / (atlasFilenamesStem + ".atlas.metadata");
    auto outputAtlasImageFilePath = outputDirectoryPath / (atlasFilenamesStem + ".atlas.png");

    std::cout << "Output atlas metadata file: " << outputAtlasMetadataFilePath << std::endl;
    std::cout << "Output atlas image file   : " << outputAtlasImageFilePath << std::endl;
    // TODOHERE
}