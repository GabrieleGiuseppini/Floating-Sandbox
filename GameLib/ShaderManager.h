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
        GenericTextures,
        Land,
        Matte,
        MatteNDC,
        ShipRopes,
        ShipStressedSprings,
        ShipTrianglesColor,
        ShipTrianglesTexture,
        VectorArrows,
        Water,

        _Last = Water
    };

    enum class DynamicParameterType : uint32_t
    {
        AmbientLightIntensity = 0,
        MatteColor,
        OrthoMatrix,
        TextureScaling,
        WaterLevelThreshold,
        WaterTransparency
    };

    struct GlobalParameters
    {
        vec3f RopeColor;

        GlobalParameters(
            vec3f ropeColor)
            : RopeColor(ropeColor)
        {}

        void ToParameters(std::map<std::string, std::string> & parameters) const
        {
            std::stringstream ss;
            ss << std::fixed << RopeColor.x << ", " << RopeColor.y << ", " << RopeColor.z;

            parameters.insert(
                std::make_pair(
                    "ROPE_COLOR_VEC3",
                    ss.str()));
        }
    };

public:

    static std::unique_ptr<ShaderManager> CreateInstance(
        ResourceLoader & resourceLoader,
        GlobalParameters const & globalParameters);

    static std::unique_ptr<ShaderManager> CreateInstance(
        std::filesystem::path const & shadersRoot,
        GlobalParameters const & globalParameters);

    template <ProgramType _ProgramType>
    inline void ActivateProgram()
    {
        uint32_t const programIndex = static_cast<uint32_t>(_ProgramType);

        glUseProgram(*(mPrograms[programIndex].OpenGLHandle));
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform1f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value);
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(vec2f const & value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform2f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y);
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(vec3f const & value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform3f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y, 
            value.z);
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(vec4f const & value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform4f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value.x, 
            value.y, 
            value.z, 
            value.w);
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float const(&value)[4][4])
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

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

