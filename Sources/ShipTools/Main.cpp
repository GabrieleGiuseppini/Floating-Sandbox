/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include "AndroidTextureDatabases.h"
#include "Baker.h"

#include <Render/GameTextureDatabases.h>

#include <Core/ImageData.h>
#include <Core/Utils.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#define SEPARATOR "------------------------------------------------------"

int DoBakeAtlas(int argc, char ** argv);

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
        if (verb == "bake_atlas")
        {
            return DoBakeAtlas(argc, argv);
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

int DoBakeAtlas(int argc, char ** argv)
{
    if (argc < 5 || argc > 7)
    {
        PrintUsage();
        return 0;
    }

    std::string const databaseName = argv[2];
    std::filesystem::path texturesRootDirectoryPath(argv[3]);
    std::filesystem::path outputDirectoryPath(argv[4]);
    Baker::AtlasBakingOptions options({
        false,
        false,
        false,
        false
        });

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

            options = Baker::AtlasBakingOptions::Deserialize(argv[i + 1]);

            ++i;
        }
        else
        {
            throw std::runtime_error("Unrecognized option '" + option + "'");
        }
    }

    std::cout << SEPARATOR << std::endl;

    std::cout << "Running bake_atlas:" << std::endl;
    std::cout << "  database name                 : " << databaseName << std::endl;
    std::cout << "  textures root directory       : " << texturesRootDirectoryPath << std::endl;
    std::cout << "  output directory              : " << outputDirectoryPath << std::endl;
    std::cout << "  alpha-premultiply             : " << options.AlphaPremultiply << std::endl;
    std::cout << "  binary transparency smoothing : " << options.BinaryTransparencySmoothing << std::endl;
    std::cout << "  mip-mappable                  : " << options.MipMappable << std::endl;
    std::cout << "  regular                       : " << options.Regular << std::endl;
    std::cout << "  duplicates suppression        : " << options.SuppressDuplicates << std::endl;

    std::tuple<size_t, ImageSize> atlasData{ 0, ImageSize(0, 0) };
    if (Utils::CaseInsensitiveEquals(databaseName, "cloud"))
    {
        atlasData = Baker::BakeAtlas<GameTextureDatabases::CloudTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "explosion"))
    {
        atlasData = Baker::BakeAtlas<GameTextureDatabases::ExplosionTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "fish"))
    {
        atlasData = Baker::BakeAtlas<GameTextureDatabases::FishTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "npc"))
    {
        atlasData = Baker::BakeAtlas<GameTextureDatabases::NpcTextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "androidui"))
    {
        atlasData = Baker::BakeAtlas<UITextureDatabases::UITextureDatabase>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
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
    std::cout << " bake_atlas Cloud|Explosion|NPC|AndroidUI <textures_root_dir> <out_dir> [[-a] [-b] [-m] [-d] [-r] | -o <options_json>]" << std::endl;
}