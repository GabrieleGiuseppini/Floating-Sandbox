/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"
#include "ResourceLoader.h"
#include "Vectors.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

namespace Render {

template <typename Traits>
class ShaderManager
{
private:

    template <typename T>
    static std::string ToString(T const & v)
    {
        return std::to_string(v);
    }

    template <>
    static std::string ToString(float const & v)
    {
        std::stringstream stream;
        stream << std::fixed << v;
        return stream.str();
    }

public:

    struct GlobalParameters
    {
        vec4f RopeColor;

        GlobalParameters(
            vec4f ropeColor)
            : RopeColor(ropeColor)
        {}

        void ToParameters(std::map<std::string, std::string> & parameters) const
        {
            std::stringstream ss;
            ss << std::fixed << RopeColor.x << ", " << RopeColor.y << ", " << RopeColor.z << ", " << RopeColor.w;

            parameters.insert(
                std::make_pair(
                    "ROPE_COLOR_VEC4",
                    ss.str()));
        }
    };

public:

    static std::unique_ptr<ShaderManager> CreateInstance(
        ResourceLoader & resourceLoader,
        GlobalParameters const & globalParameters)
    {
        return CreateInstance(resourceLoader.GetShadersRootPath(), globalParameters);
    }

    static std::unique_ptr<ShaderManager> CreateInstance(
        std::filesystem::path const & shadersRoot,
        GlobalParameters const & globalParameters)
    {
        return std::unique_ptr<ShaderManager>(
            new ShaderManager(shadersRoot, globalParameters));
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        glUniform1f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            value);

        CheckUniformError<Program, Parameter>();
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        glUniform2f(
            mPrograms[programIndex].UniformLocations[parameterIndex], 
            val1,
            val2);

        CheckUniformError<Program, Parameter>();
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2, float val3)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        glUniform3f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            val1, 
            val2, 
            val3);

        CheckUniformError<Program, Parameter>();
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float val1, float val2, float val3, float val4)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        glUniform4f(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            val1,
            val2,
            val3,
            val4);

        CheckUniformError<Program, Parameter>();
    }

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    inline void SetProgramParameter(float const(&value)[4][4])
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(Program);
        constexpr uint32_t parameterIndex = static_cast<uint32_t>(Parameter);

        glUniformMatrix4fv(
            mPrograms[programIndex].UniformLocations[parameterIndex],
            1,
            GL_FALSE,
            &(value[0][0]));

        CheckUniformError<Program, Parameter>();
    }

    template <typename Traits::ProgramType Program>
    inline void ActivateProgram()
    {
        uint32_t const programIndex = static_cast<uint32_t>(Program);

        glUseProgram(*(mPrograms[programIndex].OpenGLHandle));
    }

private:

    template <typename Traits::ProgramType Program, typename Traits::ProgramParameterType Parameter>
    static void CheckUniformError()
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting uniform for parameter \"" + Traits::ProgramParameterTypeToStr(Parameter) + "\" on program \"" + Traits::ProgramTypeToStr(Program) + "\"");
        }
    }

private:

    ShaderManager(
        std::filesystem::path const & shadersRoot,
        GlobalParameters const & globalParameters);

    void CompileShader(
        std::filesystem::path const & shaderFilepath,
        std::map<std::string, std::string> const & staticParameters);

    static std::tuple<std::string, std::string> SplitSource(std::string const & source);

    static void ParseLocalStaticParameters(
        std::string const & localStaticParametersSource,
        std::map<std::string, std::string> & staticParameters);

    static std::string SubstituteStaticParameters(
        std::string const & source,
        std::map<std::string, std::string> const & staticParameters);

    static std::set<typename Traits::ProgramParameterType> ExtractShaderParameters(std::string const & source);

    static std::set<typename Traits::VertexAttributeType> ExtractVertexAttributes(std::string const & source);

private:

    struct ProgramInfo
    {
        // The OpenGL handle to the program
        GameOpenGLShaderProgram OpenGLHandle;

        // The uniform locations, indexed by shader parameter type
        std::vector<GLint> UniformLocations;
    };

    // All programs, indexed by program type
    std::vector<ProgramInfo> mPrograms;

private:

    friend class ShaderManagerTests_SplitsShaders_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingVertexSection_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingVertexSection_EmptyFile_Test;
    friend class ShaderManagerTests_SplitsShaders_ErrorsOnMissingFragmentSection_Test;

    friend class ShaderManagerTests_ParsesStaticParameters_Single_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_Multiple_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_Multiple_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_ErrorsOnRepeatedParameter_Test;
    friend class ShaderManagerTests_ParsesStaticParameters_ErrorsOnMalformedDefinition_Test;

    friend class ShaderManagerTests_SubstitutesStaticParameters_Single_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_Multiple_Different_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_Multiple_Repeated_Test;
    friend class ShaderManagerTests_SubstitutesStaticParameters_ErrorsOnUnrecognizedParameter_Test;

    friend class ShaderManagerTests_ExtractsShaderParameters_Single_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_Multiple_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_ErrorsOnUnrecognizedParameter_Test;
    friend class ShaderManagerTests_ExtractsShaderParameters_ErrorsOnRedefinedParameter_Test;

    friend class ShaderManagerTests_ExtractsVertexAttributes_Single_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributes_Multiple_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributes_ErrorsOnUnrecognizedAttribute_Test;
    friend class ShaderManagerTests_ExtractsVertexAttributes_ErrorsOnRedeclaredAttribute_Test;
};

}

#include "ShaderManager.cpp.inl"
