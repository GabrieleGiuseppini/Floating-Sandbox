/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SoundAtlas.h"

SoundAtlas SoundAtlas::Deserialize(
    picojson::value const & atlasJson,
    BinaryReadStream & atlasData)
{
    SoundAtlasAssetsMetadata metadata = SoundAtlasAssetsMetadata::Dererialize(atlasJson);
    // TODOHERE: read whole stream into buffer

}

SoundAtlasAssetsMetadata SoundAtlasBuilder::BuildAtlas(
    std::vector<std::string> const & assetNames,
    std::unordered_map<std::string, SoundAssetProperties> assetPropertiesProvider,
    std::function<Buffer<float>(std::string const & assetName)> const & assetLoader,
    BinaryWriteStream & outputStream)
{
    // TODOHERE
}