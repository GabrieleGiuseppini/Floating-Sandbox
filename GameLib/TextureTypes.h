/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-30
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

using TextureFrameIndex = std::uint16_t;

enum class TextureGroupType : uint16_t
{
    Cloud = 0,
    Land = 1,
    PinnedPoint = 2,
    RcBomb = 3,
    RcBombExplosion = 4,
    RcBombPing = 5,
    TimerBomb = 6,
    TimerBombDefuse = 7,
    TimerBombExplosion = 8,
    TimerBombFuse = 9,
    Water = 10,

    _Count = 11
};

/*
 * Describes the global identifier of a texture frame.
 */
struct TextureFrameId
{
    TextureGroupType Group;
    TextureFrameIndex FrameIndex;

    TextureFrameId(
        TextureGroupType group,
        TextureFrameIndex frameIndex)
        : Group(group)
        , FrameIndex(frameIndex)
    {}


    TextureFrameId & operator=(TextureFrameId const & other) = default;

    inline bool operator<(TextureFrameId const & other) const
    {
        return this->Group < other.Group
            || (this->Group == other.Group && this->FrameIndex < other.FrameIndex);
    }
};

/*
 * Describes a vertex of a texture, with all the information necessary to the shader.
 */
#pragma pack(push)
struct TextureRenderPolygonVertex
{
    vec2f position;
    vec2f textureCoordinate;

    // When 1.0, totally subject to ambient light; when 0.0, totally independent from it
    float ambientLightSensitivity;

    TextureRenderPolygonVertex(
        vec2f _position,
        vec2f _textureCoordinate,
        float _ambientLightSensitivity)
        : position(_position)
        , textureCoordinate(_textureCoordinate)
        , ambientLightSensitivity(_ambientLightSensitivity)
    {}
};
#pragma pack(pop)