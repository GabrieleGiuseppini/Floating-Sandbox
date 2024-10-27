/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include "Baker.h"
#include "Quantizer.h"
#include "Resizer.h"
#include "ShipAnalyzer.h"

#include <GameCore/Utils.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#define SEPARATOR "------------------------------------------------------"

int DoAnalyzeShip(int argc, char ** argv);
int DoBakeAtlas(int argc, char ** argv);
int DoQuantize(int argc, char ** argv);
int DoResize(int argc, char ** argv);

void PrintUsage();

int main(int argc, char ** argv)
{
    // Initialize DevIL
    ilInit();
    iluInit();

    if (argc == 1)
    {
        PrintUsage();
        return 0;
    }

    std::string verb(argv[1]);
    try
    {
        if (verb == "analyze")
        {
            return DoAnalyzeShip(argc, argv);
        }
        else if (verb == "bake_atlas")
        {
            return DoBakeAtlas(argc, argv);
        }
        else if (verb == "quantize")
        {
            return DoQuantize(argc, argv);
        }
        else if (verb == "resize")
        {
            return DoResize(argc, argv);
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

int DoAnalyzeShip(int argc, char ** argv)
{
    if (argc < 4)
    {
        PrintUsage();
        return 0;
    }

    std::string materialsDirectory(argv[2]);
    std::string inputFile(argv[3]);

    auto analysisInfo = ShipAnalyzer::Analyze(inputFile, materialsDirectory);

    std::cout << std::fixed;

    std::cout << "  Total mass                   : " << analysisInfo.TotalMass << std::endl;
    std::cout << "  Equivalent mass              : " << analysisInfo.AverageMassPerPoint << std::endl;
    std::cout << "  Equivalent air buoyant mass  : " << analysisInfo.AverageAirBuoyantMassPerPoint << " => R=" << (analysisInfo.AverageMassPerPoint - analysisInfo.AverageAirBuoyantMassPerPoint) << std::endl;
    std::cout << "  Equivalent water buoyant mass: " << analysisInfo.AverageWaterBuoyantMassPerPoint << " => R=" << (analysisInfo.AverageMassPerPoint - analysisInfo.AverageWaterBuoyantMassPerPoint) << std::endl;
    std::cout << "  Center of mass               : " << analysisInfo.CenterOfMass << std::endl;
    std::cout << "  Center of buoyancy           : " << analysisInfo.CenterOfDisplacedDensity << std::endl;
    std::cout << "  Momentum at Equilibrium      : " << analysisInfo.EquilibriumMomentum << std::endl;

    return 0;
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

    size_t frameCount;
    if (Utils::CaseInsensitiveEquals(databaseName, "cloud"))
    {
        frameCount = Baker::BakeAtlas<Render::CloudTextureDatabaseTraits>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "explosion"))
    {
        frameCount = Baker::BakeAtlas<Render::ExplosionTextureDatabaseTraits>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else if (Utils::CaseInsensitiveEquals(databaseName, "npc"))
    {
        frameCount = Baker::BakeAtlas<Render::NpcTextureDatabaseTraits>(
            texturesRootDirectoryPath,
            outputDirectoryPath,
            options);
    }
    else
    {
        throw std::runtime_error("Unrecognized database name '" + databaseName + "'");
    }

    std::cout << "Baking completed - " << frameCount << " frames." << std::endl;

    return 0;
}

int DoQuantize(int argc, char ** argv)
{
    if (argc < 5)
    {
        PrintUsage();
        return 0;
    }

    std::string materialsDirectory(argv[2]);
    std::string inputFile(argv[3]);
    std::string outputFile(argv[4]);

    bool doKeepRopes = false;
    bool doKeepGlass = false;
    std::optional<rgbColor> targetFixedColor;
    std::string targetFixedColorStr;
    for (int i = 5; i < argc; ++i)
    {
        std::string option(argv[i]);
        if (option == "-r" || option == "--keep_ropes")
        {
            doKeepRopes = true;
        }
        else if (option == "-g" || option == "--keep_glass")
        {
            doKeepGlass = true;
        }
        else if (option == "-c")
        {
            ++i;
            if (i == argc)
            {
                throw std::runtime_error("-c option specified without a color");
            }

            targetFixedColorStr = argv[i];
            targetFixedColor = Utils::Hex2RgbColor(targetFixedColorStr);
        }
        else
        {
            throw std::runtime_error("Unrecognized option '" + option + "'");
        }
    }

    std::cout << SEPARATOR << std::endl;
    std::cout << "Running quantize:" << std::endl;
    std::cout << "  input file    : " << inputFile << std::endl;
    std::cout << "  output file   : " << outputFile << std::endl;
    std::cout << "  materials dir : " << materialsDirectory << std::endl;
    std::cout << "  keep ropes    : " << doKeepRopes << std::endl;
    std::cout << "  keep glass    : " << doKeepGlass << std::endl;
    if (!!targetFixedColor)
        std::cout << "  target color  : " << targetFixedColorStr << std::endl;

    Quantizer::Quantize(
        inputFile,
        outputFile,
        materialsDirectory,
        doKeepRopes,
        doKeepGlass,
        targetFixedColor);

    std::cout << "Quantize completed." << std::endl;

    return 0;
}

int DoResize(int argc, char ** argv)
{
    if (argc < 5)
    {
        PrintUsage();
        return 0;
    }

    std::string inputFile(argv[2]);
    std::string outputFile(argv[3]);
    int width = std::stoi(argv[4]);

    std::cout << SEPARATOR << std::endl;
    std::cout << "Running resize:" << std::endl;
    std::cout << "  input file : " << inputFile << std::endl;
    std::cout << "  output file: " << outputFile << std::endl;
    std::cout << "  width      : " << width << std::endl;

    Resizer::Resize(inputFile, outputFile, width);

    std::cout << "Resize completed." << std::endl;

    return 0;
}

void PrintUsage()
{
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << " analyze <materials_dir> <in_file>" << std::endl;
    std::cout << " bake_atlas Cloud|Explosion|NPC <textures_root_dir> <out_dir> [[-a] [-b] [-m] [-r] | -o <options_json>]" << std::endl;
    std::cout << " quantize <materials_dir> <in_file> <out_png> [-c <target_fixed_color>]" << std::endl;
    std::cout << "          -r, --keep_ropes] [-g, --keep_glass]" << std::endl;
    std::cout << " resize <in_file> <out_png> <width>" << std::endl;
}