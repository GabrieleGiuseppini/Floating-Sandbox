/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include "Quantizer.h"
#include "Resizer.h"
#include "ShipAnalyzer.h"

#include <GameLib/Utils.h>

#include <IL/il.h>
#include <IL/ilu.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#define SEPARATOR "------------------------------------------------------"

int DoQuantize(int argc, char ** argv);
int DoResize(int argc, char ** argv);
int DoAnalyzeShip(int argc, char ** argv);

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
        if (verb == "quantize")
        {
            return DoQuantize(argc, argv);
        }
        else if (verb == "resize")
        {
            return DoResize(argc, argv);
        }
        else if (verb == "analyze")
        {
            return DoAnalyzeShip(argc, argv);
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

int DoQuantize(int argc, char ** argv)
{
    if (argc < 5)
    {
        PrintUsage();
        return 0;
    }

    std::string materialsFile(argv[2]);
    std::string inputFile(argv[3]);
    std::string outputFile(argv[4]);    

    bool doKeepRopes = false;
    bool doKeepGlass = false;
    std::optional<std::array<uint8_t, 3u>> targetFixedColor;
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
            targetFixedColor = Utils::Hex2RgbColour(targetFixedColorStr);
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
    std::cout << "  materials file: " << materialsFile << std::endl;
    std::cout << "  keep ropes    : " << doKeepRopes << std::endl;
    std::cout << "  keep glass    : " << doKeepGlass << std::endl;
    if (!!targetFixedColor)
        std::cout << "  target color  : " << targetFixedColorStr << std::endl;

    Quantizer::Quantize(
        inputFile, 
        outputFile, 
        materialsFile, 
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

int DoAnalyzeShip(int argc, char ** argv)
{
    if (argc < 4)
    {
        PrintUsage();
        return 0;
    }

    std::string materialsFile(argv[2]);
    std::string inputFile(argv[3]);    

    auto analysisInfo = ShipAnalyzer::Analyze(inputFile, materialsFile);

    std::cout << std::fixed;

    std::cout << "  Total mass             : " << analysisInfo.TotalMass << std::endl;
    std::cout << "  Equivalent mass        : " << analysisInfo.MassPerPoint<< std::endl;
    std::cout << "  Equivalent buoyant mass: " << analysisInfo.BuoyantMassPerPoint << std::endl;
    std::cout << "  Center of mass         : " << analysisInfo.BaricentricX << "," << analysisInfo.BaricentricY << std::endl;

    return 0;
}

void PrintUsage()
{
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << " quantize <materials_file> <in_file> <out_png> [-c <target_fixed_color>]" << std::endl;
    std::cout << "          -r, --keep_ropes] [-g, --keep_glass]" << std::endl;
    std::cout << " resize <in_file> <out_png> <width>" << std::endl;
    std::cout << " analyze <materials_file> <in_file>" << std::endl;
}
