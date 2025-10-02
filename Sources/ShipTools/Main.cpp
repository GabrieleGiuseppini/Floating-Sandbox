/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include "AndroidTextureDatabases.h"
#include "ShipDatabaseBaker.h"
#include "SoundAtlasBaker.h"
#include "TextureAtlasBaker.h"

#include <Render/GameTextureDatabases.h>

#include <Core/ImageData.h>
#include <Core/Utils.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#define SEPARATOR "------------------------------------------------------"

int DoBakeShipDatabase(int argc, char ** argv);

int DoBakeSoundAtlas(int argc, char ** argv);

int DoBakeTextureAtlas(int argc, char ** argv);

void PrintUsage();

int main(int argc, char ** argv)
{
    if (argc == 1)
    {
        PrintUsage();
        return 0;
    }

    std::string verb(argv[1]);
    try
    {
        if (verb == "bake_ship_database")
        {
            return DoBakeShipDatabase(argc, argv);
        }
        else if (verb == "bake_sound_atlas")
        {
            return DoBakeSoundAtlas(argc, argv);
        }
        else if (verb == "bake_texture_atlas")
        {
            return DoBakeTextureAtlas(argc, argv);
        }
        else
        {
            throw std::runtime_error("Unrecognized verb '" + verb + "'");
        }
    }
    catch (std::exception & ex)
    {
        std::cout << "ERROR: " << ex.what() << std::endl;
        return -1;
    }
}

int DoBakeShipDatabase(int argc, char ** argv)
{
    if (argc < 7)
    {
        PrintUsage();
        return 0;
    }

    std::filesystem::path const shipDirectoryJsonFilePath(argv[2]);
    std::filesystem::path const shipRootPath(argv[3]);
    std::filesystem::path const outputDirectoryPath(argv[4]);
    int const maxPreviewImageSizeWidth = atoi(argv[5]);
    int const maxPreviewImageSizeHeight = atoi(argv[6]);

    std::cout << SEPARATOR << std::endl;

    std::cout << "Running bake_ship_database:" << std::endl;
    std::cout << "  directory json                : " << shipDirectoryJsonFilePath << std::endl;
    std::cout << "  ship root directory           : " << shipRootPath << std::endl;
    std::cout << "  output directory              : " << outputDirectoryPath << std::endl;
    std::cout << "  max preview image size        : " << maxPreviewImageSizeWidth << "x" << maxPreviewImageSizeHeight << std::endl;

    ShipDatabaseBaker::Bake(
        shipDirectoryJsonFilePath,
        shipRootPath,
        outputDirectoryPath,
        ImageSize(maxPreviewImageSizeWidth, maxPreviewImageSizeHeight));

    std::cout << "Baking completed." << std::endl;

    return 0;
}

int DoBakeSoundAtlas(int argc, char ** argv)
{
    //
    // Parse args
    //

    if (argc != 5)
    {
        PrintUsage();
        return 0;
    }

    std::filesystem::path const soundsRootDirectoryPath(argv[2]);
    std::string const atlasName = argv[3];
    std::filesystem::path const outputDirectoryPath(argv[4]);

    std::cout << SEPARATOR << std::endl;

    std::cout << "Running bake_sound_atlas:" << std::endl;
    std::cout << "  sounds root directory         : " << soundsRootDirectoryPath << std::endl;
    std::cout << "  atlas name                    : " << atlasName << std::endl;
    std::cout << "  output directory              : " << outputDirectoryPath << std::endl;

    //
    // Bake
    //

    auto const [soundCount, atlasFileSize] = SoundAtlasBaker::Bake(soundsRootDirectoryPath, atlasName, outputDirectoryPath);

    //
    // Stats
    //

    std::cout << "Baking completed - " << soundCount << " sounds, " << static_cast<float>(atlasFileSize) / (1024.0f * 1024.0f) << " MBs." << std::endl;

    return 0;
}

int DoBakeTextureAtlas(int argc, char ** argv)
{
    if (argc < 5)
    {
        PrintUsage();
        return 0;
    }

    std::string const databaseName = argv[2];
    std::filesystem::path texturesRootDirectoryPath(argv[3]);
    std::filesystem::path outputDirectoryPath(argv[4]);
    TextureAtlasBaker::BakingOptions options({
        false,
        false,
        false,
        false
        });
    float resizeFactor = 1.0f;

    for (int i = 5; i < argc; ++i)
    {
        std::string option(argv[i]);
        if (option == "-a")
        {
            options.AlphaPremultiply = true;
        }
        else if (option == "-b")
        {
            options.BinaryTransparencySmoothing = true;
        }
        else if (option == "-d")
        {
            options.SuppressDuplicates = true;
        }
        else if (option == "-m")
        {
            options.MipMappable = true;
        }
        else if (option == "-r")
        {
            options.Regular = true;
        }
        else if (option == "-o")
        {
            if (i == argc - 1)
            {
                throw std::runtime_error("Missing options json filepath");
            }

            options = TextureAtlasBaker::BakingOptions::Deserialize(argv[i + 1]);

            ++i;
        }
        else if (option == "-z")
        {
            if (i == argc - 1)
            {
                throw std::runtime_error("Missing resize factor");
            }

            resizeFactor = static_cast<float>(atof(argv[i + 1]));

            ++i;
        }
        else
        {
            throw std::runtime_error("Unrecognized option '" + option + "'");
        }
    }

    std::cout << SEPARATOR << std::endl;

    std::cout << "Running bake_texture_atlas:" << std::endl;
    std::cout << "  database name                 : " << databaseName << std::endl;
    std::cout << "  textures root directory       : " << texturesRootDirectoryPath << std::endl;
    std::cout << "  output directory              : " << outputDirectoryPath << std::endl;
    std::cout << "  alpha-premultiply             : " << options.AlphaPremultiply << std::endl;
    std::cout << "  binary transparency smoothing : " << options.BinaryTransparencySmoothing << std::endl;
    std::cout << "  mip-mappable                  : " << options.MipMappable << std::endl;
    std::cout << "  regular                       : " << options.Regular << std::endl;
    std::cout << "  duplicates suppression        : " << options.SuppressDuplicates << std::endl;
    std::cout << "  resize factor                 : " << resizeFactor << std::endl;

    std::tuple<size_t, ImageSize> atlasData{ 0, ImageSize(0, 0) };
    if (Utils::CaseInsensitiveEquals(databaseName, "cloud"))
    {
        atlasData = TextureAtlasBaker::Bake<GameTextureDatabases::CloudTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options,
            resizeFactor);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "explosion"))
    {
        atlasData = TextureAtlasBaker::Bake<GameTextureDatabases::ExplosionTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options,
            resizeFactor);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "fish"))
    {
        atlasData = TextureAtlasBaker::Bake<GameTextureDatabases::FishTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options,
            resizeFactor);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "npc"))
    {
        atlasData = TextureAtlasBaker::Bake<GameTextureDatabases::NpcTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options,
            resizeFactor);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "androidui"))
    {
        atlasData = TextureAtlasBaker::Bake<UITextureDatabases::UITextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options,
            resizeFactor);
    }
    else
    {
        throw std::runtime_error("Unrecognized database name '" + databaseName + "'");
    }

    std::cout << "Baking completed - " << std::get<0>(atlasData) << " frames, " << std::get<1>(atlasData).width
        << "x" << std::get<1>(atlasData).height << "." << std::endl;

    return 0;
}

void PrintUsage()
{
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << " bake_ship_database <ship_directory_json> <ship_root_dir> <out_dir> <max_preview_w> <max_preview_h>" << std::endl;
    std::cout << " bake_sound_atlas <sounds_root_dir> <atlas_name> <out_dir>" << std::endl;
    std::cout << " bake_texture_atlas Cloud|Explosion|NPC|AndroidUI <textures_root_dir> <out_dir> [[-a] [-b] [-m] [-d] [-r] | -o <options_json>] [-z <resize_factor>]" << std::endl;
}