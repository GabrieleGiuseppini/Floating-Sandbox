/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-14
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <png.h> // Include libpng right now, before we pollute the global namespace with e.g. "restrict"

#include "PngTools.h"

#include "Log.h"

#include <cstring>

#ifdef _MSC_VER
#pragma warning( disable: 4611 )  // Interaction between setjmp and C++ is not portable
#endif

namespace _detail
{
    void ThrowErrorDecodingPng()
    {
        throw std::runtime_error("Error reading PNG file");
    }

    void ThrowErrorUnsupportedPng()
    {
        throw std::runtime_error("This PNG format is not supported");
    }

    struct PngDecodeContext
    {
        Buffer<std::uint8_t> raw_data_buffer;
        png_structp png_ptr;
        png_infop info_ptr;

        png_size_t raw_data_buffer_read_offset;

        PngDecodeContext(
            Buffer<std::uint8_t> && _raw_data_buffer,
            png_structp _png_ptr,
            png_infop _info_ptr)
            : raw_data_buffer(std::move(_raw_data_buffer))
            , png_ptr(_png_ptr)
            , info_ptr(_info_ptr)
            , raw_data_buffer_read_offset(0)
        {}

        ~PngDecodeContext()
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        }
    };

    void ReadPngDataCallback(
        png_structp png_ptr,
        png_byte * png_data,
        png_size_t read_length)
    {
        auto * png_read_ctx_ptr = reinterpret_cast<PngDecodeContext *>(png_get_io_ptr(png_ptr));
        png_byte const * const raw_data_src = png_read_ctx_ptr->raw_data_buffer.data() + png_read_ctx_ptr->raw_data_buffer_read_offset;

        // Read
        memcpy(
            png_data,
            raw_data_src,
            read_length);

        // Advance
        png_read_ctx_ptr->raw_data_buffer_read_offset += read_length;
    }

    std::unique_ptr<PngDecodeContext> DecodeProlog(Buffer<std::uint8_t> && pngImageData)
    {
        // Sanity checks

        if (pngImageData.GetSize() <= 8
            || !png_check_sig(reinterpret_cast<png_const_bytep>(pngImageData.data()), 8))
        {
            ThrowErrorDecodingPng();
        }

        // Allocate structs

        png_structp png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            nullptr,
            nullptr,
            nullptr);
        assert(png_ptr != nullptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        assert(info_ptr != nullptr);

        // Create context

        auto context = std::make_unique<PngDecodeContext>(
            std::move(pngImageData),
            png_ptr,
            info_ptr);

        // Set buffer reading callback

        png_set_read_fn(png_ptr, context.get(), ReadPngDataCallback);

        return context;
    }

    /////

    void ThrowErrorEncodingPng()
    {
        throw std::runtime_error("Error encoding PNG file");
    }

    struct PngEncodeContext
    {
        std::vector<std::uint8_t> & raw_data_buffer;
        png_structp png_ptr;
        png_infop info_ptr;

        PngEncodeContext(
            std::vector<std::uint8_t> & _raw_data_buffer,
            png_structp _png_ptr,
            png_infop _info_ptr)
            : raw_data_buffer(_raw_data_buffer)
            , png_ptr(_png_ptr)
            , info_ptr(_info_ptr)
        {}

        ~PngEncodeContext()
        {
            png_destroy_write_struct(&png_ptr, &info_ptr);
        }
    };

    void WritePngDataCallback(
        png_structp png_ptr,
        png_byte * png_data,
        png_size_t write_length)
    {
        auto * png_write_ctx_ptr = reinterpret_cast<PngEncodeContext *>(png_get_io_ptr(png_ptr));
        png_write_ctx_ptr->raw_data_buffer.insert(
            png_write_ctx_ptr->raw_data_buffer.end(),
            png_data,
            png_data + write_length);
    }

    std::unique_ptr<PngEncodeContext> EncodeProlog(std::vector<std::uint8_t> & buffer)
    {
        // Allocate structs

        png_structp png_ptr = png_create_write_struct(
            PNG_LIBPNG_VER_STRING,
            nullptr,
            nullptr,
            nullptr);
        assert(png_ptr != nullptr);

        png_infop info_ptr = png_create_info_struct(png_ptr);
        assert(info_ptr != nullptr);

        // Create context

        auto context = std::make_unique<PngEncodeContext>(
            buffer,
            png_ptr,
            info_ptr);

        // Set buffer writing callback

        png_set_write_fn(png_ptr, context.get(), _detail::WritePngDataCallback, nullptr);

        return context;
    }
}

RgbaImageData PngTools::DecodeImageRgba(Buffer<std::uint8_t> && pngImageData)
{
    return InternalDecodeImage<RgbaImageData>(std::move(pngImageData));
}

RgbImageData PngTools::DecodeImageRgb(Buffer<std::uint8_t> && pngImageData)
{
    return InternalDecodeImage<RgbImageData>(std::move(pngImageData));
}

ImageSize PngTools::GetImageSize(Buffer<std::uint8_t> && pngImageData)
{
    auto context = _detail::DecodeProlog(std::move(pngImageData));

    // Setup error callback
    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        // An error occurred
        _detail::ThrowErrorDecodingPng();
    }

    // Get info
    png_uint_32 width, height;
    {
        png_read_info(
            context->png_ptr,
            context->info_ptr);

        png_get_IHDR(
            context->png_ptr,
            context->info_ptr,
            &width,
            &height,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
    }

    // Do not call png_read_end, as we haven't read it all

    return {
        static_cast<int>(width),
        static_cast<int>(height) };
}

Buffer<std::uint8_t> PngTools::EncodeImage(RgbaImageData const & image)
{
    return InternalEncodeImage<RgbaImageData>(image);
}

Buffer<std::uint8_t> PngTools::EncodeImage(RgbImageData const & image)
{
    return InternalEncodeImage<RgbImageData>(image);
}

///////////////////////////////////////////////////

template<typename TImageData>
TImageData PngTools::InternalDecodeImage(Buffer<std::uint8_t> && pngImageData)
{
    auto context = _detail::DecodeProlog(std::move(pngImageData));

    //
    // Setup error callback
    //

    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        // An error occurred
        _detail::ThrowErrorDecodingPng();
    }

    //
    // Get info and convert
    //

    png_uint_32 width, height;
    {
        int bit_depth, color_type, interlace_type;

        png_read_info(
            context->png_ptr,
            context->info_ptr);

        png_get_IHDR(
            context->png_ptr,
            context->info_ptr,
            &width,
            &height,
            &bit_depth,
            &color_type,
            &interlace_type,
            nullptr,
            nullptr);

        bool const has_tRNS = png_get_valid(
            context->png_ptr,
            context->info_ptr,
            PNG_INFO_tRNS);

        LogMessage("color_type=", color_type, " bit_depth=", bit_depth, " interlace_type=", interlace_type, " has_tRNS=", has_tRNS);

        //
        // Build transformation plan
        //
        // From libpngmanual.txt, "Input Transformations" section
        //

        char transformation_plan[4] = { 0x00, 0x00, 0x00, 0x00 }; // Up to 3, null-terminated
        int iPlan = 0;

        if constexpr (TImageData::element_type::channel_count == 3)
        {
            //
            // To RGB (row 2)
            //

            // Grayscale no Alpha (0)
            if (color_type == PNG_COLOR_TYPE_GRAY)
            {
                // 01, 0, 0T, 0O

                // png_set_gray_to_rgb
                //  * expands to 8-bit
                transformation_plan[iPlan++] = 'C';

                // We ignore tRNS
            }
            // Indexed-Color no Alpha (3)
            else if (color_type == PNG_COLOR_TYPE_PALETTE)
            {
                // 31, 3, 3T, 30

                // tRNS is not supported when converting to RGB (according to manual and verified),
                // because it forces an output alpha channel messing up everything.
                // Note: we could force conversion to RGBA here and drop the alpha channel ourselves.
                if (has_tRNS)
                {
                    _detail::ThrowErrorUnsupportedPng();
                }

                // png_set_palette_to_rgb
                //  * expands to 8-bit
                //  * expands tRNS (which we verified we don't have)
                transformation_plan[iPlan++] = 'P'; // Note: manual says C for 3
            }
            // RGB no Alpha (2)
            else if (color_type == PNG_COLOR_TYPE_RGB)
            {
                // 2, 2T, 20

                // NOP

                // tRNS will be ignored
            }
            // Grayscale with Alpha (4)
            else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            {
                // 4A, 4O

                // tRNS prohibited with alpha
                if (has_tRNS)
                {
                    _detail::ThrowErrorUnsupportedPng();
                }

                // png_set_gray_to_rgb
                //  * expands to 8-bit
                transformation_plan[iPlan++] = 'C';
                // png_set_background
                //  * flattens alpha or tRNS
                transformation_plan[iPlan++] = 'B';
            }
            // RGB with Alpha (6)
            else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            {
                // 6A, 6O

                // tRNS prohibited with alpha
                if (has_tRNS)
                {
                    _detail::ThrowErrorUnsupportedPng();
                }

                // png_set_background
                //  * flattens alpha or tRNS to bg color
                transformation_plan[iPlan++] = 'B';
            }
            else
            {
                _detail::ThrowErrorUnsupportedPng();
            }
        }
        else
        {
            assert(TImageData::element_type::channel_count == 4);

            //
            // To RGBA (row 6A)
            //

            // Grayscale no Alpha (0)
            if (color_type == PNG_COLOR_TYPE_GRAY)
            {
                // 01, 0, 0T, 0O

                // png_set_gray_to_rgb
                //  * expands to 8-bit
                transformation_plan[iPlan++] = 'C';

                if (!has_tRNS)
                {
                    // png_set_add_alpha
                    transformation_plan[iPlan++] = 'A';
                }
                else
                {
                    // Note: this is ours, according to manual it's only the C above
                    // png_set_tRNS_to_alpha
                    transformation_plan[iPlan++] = 'T';
                }
            }
            // Indexed-Color no Alpha (3)
            else if (color_type == PNG_COLOR_TYPE_PALETTE)
            {
                // 31, 3, 3T, 30

                // png_set_palette_to_rgb
                //  * expands to 8-bit
                //  * expands tRNS
                transformation_plan[iPlan++] = 'P';

                if (!has_tRNS)
                {
                    // png_set_add_alpha
                    transformation_plan[iPlan++] = 'A';
                }
            }
            // RGB no Alpha (2)
            else if (color_type == PNG_COLOR_TYPE_RGB)
            {
                // 2, 2T, 20

                if (!has_tRNS)
                {
                    // png_set_add_alpha
                    transformation_plan[iPlan++] = 'A';
                }
                else
                {
                    // png_set_tRNS_to_alpha
                    transformation_plan[iPlan++] = 'T';
                }
            }
            // Grayscale with Alpha (4)
            else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            {
                // 4A, 4O

                // png_set_gray_to_rgb
                //  * expands to 8-bit
                transformation_plan[iPlan++] = 'C';

                // tRNS prohibited with alpha
                if (has_tRNS)
                {
                    _detail::ThrowErrorUnsupportedPng();
                }
            }
            // RGB with Alpha (6)
            else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
            {
                // 6A, 6O

                // tRNS prohibited with alpha
                if (has_tRNS)
                {
                    _detail::ThrowErrorUnsupportedPng();
                }

                // NOP
            }
            else
            {
                _detail::ThrowErrorUnsupportedPng();
            }

            ////// TODOOLD

            ////// Grayscale with depth<8 with or without transparency
            ////if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            ////{
            ////    // png_set_gray_to_rgb
            ////    transformation_plan[iPlan++] = 'C';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// Indexed with depth<8 without transparency
            ////else if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth < 8 && !has_tRNS)
            ////{
            ////    // png_set_palette_to_rgb
            ////    transformation_plan[iPlan++] = 'P';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// Grayscale with or without transparency
            ////else if (color_type == PNG_COLOR_TYPE_GRAY && !has_tRNS)
            ////{
            ////    // png_set_gray_to_rgb
            ////    transformation_plan[iPlan++] = 'C';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// Grayscale with transparency
            ////else if (color_type == PNG_COLOR_TYPE_GRAY && has_tRNS)
            ////{
            ////    // png_set_gray_to_rgb
            ////    transformation_plan[iPlan++] = 'C';
            ////    // png_set_tRNS_to_alpha
            ////    transformation_plan[iPlan++] = 'T';
            ////}
            ////// RGB without transparency
            ////else if (color_type == PNG_COLOR_TYPE_RGB && !has_tRNS)
            ////{
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// RGB with transparency
            ////else if (color_type == PNG_COLOR_TYPE_RGB && has_tRNS)
            ////{
            ////    // png_set_tRNS_to_alpha
            ////    transformation_plan[iPlan++] = 'T';
            ////}
            ////// Indexed without transparency
            ////else if (color_type == PNG_COLOR_TYPE_PALETTE && !has_tRNS)
            ////{
            ////    // png_set_palette_to_rgb
            ////    transformation_plan[iPlan++] = 'P';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// Indexed with transparency
            ////else if (color_type == PNG_COLOR_TYPE_PALETTE && has_tRNS)
            ////{
            ////    // png_set_palette_to_rgb
            ////    transformation_plan[iPlan++] = 'P';
            ////}
            ////// Grayscale with alpha without transparency
            ////else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA && !has_tRNS)
            ////{
            ////    // png_set_gray_to_rgb
            ////    transformation_plan[iPlan++] = 'C';
            ////}
            ////// Grayscale without alpha with transparency
            ////else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA && has_tRNS)
            ////{
            ////    // png_set_gray_to_rgb
            ////    transformation_plan[iPlan++] = 'C';
            ////    // png_set_background
            ////    transformation_plan[iPlan++] = 'B';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////// RGB with alpha without transparency
            ////else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA && !has_tRNS)
            ////{
            ////    // Nop
            ////}
            ////// RGB with alpha with transparency
            ////else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA && has_tRNS)
            ////{
            ////    // png_set_background
            ////    transformation_plan[iPlan++] = 'B';
            ////    // png_set_add_alpha
            ////    transformation_plan[iPlan++] = 'A';
            ////}
            ////else
            ////{
            ////    _detail::ThrowErrorUnsupportedPng();
            ////}
        }

        LogMessage("transformation_plan=", transformation_plan);

        //
        // Run transformation plan
        //

        // First off, allow lib to handle interlacing
        png_set_interlace_handling(context->png_ptr);

        for (int i = 0; i < iPlan; ++i)
        {
            if (transformation_plan[i] == 'C')
            {
                png_set_gray_to_rgb(context->png_ptr);
            }
            else if (transformation_plan[i] == 'P')
            {
                png_set_palette_to_rgb(context->png_ptr);
            }
            else if (transformation_plan[i] == 'B')
            {
                png_color_16 background_color = { 0, 0xFF, 0xFF, 0xFF, 0x00 }; // White
                png_set_background(
                    context->png_ptr,
                    &background_color,
                    PNG_BACKGROUND_GAMMA_SCREEN,
                    0,
                    0.0);
            }
            else if (transformation_plan[i] == 'A')
            {
                png_set_add_alpha(
                    context->png_ptr,
                    0xFF, // Full opaque by default
                    PNG_FILLER_AFTER);
            }
            else if (transformation_plan[i] == 'T')
            {
                png_set_tRNS_to_alpha(context->png_ptr);
            }
            else if (transformation_plan[i] == 't')
            {
                unsigned char trans_alpha = 0xff;

                png_color_16 trans_value = {0, 0xFF, 0xFF, 0xFF, 0x00};

                png_set_tRNS(
                    context->png_ptr,
                    context->info_ptr,
                    &trans_alpha,
                    1,
                    &trans_value);
            }
            else
            {
                assert(false);
            }
        }

        // TODOHERE

        ////// TODOOLD
        ////// Convert transparency to full alpha
        ////if (png_get_valid(
        ////    context->png_ptr,
        ////    context->info_ptr,
        ////    PNG_INFO_tRNS))
        ////{
        ////    png_set_tRNS_to_alpha(context->png_ptr);
        ////}

        ////// Convert grayscale to 8-bit, if needed
        ////if ((color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) && bit_depth < 8)
        ////{
        ////    png_set_expand_gray_1_2_4_to_8(context->png_ptr);
        ////}

        ////// Convert grayscale to RGB
        ////if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        ////{
        ////    png_set_gray_to_rgb(context->png_ptr);
        ////}

        ////// Convert paletted images to RGB, if needed
        ////if (color_type == PNG_COLOR_TYPE_PALETTE)
        ////{
        ////    png_set_palette_to_rgb(context->png_ptr);
        ////}

        ////// Add/remove alpha channel as needed
        ////if constexpr (TImageData::element_type::has_alpha)
        ////{
        ////    if ((color_type & PNG_COLOR_MASK_ALPHA) == 0)
        ////    {
        ////        png_set_add_alpha(
        ////            context->png_ptr,
        ////            0xFF,
        ////            PNG_FILLER_AFTER);
        ////    }
        ////}
        ////else
        ////{
        ////    if ((color_type & PNG_COLOR_MASK_ALPHA) != 0)
        ////    {
        ////        png_set_strip_alpha(context->png_ptr);
        ////    }
        ////}

        //
        // Ensure 8-bit packing
        //

        if (bit_depth < 8)
        {
            png_set_packing(context->png_ptr);
        }
        else if (bit_depth == 16)
        {
            png_set_scale_16(context->png_ptr);
        }

        //
        // Finalize transformations
        //

        png_read_update_info(
            context->png_ptr,
            context->info_ptr);
    }

    //
    // Read whole
    //

    TImageData image(
        static_cast<int>(width),
        static_cast<int>(height));
    {
        png_size_t const row_size = png_get_rowbytes(context->png_ptr, context->info_ptr);
        assert(row_size > 0);

        std::vector<png_byte *> row_ptrs;
        row_ptrs.resize(height);

        for (png_uint_32 y = 0; y < height; ++y)
        {
            // PNGs scanlines are from top to bottom, but here we fix to bottom->top
            row_ptrs[height - y - 1] = reinterpret_cast<png_byte *>(image.Data.get()) + y * row_size;
        }

        png_read_image(context->png_ptr, row_ptrs.data());
    }

    //
    // We are done
    //

    png_read_end(context->png_ptr, context->info_ptr);

    return image;
}

template<typename TImageData>
Buffer<std::uint8_t> PngTools::InternalEncodeImage(TImageData const & image)
{
    std::vector<std::uint8_t> tmpOutputBuffer;

    auto context = _detail::EncodeProlog(tmpOutputBuffer);

    // Setup error callback
    if (setjmp(png_jmpbuf(context->png_ptr)))
    {
        // An error occurred
        _detail::ThrowErrorEncodingPng();
    }

    // Encode
    {
        // Use provided image's color type
        int color_type;
        if constexpr (TImageData::element_type::channel_count == 3)
        {
            color_type = PNG_COLOR_TYPE_RGB;
        }
        else
        {
            assert(TImageData::element_type::channel_count == 4);
            color_type = PNG_COLOR_TYPE_RGBA;
        }

        png_set_IHDR(
            context->png_ptr,
            context->info_ptr,
            image.Size.width,
            image.Size.height,
            8,
            color_type,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
    }

    // Write whole
    {
        std::vector<png_byte *> row_ptrs;
        row_ptrs.resize(image.Size.height);

        for (int y = 0; y < image.Size.height; ++y)
        {
            // Our image scanlines are from bottom to top, but PNG wants them top to bottom
            row_ptrs[image.Size.height - y - 1] = (png_byte *)image.Data.get() + y * image.Size.width * TImageData::element_type::channel_count * sizeof(png_byte);
        }

        png_set_rows(
            context->png_ptr,
            context->info_ptr,
            &row_ptrs[0]);

        png_write_png(
            context->png_ptr,
            context->info_ptr,
            PNG_TRANSFORM_IDENTITY,
            nullptr);
    }

    // We are done
    png_write_end(context->png_ptr, context->info_ptr);

    // Convert to buffer
    Buffer<std::uint8_t> outputBuffer(tmpOutputBuffer.size());
    std::memcpy(outputBuffer.data(), tmpOutputBuffer.data(), tmpOutputBuffer.size() * sizeof(std::uint8_t));

    return outputBuffer;
}