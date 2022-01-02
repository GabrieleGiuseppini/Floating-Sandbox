/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2021-09-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameTypes.h"

#include <algorithm>
#include <cstdint>
#include <limits>

class BigEndianess
{
public:

    static bool ShouldSwap()
    {
        static uint16_t const swapTest = 1;
        return (*((char *)&swapTest) == 1);
    }
};

class LittleEndianess
{
public:

    static bool ShouldSwap()
    {
        return !BigEndianess::ShouldSwap();
    }
};

template<typename TEndianess, typename T>
class Endian
{
public:

    static size_t Read(unsigned char const * ptr, T & value) noexcept;

    static size_t Write(T const & value, unsigned char * ptr) noexcept;
};

template<typename TEndianess>
class Endian<std::uint8_t, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, std::uint8_t & value) noexcept
    {
        value  = *(reinterpret_cast<std::uint8_t const *>(ptr));
        return sizeof(std::uint8_t);
    }

    static size_t Write(std::uint8_t const & value, unsigned char * ptr) noexcept
    {
        *reinterpret_cast<std::uint8_t *>(ptr) = value;
        return sizeof(std::uint8_t);
    }
};

template<typename TEndianess>
class Endian<std::uint16_t, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, std::uint16_t & value) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            value =(static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
        }
        else
        {
            value = *(reinterpret_cast<std::uint16_t const *>(ptr));
        }

        return sizeof(std::uint16_t);
    }

    static size_t Write(std::uint16_t const & value, unsigned char * ptr) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
            ptr[0] = vPtr[1];
            ptr[1] = vPtr[0];
        }
        else
        {
            *reinterpret_cast<std::uint16_t *>(ptr) = value;
        }

        return sizeof(std::uint16_t);
    }
};

template<typename TEndianess>
class Endian<std::uint32_t, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, std::uint32_t & value) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            value = (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16)
                | (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
        }
        else
        {
            value = *(reinterpret_cast<std::uint32_t const *>(ptr));
        }

        return sizeof(std::uint32_t);
    }

    static size_t Write(std::uint32_t const & value, unsigned char * ptr) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
            ptr[0] = vPtr[3];
            ptr[1] = vPtr[2];
            ptr[2] = vPtr[1];
            ptr[3] = vPtr[0];
        }
        else
        {
            *reinterpret_cast<std::uint32_t *>(ptr) = value;
        }

        return sizeof(std::uint32_t);
    }
};

template<typename TEndianess>
class Endian<std::int32_t, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, std::int32_t & value) noexcept
    {
        std::uint32_t uValue;
        size_t const sz = Endian<std::uint32_t, TEndianess>::Read(ptr, uValue);
        value = static_cast<std::int32_t>(uValue);
        return sz;
    }

    static size_t Write(std::int32_t const & value, unsigned char * ptr) noexcept
    {
        std::uint32_t const uValue = static_cast<std::uint32_t>(value);
        return Endian<std::uint32_t, TEndianess>::Write(uValue, ptr);
    }
};

template<typename TEndianess>
class Endian<std::uint64_t, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, std::uint64_t & value) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            value = (static_cast<std::uint64_t>(ptr[0]) << 56) | (static_cast<std::uint64_t>(ptr[1]) << 48)
                | (static_cast<std::uint64_t>(ptr[2]) << 40) | (static_cast<std::uint64_t>(ptr[3]) << 32)
                | (static_cast<std::uint64_t>(ptr[4]) << 24) | (static_cast<std::uint64_t>(ptr[5] << 16))
                | (static_cast<std::uint64_t>(ptr[6]) << 8) | static_cast<std::uint64_t>(ptr[7]);
        }
        else
        {
            value = *(reinterpret_cast<std::uint64_t const *>(ptr));
        }

        return sizeof(std::uint64_t);
    }

    static size_t Write(std::uint64_t const & value, unsigned char * ptr) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
            ptr[0] = vPtr[7];
            ptr[1] = vPtr[6];
            ptr[2] = vPtr[5];
            ptr[3] = vPtr[4];
            ptr[4] = vPtr[3];
            ptr[5] = vPtr[2];
            ptr[6] = vPtr[1];
            ptr[7] = vPtr[0];
        }
        else
        {
            *reinterpret_cast<std::uint64_t *>(ptr) = value;
        }

        return sizeof(std::uint64_t);
    }
};

template<typename TEndianess>
class Endian<float, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, float & value) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            unsigned char buffer[4];
            buffer[0] = ptr[3];
            buffer[1] = ptr[2];
            buffer[2] = ptr[1];
            buffer[3] = ptr[0];
            value = *(reinterpret_cast<float const *>(buffer));
        }
        else
        {
            value = *(reinterpret_cast<float const *>(ptr));
        }

        return sizeof(float);
    }

    static size_t Write(float const & value, unsigned char * ptr) noexcept
    {
        if (TEndianess::ShouldSwap())
        {
            unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
            ptr[0] = vPtr[3];
            ptr[1] = vPtr[2];
            ptr[2] = vPtr[1];
            ptr[3] = vPtr[0];
        }
        else
        {
            *reinterpret_cast<float *>(ptr) = value;
        }

        return sizeof(float);
    }
};

template<typename TEndianess>
class Endian<bool, TEndianess>
{
public:

    static size_t Read(unsigned char const * ptr, bool & value) noexcept
    {
        value = static_cast<bool>(*ptr);
        return sizeof(unsigned char);
    }

    static size_t Write(bool const & value, unsigned char * ptr) noexcept
    {
        *ptr = static_cast<unsigned char>(value);
        return sizeof(unsigned char);
    }
};

template<typename TEndianess>
class Endian<var_uint16_t, TEndianess>
{
public:

    //
    // Note: here we have the same behavior regardless of big vs little endianess

    static size_t Read(unsigned char const * ptr, var_uint16_t & value) noexcept
    {
        std::uint8_t const value1 = static_cast<std::uint8_t>(ptr[0]);
        if (value1 <= 0x7f)
        {
            value = var_uint16_t(static_cast<std::uint16_t>(value1));
            return sizeof(std::uint8_t);
        }
        else
        {
            std::uint8_t const value2 = static_cast<std::uint8_t>(ptr[1]);
            value = var_uint16_t(
                static_cast<std::uint16_t>(value1 & 0x7f)
                | (static_cast<std::uint16_t>(value2) << 7));
            return sizeof(std::uint16_t);
        }
    }

    static size_t Write(var_uint16_t const & value, unsigned char * ptr) noexcept
    {
        assert(value.value() >= std::numeric_limits<var_uint16_t>::min().value() && value.value() <= std::numeric_limits<var_uint16_t>::max().value());

        if (value.value() <= 0x7f)
        {
            ptr[0] = static_cast<unsigned char>(static_cast<std::uint8_t>(value.value()));
            return sizeof(std::uint8_t);
        }
        else
        {
            ptr[0] = static_cast<unsigned char>(static_cast<std::uint8_t>(0x80 | (value.value() & 0x7f)));
            ptr[1] = static_cast<unsigned char>(static_cast<std::uint8_t>(value.value() >> 7));
            return sizeof(std::uint16_t);
        }
    }
};

////////////////////////////////////////////////////////////////

template <typename T>
using BigEndian = Endian<T, BigEndianess>;

template <typename T>
using LittleEndian = Endian<T, LittleEndianess>;
