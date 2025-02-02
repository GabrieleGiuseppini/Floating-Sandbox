/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShaderManager.h"

#include <Core/GameException.h>

#include <regex>
#include <unordered_map>
#include <unordered_set>

template<typename TShaderSet>
ShaderManager<TShaderSet>::ShaderManager(IAssetManager const & assetManager)
{
    //
    // Load all shader files
    //

    // Shader name -> (isShader, source)
    std::unordered_map<std::string, std::pair<bool, std::string>> shaderSources;

    for (auto const & shaderDescriptor : assetManager.EnumerateShaders(TShaderSet::ShaderSetName))
    {
        assert(shaderSources.count(shaderDescriptor.Name) == 0); // Guaranteed by file system

        #define IncludeExtension ".glsl"
        shaderSources[shaderDescriptor.Name] = std::make_pair<bool, std::string>(
            shaderDescriptor.RelativePath.find(IncludeExtension, shaderDescriptor.RelativePath.size() - sizeof(IncludeExtension)) != std::string::npos,
            assetManager.LoadShader(TShaderSet::ShaderSetName, shaderDescriptor.RelativePath));
    }

    //
    // Compile all and only shader files (not includes)
    //

    for (auto const & entryIt : shaderSources)
    {
        if (entryIt.second.first) // Do not compile include files
        {
            CompileShader(
                entryIt.first,
                entryIt.second.second,
                shaderSources);
        }
    }

    //
    // Verify all expected programs have been loaded
    //

    for (uint32_t i = 0; i <= static_cast<uint32_t>(TShaderSet::ProgramKindType::_Last); ++i)
    {
        if (i >= mPrograms.size() || !(mPrograms[i].OpenGLHandle))
        {
            throw GameException("Cannot find GLSL source file for program \"" + TShaderSet::ProgramKindToStr(static_cast<typename TShaderSet::ProgramKindType>(i)) + "\"");
        }
    }
}

template<typename TShaderSet>
void ShaderManager<TShaderSet>::CompileShader(
    std::string const & shaderName,
    std::string const & shaderSource,
    std::unordered_map<std::string, std::pair<bool, std::string>> const & allShaderSources)
{
    try
    {
        // Get the program type
        typename TShaderSet::ProgramKindType const program = TShaderSet::ShaderNameToProgramKind(shaderName);
        std::string const programName = TShaderSet::ProgramKindToStr(program);

        // Make sure we have room for it
        size_t programIndex = static_cast<size_t>(program);
        if (programIndex + 1 > mPrograms.size())
        {
            mPrograms.resize(programIndex + 1);
        }

        // First time we see it (guaranteed by file system)
        assert(!(mPrograms[programIndex].OpenGLHandle));

        // Resolve includes
        std::string preprocessedShaderSource = ResolveIncludes(
            shaderSource,
            allShaderSources);

        // Split the source file
        auto [vertexShaderSource, fragmentShaderSource] = SplitSource(preprocessedShaderSource);


        //
        // Create program
        //

        mPrograms[programIndex].OpenGLHandle = glCreateProgram();
        CheckOpenGLError();


        //
        // Compile vertex shader
        //

        GameOpenGL::CompileShader(
            vertexShaderSource,
            GL_VERTEX_SHADER,
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Compile fragment shader
        //

        GameOpenGL::CompileShader(
            fragmentShaderSource,
            GL_FRAGMENT_SHADER,
            mPrograms[programIndex].OpenGLHandle,
            programName);


        //
        // Link a first time, to enable extraction of attributes and uniforms
        //

        GameOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, programName);


        //
        // Extract attribute names from vertex shader and bind them
        //

        std::set<std::string> vertexAttributeNames = ExtractVertexAttributeNames(mPrograms[programIndex].OpenGLHandle);

        for (auto const & vertexAttributeName : vertexAttributeNames)
        {
            auto vertexAttribute = TShaderSet::StrToVertexAttributeKind(vertexAttributeName);

            GameOpenGL::BindAttributeLocation(
                mPrograms[programIndex].OpenGLHandle,
                static_cast<GLuint>(vertexAttribute),
                "in" + vertexAttributeName);
        }


        //
        // Link a second time, to freeze vertex attribute binding
        //

        GameOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, programName);


        //
        // Extract uniform locations
        //

        std::vector<GLint> uniformLocations;

        std::set<std::string> parameterNames = ExtractParameterNames(mPrograms[programIndex].OpenGLHandle);

        for (auto const & parameterName : parameterNames)
        {
            typename TShaderSet::ProgramParameterKindType programParameter = TShaderSet::StrToProgramParameterKind(parameterName);
            size_t programParameterIndex = static_cast<size_t>(programParameter);

            //
            // Store uniform location
            //

            // Make sure there is room
            if (mPrograms[programIndex].UniformLocations.size() <= programParameterIndex)
            {
                mPrograms[programIndex].UniformLocations.resize(programParameterIndex + 1, NoParameterLocation);
            }

            // Get and store
            mPrograms[programIndex].UniformLocations[programParameterIndex] = GameOpenGL::GetParameterLocation(
                mPrograms[programIndex].OpenGLHandle,
                "param" + TShaderSet::ProgramParameterKindToStr(programParameter));

            //
            // Store in ProgramParameter->Program index
            //

            // Make sure there is room
            if (mProgramsByProgramParameter.size() <= programParameterIndex)
            {
                mProgramsByProgramParameter.resize(programParameterIndex + 1);
            }

            mProgramsByProgramParameter[programParameterIndex].push_back(program);
        }
    }
    catch (GameException const & ex)
    {
        throw GameException("Error compiling shader file \"" + shaderName + "\": " + ex.what());
    }
}

template<typename TShaderSet>
std::string ShaderManager<TShaderSet>::ResolveIncludes(
    std::string const & shaderSource,
    std::unordered_map<std::string, std::pair<bool, std::string>> const & shaderSources)
{
    /*
     * Strategy:
     * - We treat each include as if having #pragma once
     * - We resolve includes depth-first, so that a declaration from a source file included multiple times
     *   is inserted at the earliest location in the include chain
     */

    static std::regex const IncludeRegex(R"!(^\s*#include\s+\"\s*([_a-zA-Z0-9\.]+)\s*\"\s*$)!");

    std::unordered_set<std::string> resolvedIncludes;

    std::string resolvedSource = shaderSource;

    for (bool hasResolved = true; hasResolved; )
    {
        std::stringstream sSource(resolvedSource);
        std::stringstream sSubstitutedSource;

        hasResolved = false;

        std::string line;
        while (std::getline(sSource, line))
        {
            std::smatch match;
            if (std::regex_search(line, match, IncludeRegex))
            {
                //
                // Found an include
                //

                assert(2 == match.size());

                auto includeFilename = match[1].str();
                auto includeIt = shaderSources.find(includeFilename);
                if (includeIt == shaderSources.end())
                {
                    throw GameException("Cannot find include file \"" + includeFilename + "\"");
                }

                // Check whether we've included this one already
                if (resolvedIncludes.count(includeFilename) == 0)
                {
                    // Insert include
                    sSubstitutedSource << includeIt->second.second << sSource.widen('\n');

                    // Remember the files we've included in this path
                    resolvedIncludes.insert(includeFilename);

                    // Append rest of source file
                    while (std::getline(sSource, line))
                    {
                        sSubstitutedSource << line << sSource.widen('\n');
                    }

                    // Remember we've included something
                    hasResolved = true;

                    // Restart from scratch (to enforce depth-first)
                    break;
                }
            }
            else
            {
                sSubstitutedSource << line << sSource.widen('\n');
            }
        }

        resolvedSource = sSubstitutedSource.str();
    }

    return resolvedSource;
}

template<typename TShaderSet>
std::tuple<std::string, std::string> ShaderManager<TShaderSet>::SplitSource(std::string const & source)
{
    static std::regex const VertexHeaderRegex(R"!(\s*###VERTEX-(\d{3})\s*)!");
    static std::regex const FragmentHeaderRegex(R"!(\s*###FRAGMENT-(\d{3})\s*)!");

    std::stringstream sSource(source);

    std::string line;

    std::stringstream commonCode;
    std::stringstream vertexShaderCode;
    std::stringstream fragmentShaderCode;

    //
    // Common code
    //

    while (true)
    {
        if (!std::getline(sSource, line))
            throw GameException("Cannot find ###VERTEX declaration");

        std::smatch match;
        if (std::regex_search(line, match, VertexHeaderRegex))
        {
            // Found beginning of vertex shader

            // Initialize vertex shader GLSL version
            vertexShaderCode << "#version " << match[1].str() << sSource.widen('\n');

            // Initialize vertex shader with common code
            vertexShaderCode << commonCode.str();

            break;
        }
        else
        {
            commonCode << line << sSource.widen('\n');
        }
    }

    //
    // Vertex shader
    //

    while (true)
    {
        if (!std::getline(sSource, line))
            throw GameException("Cannot find ###FRAGMENT declaration");

        std::smatch match;
        if (std::regex_search(line, match, FragmentHeaderRegex))
        {
            // Found beginning of fragment shader

            // Initialize fragment shader GLSL version
            fragmentShaderCode << "#version " << match[1].str() << sSource.widen('\n');

            // Initialize fragment shader with common code
            fragmentShaderCode << commonCode.str();

            break;
        }
        else
        {
            vertexShaderCode << line << sSource.widen('\n');
        }
    }

    //
    // Fragment shader
    //

    while (std::getline(sSource, line))
    {
        fragmentShaderCode << line << sSource.widen('\n');
    }


    return std::make_tuple(
        vertexShaderCode.str(),
        fragmentShaderCode.str());
}

template<typename TShaderSet>
std::set<std::string> ShaderManager<TShaderSet>::ExtractVertexAttributeNames(GameOpenGLShaderProgram const & shaderProgram)
{
    std::set<std::string> attributeNames;

    GLint count;
    glGetProgramiv(*shaderProgram, GL_ACTIVE_ATTRIBUTES, &count);

    for (GLuint i = 0; i < static_cast<GLuint>(count); ++i)
    {
        char nameBuffer[256];
        GLsizei nameLength;
        GLint attributeSize;
        GLenum attributeType;

        glGetActiveAttrib(
            *shaderProgram,
            i,
            static_cast<GLsizei>(sizeof(nameBuffer)),
            &nameLength,
            &attributeSize,
            &attributeType,
            nameBuffer);
        CheckOpenGLError();

        if (nameLength < 2 || strncmp(nameBuffer, "in", 2))
        {
            throw GameException("Attribute name \"" + std::string(nameBuffer, nameLength) + "\" does not follow the expected name structure: missing \"in\" prefix");
        }

        std::string const attributeName(nameBuffer + 2, nameLength - 2);

        // Lookup the attribute name - just as a sanity check
        TShaderSet::StrToVertexAttributeKind(attributeName);

        // Store it, making sure it's not specified more than once
        if (!attributeNames.insert(attributeName).second)
        {
            throw GameException("Attribute name \"" + attributeName + "\" is declared more than once");
        }
    }

    return attributeNames;
}

template<typename TShaderSet>
std::set<std::string> ShaderManager<TShaderSet>::ExtractParameterNames(GameOpenGLShaderProgram const & shaderProgram)
{
    static std::regex const ArrayParameterRegEx(R"!(^(.+)\[[0-9]+\]$)!");
    static char constexpr ParamPrefix[5] = { 'p', 'a', 'r', 'a', 'm' };

    std::set<std::string> parameterNames;

    GLint count;
    glGetProgramiv(*shaderProgram, GL_ACTIVE_UNIFORMS, &count);

    for (GLuint i = 0; i < static_cast<GLuint>(count); ++i)
    {
        char nameBuffer[256];
        GLsizei nameLength;
        GLint attributeSize;
        GLenum attributeType;

        glGetActiveUniform(
            *shaderProgram,
            i,
            static_cast<GLsizei>(sizeof(nameBuffer)),
            &nameLength,
            &attributeSize,
            &attributeType,
            nameBuffer);
        CheckOpenGLError();

        if (static_cast<size_t>(nameLength) < sizeof(ParamPrefix) || strncmp(nameBuffer, ParamPrefix, sizeof(ParamPrefix)))
        {
            throw GameException("Uniform name \"" + std::string(nameBuffer, nameLength) + "\" does not follow the expected name structure: missing \"param\" prefix");
        }

        // Remove "param" prefix
        std::string parameterName(nameBuffer + sizeof(ParamPrefix), nameLength - sizeof(ParamPrefix));

        // Check if it's an array (element)
        std::smatch arrayParameterMatch;
        if (std::regex_match(parameterName, arrayParameterMatch, ArrayParameterRegEx))
        {
            // Remove suffix
            assert(arrayParameterMatch.size() == 1 + 1);
            parameterName = arrayParameterMatch[1].str();
        }

        // Lookup the parameter name - just as a sanity check
        TShaderSet::StrToProgramParameterKind(parameterName);

        // Store it, making sure it's not specified more than once
        if (!parameterNames.insert(parameterName).second
            && arrayParameterMatch.empty())
        {
            throw GameException("Uniform name \"" + parameterName + "\" is declared more than once");
        }
    }

    return parameterNames;
}