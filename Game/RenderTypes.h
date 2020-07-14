/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>

namespace Render {

//
// Text
//

/*
 * The different fonts available.
 */
enum class FontType
{
    // Indices must match suffix of filename
    StatusText = 0,
    GameText = 1,

    _Last = GameText
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

    RenderStatistics()
    {
        Reset();
    }

    void Reset()
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
