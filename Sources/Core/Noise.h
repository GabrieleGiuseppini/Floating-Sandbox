/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2024-09-01
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Buffer2D.h"
#include "GameTypes.h"
#include "Vectors.h"

class Noise final
{
public:

	static Buffer2D<float, struct IntegralTag> CreateRepeatableFractal2DPerlinNoise(
		IntegralRectSize const & size,
		int firstGridDensity, // Number of cells
		int lastGridDensity, // Number of cells
		float persistence);

private:

	static Buffer2D<vec2f, struct IntegralTag> MakePerlinVectorGrid(IntegralRectSize const & size);

	static void AddRepeatableUnscaledPerlinNoise(
		Buffer2D<float, struct IntegralTag> & buffer,
		int gridDensity, // Number of cells
		float amplitude);
};