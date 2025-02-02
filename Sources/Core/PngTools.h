/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-14
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include "BinaryStreams.h"
#include "ImageData.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

/*
 * PNG encoder and decoder for the game.
 *
 * Standard image formats: 8-bit per-channel, RGB or RGBA, left-bottom origin.
 */
class PngTools
{
public:

    static RgbaImageData DecodeImageRgba(BinaryReadStream & pngImageData);
    static RgbImageData DecodeImageRgb(BinaryReadStream & pngImageData);

    static ImageSize GetImageSize(BinaryReadStream & pngImageData);

    static void EncodeImage(RgbaImageData const & image, BinaryWriteStream & outputStream);
    static void EncodeImage(RgbImageData const & image, BinaryWriteStream & outputStream);

private:

    template<typename TImageData>
    static TImageData InternalDecodeImage(BinaryReadStream & pngImageData);

    template<typename TImageData>
    static void InternalEncodeImage(TImageData const & image, BinaryWriteStream & outputStream);
};