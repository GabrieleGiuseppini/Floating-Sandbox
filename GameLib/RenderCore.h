/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameOpenGL.h"

#include <string>

namespace Render {

    enum class ProgramType
    {
        Clouds = 0,
        GenericTextures,
        Land,
        Matte,
        MatteNDC,
        MatteWater,
        ShipRopes,
        ShipStressedSprings,
        ShipTrianglesColor,
        ShipTrianglesTexture,
        Water,

        _Last = Water
    };

    ProgramType StrToProgramType(std::string const & str);

    std::string ProgramTypeToStr(ProgramType program);

    enum class ProgramParameterType
    {
        AmbientLightIntensity = 0,
        MatteColor,
        OrthoMatrix,
        TextureScaling,
        WaterLevelThreshold,
        WaterTransparency
    };

    ProgramParameterType StrToProgramParameterType(std::string const & str);

    std::string ProgramParameterTypeToStr(ProgramParameterType programParameter);

    enum class VertexAttributeType : GLuint
    {
        //
        // Vertex attributes sourced from multiple VBO's
        //

        SharedPosition = 0,
        SharedTextureCoordinates = 1,
        Shared1XFloat = 2,


        //
        // Vertex attributes dedicated to a VBO
        //

        WaterPosition = 3,

        GenericTexturePosition = 4,
        GenericTextureCoordinates = 5,
        GenericTextureAmbientLightSensitivity = 6,

        // TODO: dedicated as long as we have one single ship and one VBO per ship
        ShipPointPosition = 7,
        ShipPointColor = 8,
        ShipPointLight = 9,
        ShipPointWater = 10,
        ShipPointTextureCoordinates = 11
    };

    VertexAttributeType StrToVertexAttributeType(std::string const & str);

    std::string VertexAttributeTypeToStr(VertexAttributeType vertexAttribute);

    struct ShaderManagerTraits
    {
        using ProgramType = Render::ProgramType;
        using ProgramParameterType = Render::ProgramParameterType;
        using VertexAttributeType = Render::VertexAttributeType;

        static constexpr auto StrToProgramType = Render::StrToProgramType;
        static constexpr auto ProgramTypeToStr = Render::ProgramTypeToStr;
        static constexpr auto StrToProgramParameterType = Render::StrToProgramParameterType;
        static constexpr auto ProgramParameterTypeToStr = Render::ProgramParameterTypeToStr;
        static constexpr auto StrToVertexAttributeType = Render::StrToVertexAttributeType;
        static constexpr auto VertexAttributeTypeToStr = Render::VertexAttributeTypeToStr;
    };
}
