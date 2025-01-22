/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-14
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include "Buffer.h"
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

    static RgbaImageData DecodeImageRgba(Buffer<std::uint8_t> && pngImageData);
    static RgbImageData DecodeImageRgb(Buffer<std::uint8_t> && pngImageData);

    static ImageSize GetImageSize(Buffer<std::uint8_t> && pngImageData);

    static Buffer<std::uint8_t> EncodeImage(RgbaImageData const & image);
    static Buffer<std::uint8_t> EncodeImage(RgbImageData const & image);

private:

    template<typename TImageData>
    static TImageData InternalDecodeImage(Buffer<std::uint8_t> && pngImageData);

    template<typename TImageData>
    static Buffer<std::uint8_t> InternalEncodeImage(TImageData const & image);
};