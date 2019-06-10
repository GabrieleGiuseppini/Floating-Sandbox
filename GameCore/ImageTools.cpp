/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-02-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageTools.h"

void ImageTools::BlendWithColor(
    RgbaImageData & imageData,
    rgbColor const & color,
    float alpha)
{
    size_t const Size = imageData.Size.Width * imageData.Size.Height;
    for (size_t i = 0; i < Size; ++i)
    {
        imageData.Data[i] = imageData.Data[i].mix(color, alpha);
    }
}