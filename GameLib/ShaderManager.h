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

    enum class ProgramType : uint32_t
    {
        Clouds = 0,
        Water = 1,

        _Last = Water
    };

    enum class DynamicParameterType : uint32_t
    {
        AmbientLightIntensity = 0,
        OrthoMatrix = 1
    };

    struct GlobalParameters
    {
        float ZDepth;

        GlobalParameters(
            float zDepth)
            : ZDepth(zDepth)
        {}

        void ToParameters(std::map<std::string, std::string> & parameters) const
        {
            parameters.insert(
                std::make_pair(
                    "Z_DEPTH",
                    ToString(ZDepth)));
        }
    };

public:

    static std::unique_ptr<ShaderManager> CreateInstance(
        ResourceLoader & resourceLoader,
        GlobalParameters const & globalParameters);

    static std::unique_ptr<ShaderManager> CreateInstance(
        std::filesystem::path const & shadersRoot,
        GlobalParameters const & globalParameters);

    void SetDynamicParameter(
        ProgramType programType,
        DynamicParameterType dynamicParameterType, 
        float value)
    {
        uint32_t const programIndex = static_cast<uint32_t>(programType);
        uint32_t const dynamicParameterIndex = static_cast<uint32_t>(dynamicParameterType);

        glUniform1f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value);
    }

    void SetDynamicParameter(
        ProgramType programType,
        DynamicParameterType dynamicParameterType,
        vec2f const & value)
    {
        uint32_t const programIndex = static_cast<uint32_t>(programType);
        uint32_t const dynamicParameterIndex = static_cast<uint32_t>(dynamicParameterType);

        glUniform2f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y);
    }

    void SetDynamicParameter(
        ProgramType programType,
        DynamicParameterType dynamicParameterType,
        vec3f const & value)
    {
        uint32_t const programIndex = static_cast<uint32_t>(programType);
        uint32_t const dynamicParameterIndex = static_cast<uint32_t>(dynamicParameterType);

        glUniform3f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y, 
            value.z);
    }

    void SetDynamicParameter(
        ProgramType programType,
        DynamicParameterType dynamicParameterType,
        vec4f const & value)
    {
        uint32_t const programIndex = static_cast<uint32_t>(programType);
        uint32_t const dynamicParameterIndex = static_cast<uint32_t>(dynamicParameterType);

        glUniform4f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y, 
            value.z, 
            value.w);
    }

    void SetDynamicParameter(
        ProgramType programType,
        DynamicParameterType dynamicParameterType,
        float const(&value)[4][4])
    {
        uint32_t const programIndex = static_cast<uint32_t>(programType);
        uint32_t const dynamicParameterIndex = static_cast<uint32_t>(dynamicParameterType);

        glUniformMatrix4fv(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex],
            1,
            GL_FALSE,
            &(value[0][0]));
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

    static std::set<DynamicParameterType> ExtractDynamicParameters(std::string const & source);

private:

    struct ProgramInfo
    {
        // The OpenGL handle to the program
        GameOpenGLShaderProgram OpenGLHandle;

        // The uniform locations, indexed by dynamic parameter type
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

    friend class ShaderManagerTests_ExtractsDynamicParameters_Single_Test;
    friend class ShaderManagerTests_ExtractsDynamicParameters_Multiple_Test;
    friend class ShaderManagerTests_ExtractsDynamicParameters_ErrorsOnUnrecognizedParameter_Test;
    friend class ShaderManagerTests_ExtractsDynamicParameters_ErrorsOnRedefinedParameter_Test;
};

