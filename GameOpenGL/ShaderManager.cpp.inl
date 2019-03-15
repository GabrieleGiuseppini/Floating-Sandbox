/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderManager.h"

#include <GameCore/GameException.h>
#include <GameCore/Utils.h>

#include <regex>

static const std::string StaticParametersFilenameStem = "static_parameters";

template<typename Traits>
ShaderManager<Traits>::ShaderManager(
    std::filesystem::path const & shadersRoot)
{
    if (!std::filesystem::exists(shadersRoot))
        throw GameException("Shaders root path \"" + shadersRoot.string() + "\" does not exist");

    //
    // Make static parameters
    //

    std::map<std::string, std::string> staticParameters;

    // 1) From file
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

    for (uint32_t i = 0; i <= static_cast<uint32_t>(Traits::ProgramType::_Last); ++i)
    {
        if (i >= mPrograms.size() || !(mPrograms[i].OpenGLHandle))
        {
            throw GameException("Cannot find GLSL source file for program \"" + Traits::ProgramTypeToStr(static_cast<Traits::ProgramType>(i)) + "\"");
        }
    }
}

template<typename Traits>
void ShaderManager<Traits>::CompileShader(
    std::filesystem::path const & shaderFilepath,
    std::map<std::string, std::string> const & staticParameters)
{
    // Load the source file
    std::string shaderSource = Utils::LoadTextFile(shaderFilepath);

    try
    {
        // Get the program type
        Traits::ProgramType const program = Traits::ShaderFilenameToProgramType(shaderFilepath.stem().string());
        std::string const programName = Traits::ProgramTypeToStr(program);

        // Make sure we have room for it
        size_t programIndex = static_cast<size_t>(program);
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
        CheckOpenGLError();


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
        // Extract attribute names from vertex shader and bind them
        //

        std::set<std::string> vertexAttributeNames = ExtractVertexAttributeNames(vertexShaderSource);

        for (auto const & vertexAttributeName : vertexAttributeNames)
        {
            auto vertexAttribute = Traits::StrToVertexAttributeType(vertexAttributeName);

            GameOpenGL::BindAttributeLocation(
                mPrograms[programIndex].OpenGLHandle,
                static_cast<GLuint>(vertexAttribute),
                "in" + vertexAttributeName);
        }


        //
        // Link
        //

        GameOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, programName);


        //
        // Extract uniform locations
        //

        std::vector<GLint> uniformLocations;

        auto allProgramParameters = ExtractShaderParameters(vertexShaderSource);
        auto fragmentShaderParameters = ExtractShaderParameters(fragmentShaderSource);
        allProgramParameters.merge(fragmentShaderParameters);

        for (Traits::ProgramParameterType programParameter : allProgramParameters)
        {
            // Make sure there is room
            size_t programParameterIndex = static_cast<size_t>(programParameter);
            while (mPrograms[programIndex].UniformLocations.size() <= programParameterIndex)
            {
                mPrograms[programIndex].UniformLocations.push_back(-1);
            }

            // Get and store
            mPrograms[programIndex].UniformLocations[programParameterIndex] = GameOpenGL::GetParameterLocation(
                mPrograms[programIndex].OpenGLHandle,
                "param" + Traits::ProgramParameterTypeToStr(programParameter));
        }
    }
    catch (GameException const & ex)
    {
        throw GameException("Error compiling shader file \"" + shaderFilepath.filename().string() + "\": " + ex.what());
    }
}

template<typename Traits>
std::tuple<std::string, std::string> ShaderManager<Traits>::SplitSource(std::string const & source)
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

template<typename Traits>
void ShaderManager<Traits>::ParseLocalStaticParameters(
    std::string const & localStaticParametersSource,
    std::map<std::string, std::string> & staticParameters)
{
    static std::regex StaticParamDefinitionRegex(R"!(^\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*=\s*(.*?)\s*$)!");

    std::stringstream sSource(localStaticParametersSource);
    std::string line;
    while (std::getline(sSource, line))
    {
        line = Utils::Trim(line);

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

template<typename Traits>
std::string ShaderManager<Traits>::SubstituteStaticParameters(
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

template<typename Traits>
std::set<typename Traits::ProgramParameterType> ShaderManager<Traits>::ExtractShaderParameters(std::string const & source)
{
    static std::regex ShaderParamNameRegex(R"!((//\s*)?\buniform\s+.*?\s+param([_a-zA-Z0-9]*);)!");

    std::set<Traits::ProgramParameterType> shaderParameters;

    std::string remainingSource = source;
    std::smatch match;
    while (std::regex_search(remainingSource, match, ShaderParamNameRegex))
    {
        assert(3 == match.size());
        if (!match[1].matched)
        {
            auto const & shaderParameterName = match[2].str();

            // Lookup the parameter
            Traits::ProgramParameterType shaderParameter = Traits::StrToProgramParameterType(shaderParameterName);

            // Store it, making sure it's not specified more than once
            if (!shaderParameters.insert(shaderParameter).second)
            {
                throw GameException("Shader parameter \"" + shaderParameterName + "\" is declared more than once");
            }
        }

        // Advance
        remainingSource = match.suffix();
    }

    return shaderParameters;
}

template<typename Traits>
std::set<std::string> ShaderManager<Traits>::ExtractVertexAttributeNames(std::string const & source)
{
    static std::regex AttributeNameRegex(R"!(\bin\s+.*?\s+in([_a-zA-Z][_a-zA-Z0-9]*);)!");

    std::set<std::string> attributeNames;

    std::string remainingSource = source;
    std::smatch match;
    while (std::regex_search(remainingSource, match, AttributeNameRegex))
    {
        assert(2 == match.size());
        auto const & attributeName = match[1].str();

        // Lookup the attribute name - just as a sanity check
        Traits::StrToVertexAttributeType(attributeName);

        // Store it, making sure it's not specified more than once
        if (!attributeNames.insert(attributeName).second)
        {
            throw GameException("Attribute name \"" + attributeName + "\" is declared more than once");
        }

        // Advance
        remainingSource = match.suffix();
    }

    return attributeNames;
}