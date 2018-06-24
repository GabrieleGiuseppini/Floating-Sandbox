/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "RotatedRectangle.h"
#include "Vectors.h"

#include <algorithm>
#include <cmath>
#include <limits>

/*
 * Contains all the information necessary to render a rotated texture.
 */
struct RotatedTextureRenderInfo
{
    vec2f CenterPosition;
    float Scale;
    vec2f RotationBaseAxis;
    vec2f RotationOffsetAxis;

    RotatedTextureRenderInfo(
        vec2f const & centerPosition,
        float scale,
        vec2f const & rotationBaseAxis,
        vec2f const & rotationOffsetAxis)
        : CenterPosition(centerPosition)
        , Scale(scale)
        , RotationBaseAxis(rotationBaseAxis)
        , RotationOffsetAxis(rotationOffsetAxis)
    {}

    RotatedRectangle CalculateRotatedRectangle(
        float textureWidth,
        float textureHeight) const
    {
        //
        // Calculate rotation matrix, based off cosine of the angle between rotation offset and rotation base
        //

        float const alpha = RotationBaseAxis.angle(RotationOffsetAxis);

        float const cosAlpha = cosf(alpha);
        float const sinAlpha = sinf(alpha);

        // First column
        vec2f const rotationMatrixX(cosAlpha, sinAlpha);

        // Second column
        vec2f const rotationMatrixY(-sinAlpha, cosAlpha);


        //
        // Calculate rectangle vertices
        //
        
        float const leftX = -textureWidth * Scale / 2.0f;
        float const rightX = textureWidth * Scale / 2.0f;
        float const topY = -textureHeight * Scale / 2.0f;
        float const bottomY = textureHeight * Scale / 2.0f;

        vec2f const topLeft{ leftX, topY };
        vec2f const topRight{ rightX, topY };
        vec2f const bottomLeft{ leftX, bottomY };
        vec2f const bottomRight{ rightX, bottomY };

        return RotatedRectangle(
            { topLeft.dot(rotationMatrixX) + CenterPosition.x, topLeft.dot(rotationMatrixY) + CenterPosition.y },
            { topRight.dot(rotationMatrixX) + CenterPosition.x, topRight.dot(rotationMatrixY) + CenterPosition.y },
            { bottomLeft.dot(rotationMatrixX) + CenterPosition.x, bottomLeft.dot(rotationMatrixY) + CenterPosition.y },
            { bottomRight.dot(rotationMatrixX) + CenterPosition.x, bottomRight.dot(rotationMatrixY) + CenterPosition.y });
    }
};
