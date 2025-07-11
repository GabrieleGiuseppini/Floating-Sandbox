/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <Core/TextureAtlas.h>
#include <Core/TextureDatabase.h>
#include <Core/Utils.h>

#include <Game/FileStreams.h>
#include <Game/GameAssetManager.h>

#include <filesystem>
#include <iostream>
#include <string>

class AtlasBaker
{
public:

    struct AtlasBakingOptions
    {
        bool AlphaPremultiply;
        bool BinaryTransparencySmoothing;
        bool MipMappable;
        bool Regular;
        bool SuppressDuplicates;

        static AtlasBakingOptions Deserialize(std::filesystem::path const & optionsJsonFilePath)
        {
            picojson::object rootJsonObject = Utils::GetJsonValueAsObject(Utils::ParseJSONString(FileTextReadStream(optionsJsonFilePath).ReadAll()), "root");

            bool alphaPreMultiply = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "alpha_pre_multiply");
            bool mipMappable = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "mip_mappable");
            bool binaryTransparencySmoothing = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "binary_transparency_smoothing");
            bool regular = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "regular");
            bool suppressDuplicates = Utils::GetMandatoryJsonMember<bool>(rootJsonObject, "suppress_duplicates");

            return AtlasBakingOptions({
                alphaPreMultiply,
                binaryTransparencySmoothing,
                mipMappable,
                regular,
                suppressDuplicates });
        }
    };

public:

    template<typename TTextureDatabase>
    static std::tuple<size_t, ImageSize> Bake(
        std::filesystem::path const & texturesRootDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        AtlasBakingOptions const & options,
        float resizeFactor)
    {
        if (!std::filesystem::exists(texturesRootDirectoryPath))
        {
            throw std::runtime_error("Textures root directory '" + texturesRootDirectoryPath.string() + "' does not exist");
        }

        if (!std::filesystem::exists(outputDirectoryPath))
        {
            throw std::runtime_error("Output directory '" + outputDirectoryPath.string() + "' does not exist");
        }

        // Instantiate asset manager
        GameAssetManager assetManager(texturesRootDirectoryPath);

        // Load database
        auto textureDatabase = TextureDatabase<TTextureDatabase>::Load(assetManager);

        // Create atlas

        std::cout << "Creating " << (options.Regular ? "regular " : "") << "atlas...";

        TextureAtlasOptions atlasOptions = TextureAtlasOptions::None;
        if (options.AlphaPremultiply)
            atlasOptions = atlasOptions | TextureAtlasOptions::AlphaPremultiply;
        if (options.BinaryTransparencySmoothing)
            atlasOptions = atlasOptions | TextureAtlasOptions::BinaryTransparencySmoothing;
        if (options.MipMappable)
            atlasOptions = atlasOptions | TextureAtlasOptions::MipMappable;
        if (options.SuppressDuplicates)
            atlasOptions = atlasOptions | TextureAtlasOptions::SuppressDuplicates;

        auto textureAtlas = options.Regular
            ? TextureAtlasBuilder<TTextureDatabase>::BuildRegularAtlas(
                textureDatabase,
                atlasOptions,
                resizeFactor,
                assetManager,
                SimpleProgressCallback([](float)
                {
                    std::cout << ".";
                }))
            : TextureAtlasBuilder<TTextureDatabase>::BuildAtlas(
                textureDatabase,
                atlasOptions,
                resizeFactor,
                assetManager,
                SimpleProgressCallback([](float)
                {
                    std::cout << ".";
                }));

        std::cout << std::endl;

        // Serialize atlas
        auto const [specificationJson, atlasImage] = textureAtlas.Serialize();
        assetManager.SaveJson(specificationJson, outputDirectoryPath / GameAssetManager::MakeAtlasSpecificationFilename(TTextureDatabase::DatabaseName));
        assetManager.SavePngImage(atlasImage, outputDirectoryPath / GameAssetManager::MakeAtlasImageFilename(TTextureDatabase::DatabaseName));

        return { textureAtlas.Metadata.GetFrameCount(), textureAtlas.Image.Size };
    }
};
