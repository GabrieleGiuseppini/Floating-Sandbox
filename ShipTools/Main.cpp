/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-06-25
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include "Quantizer.h"
#include "Resizer.h"

#include <IL/il.h>
#include <IL/ilu.h>

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#define SEPARATOR "------------------------------------------------------"

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
        if (verb == "quantize")
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

int DoQuantize(int argc, char ** argv)
{
    if (argc < 5)
    {
        PrintUsage();
        return 0;
    }

    std::string inputFile(argv[2]);
    std::string outputFile(argv[3]);
    std::string materialsFile(argv[4]);

    bool doKeepRopes = false;
    bool doKeepGlass = false;
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

    Quantizer::Quantize(inputFile, outputFile, materialsFile, doKeepRopes, doKeepGlass);

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
    std::cout << " quantize <in_file> <out_png> <materials_file> [-r, --keep_ropes]" << std::endl;
    std::cout << "      -r, --keep_ropes" << std::endl;
    std::cout << "      -g, --keep_glass" << std::endl;
    std::cout << " resize <in_file> <out_png> <width>" << std::endl;
}
