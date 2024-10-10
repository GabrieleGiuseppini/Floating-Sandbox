/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/Vectors.h>

#include <cstdint>

namespace Render {

//
// Texture
//

struct TextureCoordinatesQuad
{
    float LeftX;
    float RightX;
    float BottomY;
    float TopY;

    TextureCoordinatesQuad FlipH() const
    {
        return TextureCoordinatesQuad({
            RightX,
            LeftX,
            BottomY,
            TopY });
    }
};

//
// Text
//

/*
 * The different types of fonts.
 */
enum class FontType
{
    Font0 = 0,
    Font1 = 1,
    Font2 = 2, // 7-segment

    _Last = Font2
};

/*
 * Describes a vertex of a text quad, with all the information necessary to the shader.
 */
#pragma pack(push, 1)
struct TextQuadVertex
{
    float positionNdcX;
    float positionNdcY;
    float textureCoordinateX;
    float textureCoordinateY;
    float alpha;

    TextQuadVertex(
        float _positionNdcX,
        float _positionNdcY,
        float _textureCoordinateX,
        float _textureCoordinateY,
        float _alpha)
        : positionNdcX(_positionNdcX)
        , positionNdcY(_positionNdcY)
        , textureCoordinateX(_textureCoordinateX)
        , textureCoordinateY(_textureCoordinateY)
        , alpha(_alpha)
    {}
};
#pragma pack(pop)

//
// Statistics
//

struct RenderStatistics
{
    std::uint64_t LastRenderedShipPoints;
    std::uint64_t LastRenderedShipRopes;
    std::uint64_t LastRenderedShipSprings;
    std::uint64_t LastRenderedShipTriangles;
    std::uint64_t LastRenderedShipPlanes;
    std::uint64_t LastRenderedShipFlames;
    std::uint64_t LastRenderedShipGenericMipMappedTextures;

    RenderStatistics() noexcept
    {
        Reset();
    }

    void Reset() noexcept
    {
        LastRenderedShipPoints = 0;
        LastRenderedShipRopes = 0;
        LastRenderedShipSprings = 0;
        LastRenderedShipTriangles = 0;
        LastRenderedShipPlanes = 0;
        LastRenderedShipFlames = 0;
        LastRenderedShipGenericMipMappedTextures = 0;
    }
};

//
// Misc
//

/*
 * Frontier coloring metadata.
 */
#pragma pack(push, 1)
struct FrontierColor
{
    vec3f frontierBaseColor;
    float positionalProgress;

    FrontierColor(
        vec3f _frontierBaseColor,
        float _positionalProgress)
        : frontierBaseColor(_frontierBaseColor)
        , positionalProgress(_positionalProgress)
    {}
};
#pragma pack(pop)

/*
 * The positions at which UI elements may be anchored.
 */
enum class AnchorPositionType
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

}
