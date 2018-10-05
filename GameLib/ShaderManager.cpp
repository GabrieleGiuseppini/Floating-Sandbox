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

namespace /* anonymous */ {

    ShaderManager::ProgramType StrToProgramType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);
        if (lstr == "clouds")
            return ShaderManager::ProgramType::Clouds;
        else if (lstr == "water")
            return ShaderManager::ProgramType::Water;
        else
            throw GameException("Unrecognized ProgramType \"" + str + "\"");
    }

    std::string ProgramTypeToStr(ShaderManager::ProgramType programType)
    {
        switch (programType)
        {
        case ShaderManager::ProgramType::Clouds:
            return "Clouds";
        case ShaderManager::ProgramType::Water:
            return "Water";
        }
    }

    ShaderManager::ParameterType StrToParameterType(std::string const & str)
    {
        std::string lstr = Utils::ToLower(str);
        if (lstr == "AmbientLightIntensity")
            return ShaderManager::ParameterType::AmbientLightIntensity;
        else
            throw GameException("Unrecognized ParameterType \"" + str + "\"");
    }

    std::string ParameterTypeToStr(ShaderManager::ParameterType parameterType)
    {
        switch (parameterType)
        {
        case ShaderManager::ParameterType::AmbientLightIntensity:
            return "AmbientLightIntensity";
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
    // TODOHERE


    //
    // Enumerate and compile all shader files
    //

    for (auto const & entryIt : std::filesystem::directory_iterator(shadersRoot / "*.glsl"))
    {
        if (std::filesystem::is_regular_file(entryIt.path()))
        {
            CompileShader(entryIt.path(), staticParameters);
        }
    }

    //
    // Verify all programs have been loaded
    //

    // TODO
}

void ShaderManager::CompileShader(
    std::filesystem::path const & shaderFilepath,
    std::map<std::string, std::string> const & staticParameters)
{
    // Get the program
    ProgramType programType = StrToProgramType(shaderFilepath.stem().string());

    // TODO: assert first time we see it

    // Load the source file
    std::string shaderSource = Utils::LoadTextFile(shaderFilepath);

    try
    {
        // Split the source file
        auto [vertexShaderSource, fragmentShaderSource] = SplitSource(shaderSource);

        //
        // Compile vertex shader
        //

        vertexShaderSource = SubstituteStaticParameters(vertexShaderSource, staticParameters);

        auto vertexParameters = ExtractDynamicParameters(vertexShaderSource);

        // TODOHERE

        //
        // Compile fragment shader
        //

        fragmentShaderSource = SubstituteStaticParameters(fragmentShaderSource, staticParameters);

        auto fragmentParameters = ExtractDynamicParameters(fragmentShaderSource);

        // TODOHERE
    }
    catch (GameException const & ex)
    {
        throw GameException("Error compiling shader file \"" + shaderFilepath.filename().string() + "\": " + ex.what());
    }
}

std::tuple<std::string, std::string> ShaderManager::SplitSource(std::string const & source)
{
    static std::regex VertexHeaderRegex(R"!(\s*\*\*\*VERTEX\s*)!");
    static std::regex FragmentHeaderRegex(R"!(\s*\*\*\*FRAGMENT\s*)!");

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
    static std::regex StaticParamDefinitionRegex(R"!(^\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*=\s*([^\s]*)\s*$)!");

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

std::set<ShaderManager::ParameterType> ShaderManager::ExtractDynamicParameters(std::string const & source)
{
    // TODO: make it multiline somehow
    static std::regex ParamNameRegex(R"!(^\s*uniform\s+.?\s+param([_a-zA-Z0-9]*);\s*$)!");

    // TODOHERE: parse all tokens 
    // 
    // THEN: verify known param
    // THEN: add to set

    // TODO
    return std::set<ShaderManager::ParameterType>();
}