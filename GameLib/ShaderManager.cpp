/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderManager.h"

#include "GameException.h"
#include "ResourceLoader.h"
#include "Utils.h"

#include <regex>

static const std::string StaticParametersFilenameStem = "static_parameters";

namespace /* anonymous */ {

    ShaderManager::ProgramType StrToProgramType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);
        if (lstr == "clouds")
            return ShaderManager::ProgramType::Clouds;
        else if (lstr == "generic_textures")
            return ShaderManager::ProgramType::GenericTextures;
        else if (lstr == "land")
            return ShaderManager::ProgramType::Land;
        else if (lstr == "matte")
            return ShaderManager::ProgramType::Matte;
        else if (lstr == "matte_ndc")
            return ShaderManager::ProgramType::MatteNDC;
        else if (lstr == "ship_ropes")
            return ShaderManager::ProgramType::ShipRopes;
        else if (lstr == "ship_stressed_springs")
            return ShaderManager::ProgramType::ShipStressedSprings;
        else if (lstr == "ship_triangles_color")
            return ShaderManager::ProgramType::ShipTrianglesColor;
        else if (lstr == "ship_triangles_texture")
            return ShaderManager::ProgramType::ShipTrianglesTexture;
        else if (lstr == "vector_arrows")
            return ShaderManager::ProgramType::VectorArrows;
        else if (lstr == "water")
            return ShaderManager::ProgramType::Water;
        else
            throw GameException("Unrecognized program \"" + str + "\"");
    }

    std::string ProgramTypeToStr(ShaderManager::ProgramType programType)
    {
        switch (programType)
        {
        case ShaderManager::ProgramType::Clouds:
            return "Clouds";
        case ShaderManager::ProgramType::GenericTextures:
            return "GenericTextures";
        case ShaderManager::ProgramType::Land:
            return "Land";
        case ShaderManager::ProgramType::Matte:
            return "Matte";
        case ShaderManager::ProgramType::MatteNDC:
            return "MatteNDC";
        case ShaderManager::ProgramType::ShipRopes:
            return "ShipRopes";
        case ShaderManager::ProgramType::ShipStressedSprings:
            return "ShipStressedSprings";
        case ShaderManager::ProgramType::ShipTrianglesColor:
            return "ShipTrianglesColor";
        case ShaderManager::ProgramType::ShipTrianglesTexture:
            return "ShipTrianglesTexture";
        case ShaderManager::ProgramType::VectorArrows:
            return "VectorArrows";
        case ShaderManager::ProgramType::Water:
            return "Water";
        default:
            assert(false);
            throw GameException("Unsupported ProgramType");
        }
    }

    ShaderManager::DynamicParameterType StrToDynamicParameterType(std::string const & str)
    {
        if (str == "AmbientLightIntensity")
            return ShaderManager::DynamicParameterType::AmbientLightIntensity;
        else if (str == "MatteColor")
            return ShaderManager::DynamicParameterType::MatteColor;
        else if (str == "OrthoMatrix")
            return ShaderManager::DynamicParameterType::OrthoMatrix;
        else if (str == "TextureScaling")
            return ShaderManager::DynamicParameterType::TextureScaling;
        else if (str == "WaterLevelThreshold")
            return ShaderManager::DynamicParameterType::WaterLevelThreshold;
        else if (str == "WaterTransparency")
            return ShaderManager::DynamicParameterType::WaterTransparency;
        else
            throw GameException("Unrecognized dynamic parameter \"" + str + "\"");
    }

    std::string DynamicParameterTypeToStr(ShaderManager::DynamicParameterType dynamicParameterType)
    {
        switch (dynamicParameterType)
        {
        case ShaderManager::DynamicParameterType::AmbientLightIntensity:
            return "AmbientLightIntensity";
        case ShaderManager::DynamicParameterType::MatteColor:
            return "MatteColor";
        case ShaderManager::DynamicParameterType::OrthoMatrix:
            return "OrthoMatrix";
        case ShaderManager::DynamicParameterType::TextureScaling:
            return "TextureScaling";
        case ShaderManager::DynamicParameterType::WaterLevelThreshold:
            return "WaterLevelThreshold";
        case ShaderManager::DynamicParameterType::WaterTransparency:
            return "WaterTransparency";
        default:
            assert(false);
            throw GameException("Unsupported DynamicParameterType");
        }
    }
}

std::unique_ptr<ShaderManager> ShaderManager::CreateInstance(
    ResourceLoader & resourceLoader,
    GlobalParameters const & globalParameters)
{
    return CreateInstance(resourceLoader.GetShadersRootPath(), globalParameters);
}

std::unique_ptr<ShaderManager> ShaderManager::CreateInstance(
    std::filesystem::path const & shadersRoot,
    GlobalParameters const & globalParameters)
{
    return std::unique_ptr<ShaderManager>(
        new ShaderManager(shadersRoot, globalParameters));
}

ShaderManager::ShaderManager(
    std::filesystem::path const & shadersRoot,
    GlobalParameters const & globalParameters)
{
    //
    // Make static parameters
    //

    std::map<std::string, std::string> staticParameters;

    // 1) From global parameters
    globalParameters.ToParameters(staticParameters);

    // 2) From file
    std::filesystem::path localStaticParametersFilepath = shadersRoot / (StaticParametersFilenameStem + ".glsl");
    if (std::filesystem::exists(localStaticParametersFilepath))
    {
        std::string localStaticParametersSource = Utils::LoadTextFile(localStaticParametersFilepath);
        ParseLocalStaticParameters(localStaticParametersSource, staticParameters);
    }


    //
    // Enumerate and compile all shader files
    //

    for (auto const & entryIt : std::filesystem::directory_iterator(shadersRoot))
    {
        if (std::filesystem::is_regular_file(entryIt.path())
            && entryIt.path().extension() == ".glsl"
            && entryIt.path().stem() != StaticParametersFilenameStem)
        {
            CompileShader(entryIt.path(), staticParameters);
        }
    }

    //
    // Verify all programs have been loaded
    //

    for (uint32_t i = 0; i <= static_cast<uint32_t>(ProgramType::_Last); ++i)
    {
        if (i >= mPrograms.size() || !(mPrograms[i].OpenGLHandle))
        {
            throw GameException("Cannot find GLSL source file for program \"" + ProgramTypeToStr(static_cast<ProgramType>(i)) + "\"");
        }
    }
}

void ShaderManager::CompileShader(
    std::filesystem::path const & shaderFilepath,
    std::map<std::string, std::string> const & staticParameters)
{
    // Load the source file
    std::string shaderSource = Utils::LoadTextFile(shaderFilepath);

    try
    {
        // Get the program type
        ProgramType const programType = StrToProgramType(shaderFilepath.stem().string());
        std::string const programName = ProgramTypeToStr(programType);

        // Make sure we have room for it
        size_t programIndex = static_cast<size_t>(programType);
        if (programIndex + 1 > mPrograms.size())
        {
            mPrograms.resize(programIndex + 1);
        }

        // First time we see it (guaranteed by file system)
        assert(!(mPrograms[programIndex].OpenGLHandle));

        // Split the source file
        auto [vertexShaderSource, fragmentShaderSource] = SplitSource(shaderSource);

        // Create program
        mPrograms[programIndex].OpenGLHandle = glCreateProgram();


        //
        // Compile vertex shader
        //

        vertexShaderSource = SubstituteStaticParameters(vertexShaderSource, staticParameters);

        GameOpenGL::CompileShader(
            vertexShaderSource, 
            GL_VERTEX_SHADER, 
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Compile fragment shader
        //

        fragmentShaderSource = SubstituteStaticParameters(fragmentShaderSource, staticParameters);

        GameOpenGL::CompileShader(
            fragmentShaderSource, 
            GL_FRAGMENT_SHADER, 
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Link
        //

        GameOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, programName);


        //
        // Get uniform locations
        //

        std::vector<GLint> uniformLocations;

        auto allDynamicParameters = ExtractDynamicParameters(vertexShaderSource);
        auto fragmentDynamicParameters = ExtractDynamicParameters(fragmentShaderSource);
        allDynamicParameters.merge(fragmentDynamicParameters);

        for (DynamicParameterType dynamicParameterType : allDynamicParameters)
        {
            // Make sure there is room
            size_t dynamicParameterIndex = static_cast<size_t>(dynamicParameterType);
            if (dynamicParameterIndex + 1 > mPrograms[programIndex].UniformLocations.size())
            {
                mPrograms[programIndex].UniformLocations.resize(dynamicParameterIndex + 1);
            }

            // Get and store
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex] = GameOpenGL::GetParameterLocation(
                mPrograms[programIndex].OpenGLHandle,
                "param" + DynamicParameterTypeToStr(dynamicParameterType));
        }
    }
    catch (GameException const & ex)
    {
        throw GameException("Error compiling shader file \"" + shaderFilepath.filename().string() + "\": " + ex.what());
    }
}

std::tuple<std::string, std::string> ShaderManager::SplitSource(std::string const & source)
{
    static std::regex VertexHeaderRegex(R"!(\s*###VERTEX\s*)!");
    static std::regex FragmentHeaderRegex(R"!(\s*###FRAGMENT\s*)!");

    std::stringstream sSource(source);

    std::string line;

    //
    // Vertex shader
    //

    // Eat blank lines
    while (true)
    {
        if (!std::getline(sSource, line))
        {
            throw GameException("Cannot find ***VERTEX declaration");
        }

        if (!line.empty())
        {
            if (!std::regex_match(line, VertexHeaderRegex))
            {
                throw GameException("Cannot find ***VERTEX declaration");
            }

            break;
        }
    }

    std::stringstream vertexShader;
    
    while (true)
    {
        if (!std::getline(sSource, line))
            throw GameException("Cannot find ***FRAGMENT declaration");

        if (std::regex_match(line, FragmentHeaderRegex))
            break;

        vertexShader << line << sSource.widen('\n');
    }


    //
    // Fragment shader
    //

    std::stringstream fragmentShader;

    while (std::getline(sSource, line))
    {
        fragmentShader << line << sSource.widen('\n');
    }


    return std::make_tuple(vertexShader.str(), fragmentShader.str());
}

void ShaderManager::ParseLocalStaticParameters(
    std::string const & localStaticParametersSource,
    std::map<std::string, std::string> & staticParameters)
{
    static std::regex StaticParamDefinitionRegex(R"!(^\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*=(.*)$)!");

    std::stringstream sSource(localStaticParametersSource);
    std::string line;
    while (std::getline(sSource, line))
    {
        if (!line.empty())
        {
            std::smatch match;
            if (!std::regex_search(line, match, StaticParamDefinitionRegex))
            {
                throw GameException("Error parsing static parameter definition \"" + line + "\"");
            }

            assert(3 == match.size());
            auto staticParameterName = match[1].str();
            auto staticParameterValue = match[2].str();

            // Check whether it's a dupe
            if (staticParameters.count(staticParameterName) > 0)
            {
                throw GameException("Static parameters \"" + staticParameterName + "\" has already been defined");
            }

            // Store
            staticParameters.insert(
                std::make_pair(
                    staticParameterName,
                    staticParameterValue));
        }
    }
}

std::string ShaderManager::SubstituteStaticParameters(
    std::string const & source,
    std::map<std::string, std::string> const & staticParameters)
{
    static std::regex StaticParamNameRegex("%([_a-zA-Z][_a-zA-Z0-9]*)%");

    std::string remainingSource = source;
    std::stringstream sSubstitutedSource;
    std::smatch match;
    while (std::regex_search(remainingSource, match, StaticParamNameRegex))
    {
        assert(2 == match.size());
        auto staticParameterName = match[1].str();
        
        // Lookup the parameter
        auto const & paramIt = staticParameters.find(staticParameterName);
        if (paramIt == staticParameters.end())
        {
            throw GameException("Static parameter \"" + staticParameterName + "\" is not recognized");
        }

        // Substitute the parameter
        sSubstitutedSource << match.prefix();
        sSubstitutedSource << paramIt->second;

        // Advance
        remainingSource = match.suffix();
    }

    sSubstitutedSource << remainingSource;

    return sSubstitutedSource.str();
}

std::set<ShaderManager::DynamicParameterType> ShaderManager::ExtractDynamicParameters(std::string const & source)
{
    static std::regex DynamicParamNameRegex(R"!(\buniform\s+.*?\s+param([_a-zA-Z0-9]*);)!");

    std::set<ShaderManager::DynamicParameterType> dynamicParameters;

    std::string remainingSource = source;
    std::smatch match;
    while (std::regex_search(remainingSource, match, DynamicParamNameRegex))
    {
        assert(2 == match.size());
        auto dynamicParameterName = match[1].str();

        // Lookup the parameter
        DynamicParameterType dynamicParameterType = StrToDynamicParameterType(dynamicParameterName);

        // Store it, making sure it's not specified more than once
        if (!dynamicParameters.insert(dynamicParameterType).second)
        {
            throw GameException("Dynamic parameter \"" + DynamicParameterTypeToStr(dynamicParameterType) + "\" is declared more than once");
        }

        // Advance
        remainingSource = match.suffix();
    }

    return dynamicParameters;
}