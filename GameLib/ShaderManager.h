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
    inline void BindAttributeLocation(
        GLuint attributeLocationIndex,
        std::string const & attributeName)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);

        glBindAttribLocation(
            *(mPrograms[programIndex].OpenGLHandle), 
            attributeLocationIndex, 
            attributeName.c_str());

        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error binding attribute location \"" + attributeName + "\" for program \"" + ProgramTypeToStr(_ProgramType) + "\"");
        }
    }

    template <ProgramType _ProgramType>
    void TODOTEST_Relink()
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        GameOpenGL::LinkShaderProgram(mPrograms[programIndex].OpenGLHandle, "TODO");

        std::vector<GLint> newUniformLocations;
        for (auto i = 0; i < mPrograms[programIndex].UniformLocations.size(); ++i)
        {
            if (static_cast<DynamicParameterType>(i) == DynamicParameterType::OrthoMatrix
                || static_cast<DynamicParameterType>(i) == DynamicParameterType::TextureScaling
                || static_cast<DynamicParameterType>(i) == DynamicParameterType::AmbientLightIntensity)
            {
                GLint ul = GameOpenGL::GetParameterLocation(
                    mPrograms[programIndex].OpenGLHandle,
                    "param" + DynamicParameterTypeToStr(static_cast<DynamicParameterType>(i)));

                newUniformLocations.push_back(ul);
            }
            else
            {
                newUniformLocations.push_back(0);
            }
        }

        mPrograms[programIndex].UniformLocations.swap(newUniformLocations);
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float value)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform1f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            value);

        CheckUniformError<_ProgramType, _DynamicParameterType>();
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float val1, float val2)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform2f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            val1,
            val2);

        CheckUniformError<_ProgramType, _DynamicParameterType>();
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float val1, float val2, float val3)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform3f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex], 
            val1, 
            val2, 
            val3);

        CheckUniformError<_ProgramType, _DynamicParameterType>();
    }

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    inline void SetDynamicParameter(float val1, float val2, float val3, float val4)
    {
        constexpr uint32_t programIndex = static_cast<uint32_t>(_ProgramType);
        constexpr uint32_t dynamicParameterIndex = static_cast<uint32_t>(_DynamicParameterType);

        glUniform4f(
            mPrograms[programIndex].UniformLocations[dynamicParameterIndex],
            val1,
            val2,
            val3,
            val4);

        CheckUniformError<_ProgramType, _DynamicParameterType>();
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

        CheckUniformError<_ProgramType, _DynamicParameterType>();
    }

    template <ProgramType _ProgramType>
    inline void ActivateProgram()
    {
        uint32_t const programIndex = static_cast<uint32_t>(_ProgramType);

        glUseProgram(*(mPrograms[programIndex].OpenGLHandle));
    }

private:

    template <ProgramType _ProgramType, DynamicParameterType _DynamicParameterType>
    static void CheckUniformError()
    {
        GLenum glError = glGetError();
        if (GL_NO_ERROR != glError)
        {
            throw GameException("Error setting uniform for parameter \"" + DynamicParameterTypeToStr(_DynamicParameterType) + "\" on program \"" + ProgramTypeToStr(_ProgramType) + "\"");
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

    static std::set<DynamicParameterType> ExtractDynamicParameters(std::string const & source);

    static ShaderManager::ProgramType StrToProgramType(std::string const & str);
    static std::string ProgramTypeToStr(ShaderManager::ProgramType programType);
    static ShaderManager::DynamicParameterType StrToDynamicParameterType(std::string const & str);
    static std::string DynamicParameterTypeToStr(ShaderManager::DynamicParameterType dynamicParameterType);

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

